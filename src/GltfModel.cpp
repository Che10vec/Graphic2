#include "GltfModel.h"

#include <iostream>
#include <stdexcept>
#include <type_traits>

/// @note Documentation in this file was written with AI assistance.

enum class AlphaMode
{
    Opaque = 0,
    Mask = 1,
    Blend = 2
};

/// @brief Release all OpenGL resources associated with this model.
/// Destroys textures, VAOs, VBOs, and EBOs. Should be called before destruction.
void ResourcesGPU::DestroyGL()
{
    for (auto &p : primitives)
    {
        if (p.ebo)
            glDeleteBuffers(1, &p.ebo);
        if (p.vbo)
            glDeleteBuffers(1, &p.vbo);
        if (p.vao)
            glDeleteVertexArrays(1, &p.vao);
        p = {};
    }
    primitives.clear();

    for (GLuint t : glTextures)
    {
        if (t)
            glDeleteTextures(1, &t);
    }
    glTextures.clear();
    meshToPrimitiveIndices.clear();
}

/// @brief Get the pointer to buffer data for a glTF accessor.
/// @param model The glTF model
/// @param accessor The accessor describing the data layout
/// @param view The buffer view containing the data
/// @return Pointer to the first element of the accessor data
static const unsigned char *GetBufferDataPtr(const tinygltf::Model &model,
                                             const tinygltf::Accessor &accessor,
                                             const tinygltf::BufferView &view)
{
    const tinygltf::Buffer &buffer = model.buffers[view.buffer];
    return buffer.data.data() + view.byteOffset + accessor.byteOffset;
}

/// @brief Get the byte size of a single glTF component type.
/// @param componentType glTF component type constant (TINYGLTF_COMPONENT_TYPE_*)
/// @return Size in bytes (1, 2, 4, or 8)
static size_t ComponentSizeInBytes(int componentType)
{
    switch (componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        return 1;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return 1;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
        return 2;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return 2;
    case TINYGLTF_COMPONENT_TYPE_INT:
        return 4;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return 4;
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return 4;
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        return 8;
    default:
        return 0;
    }
}

/// @brief Get the number of components in a glTF type.
/// @param type glTF type constant (TINYGLTF_TYPE_SCALAR, TINYGLTF_TYPE_VEC3, etc.)
/// @return Number of components (1, 2, 3, 4, 9, or 16)
static int NumComponentsInType(int type)
{
    switch (type)
    {
    case TINYGLTF_TYPE_SCALAR:
        return 1;
    case TINYGLTF_TYPE_VEC2:
        return 2;
    case TINYGLTF_TYPE_VEC3:
        return 3;
    case TINYGLTF_TYPE_VEC4:
        return 4;
    case TINYGLTF_TYPE_MAT2:
        return 4;
    case TINYGLTF_TYPE_MAT3:
        return 9;
    case TINYGLTF_TYPE_MAT4:
        return 16;
    default:
        return 0;
    }
}

/// @brief Read a float vector accessor from a glTF model.
/// Extracts floating-point data from a glTF accessor and interprets it as a vector
/// of type T, converting from the glTF's component type if necessary.
/// @tparam T Vector type (e.g., glm::vec2, glm::vec3, glm::vec4)
/// @param model The glTF model
/// @param accessorIndex Index of the accessor (-1 if no accessor)
/// @param expectedType Expected glTF type (TINYGLTF_TYPE_VEC3, etc.)
/// @param out Vector to populate with converted data
template <typename T>
static void ReadAccessorFloatVec(const tinygltf::Model &model,
                                 int accessorIndex,
                                 int expectedType,
                                 std::vector<T> &out)
{
    out.clear();
    if (accessorIndex < 0)
        return;

    const tinygltf::Accessor &acc = model.accessors[accessorIndex];
    const tinygltf::BufferView &view = model.bufferViews[acc.bufferView];

    if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
        throw std::runtime_error("Accessor is not float; this minimal loader expects FLOAT data.");
    if (acc.type != expectedType)
        throw std::runtime_error("Accessor type mismatch for requested attribute.");

    const int ncomp = NumComponentsInType(acc.type);
    const size_t stride = view.byteStride ? view.byteStride : (ncomp * ComponentSizeInBytes(acc.componentType));
    const unsigned char *src = GetBufferDataPtr(model, acc, view);

    out.resize(acc.count);
    for (size_t i = 0; i < acc.count; i++)
    {
        const float *f = reinterpret_cast<const float *>(src + i * stride);
        if constexpr (std::is_same_v<T, glm::vec2>)
            out[i] = glm::vec2(f[0], f[1]);
        else if constexpr (std::is_same_v<T, glm::vec3>)
            out[i] = glm::vec3(f[0], f[1], f[2]);
        else if constexpr (std::is_same_v<T, glm::vec4>)
            out[i] = glm::vec4(f[0], f[1], f[2], f[3]);
        else
            static_assert(!sizeof(T *), "Unsupported vector type");
    }
}

/// @brief Read an index buffer (uint32) from a glTF accessor.
/// Converts indices from various integer types (byte, short, int) to uint32.
/// @param model The glTF model
/// @param accessorIndex Index of the accessor (-1 if no accessor)
/// @param out Vector to populate with converted index data
static void ReadIndicesU32(const tinygltf::Model &model,
                           int accessorIndex,
                           std::vector<uint32_t> &out)
{
    out.clear();
    if (accessorIndex < 0)
        return;

    const tinygltf::Accessor &acc = model.accessors[accessorIndex];
    const tinygltf::BufferView &view = model.bufferViews[acc.bufferView];
    const unsigned char *src = GetBufferDataPtr(model, acc, view);

    const size_t stride = view.byteStride ? view.byteStride : ComponentSizeInBytes(acc.componentType);
    out.resize(acc.count);

    for (size_t i = 0; i < acc.count; i++)
    {
        const unsigned char *p = src + i * stride;
        switch (acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            out[i] = *reinterpret_cast<const uint8_t *>(p);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            out[i] = *reinterpret_cast<const uint16_t *>(p);
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            out[i] = *reinterpret_cast<const uint32_t *>(p);
            break;
        default:
            throw std::runtime_error("Unsupported index component type.");
        }
    }
}

static glm::mat4 NodeLocalMatrix(const tinygltf::Node &n)
{
    if (n.matrix.size() == 16)
    {
        glm::mat4 m(1.0f);
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                m[c][r] = static_cast<float>(n.matrix[c * 4 + r]); // column-major
        return m;
    }

    glm::vec3 t(0.0f);
    if (n.translation.size() == 3)
        t = glm::vec3((float)n.translation[0], (float)n.translation[1], (float)n.translation[2]);

    glm::quat q(1, 0, 0, 0);
    if (n.rotation.size() == 4)
        q = glm::quat((float)n.rotation[3], (float)n.rotation[0], (float)n.rotation[1], (float)n.rotation[2]);

    glm::vec3 s(1.0f);
    if (n.scale.size() == 3)
        s = glm::vec3((float)n.scale[0], (float)n.scale[1], (float)n.scale[2]);

    glm::mat4 M(1.0f);
    M = glm::translate(M, t);
    M = M * glm::toMat4(q);
    M = glm::scale(M, s);
    return M;
}

static GLint ToGLWrap(int wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return GL_REPEAT;
    default:
        return GL_REPEAT;
    }
}

static GLint ToGLFilter(int filter, bool isMin)
{
    // glTF uses the same numeric enums as OpenGL for filters.
    // If not specified, glTF defaults: mag = LINEAR, min = LINEAR_MIPMAP_LINEAR.
    if (filter == -1)
        return isMin ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    return filter;
}

static GLuint CreateTexture2DFromGLTF(const tinygltf::Model &model, int textureIndex)
{
    if (textureIndex < 0 || textureIndex >= (int)model.textures.size())
        return 0;

    const tinygltf::Texture &tex = model.textures[textureIndex];
    if (tex.source < 0 || tex.source >= (int)model.images.size())
        return 0;

    const tinygltf::Image &img = model.images[tex.source];
    if (img.image.empty() || img.width <= 0 || img.height <= 0)
        return 0;

    GLint wrapS = GL_REPEAT, wrapT = GL_REPEAT;
    GLint minF = GL_LINEAR_MIPMAP_LINEAR, magF = GL_LINEAR;

    if (tex.sampler >= 0 && tex.sampler < (int)model.samplers.size())
    {
        const tinygltf::Sampler &s = model.samplers[tex.sampler];
        wrapS = ToGLWrap(s.wrapS);
        wrapT = ToGLWrap(s.wrapT);
        minF = ToGLFilter(s.minFilter, true);
        magF = ToGLFilter(s.magFilter, false);
    }

    GLenum format = GL_RGBA;
    if (img.component == 1)
        format = GL_RED;
    else if (img.component == 2)
        format = GL_RG;
    else if (img.component == 3)
        format = GL_RGB;
    else if (img.component == 4)
        format = GL_RGBA;
    else
        return 0;

    // Treat baseColor as sRGB.
    GLint internalFormat = GL_RGBA8;
    if (format == GL_RGB)
        internalFormat = GL_SRGB8;
    if (format == GL_RGBA)
        internalFormat = GL_SRGB8_ALPHA8;

    GLuint glTex = 0;
    glGenTextures(1, &glTex);
    glBindTexture(GL_TEXTURE_2D, glTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minF);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magF);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return glTex;
}

/// @brief Create OpenGL texture objects from glTF texture data.
/// Loads and uploads all textures from the glTF model to GPU memory.
/// @param out ModelGPU structure to populate with texture handles
static void BuildTextures(const tinygltf::Model &model, ResourcesGPU &outGPU)
{
    outGPU.glTextures.clear();
    outGPU.glTextures.resize(model.textures.size(), 0);

    for (int t = 0; t < (int)model.textures.size(); t++)
        outGPU.glTextures[t] = CreateTexture2DFromGLTF(model, t);
}

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 nrm;
    glm::vec2 uv;
};

/// @brief Build GPU primitive objects from glTF mesh data.
/// Creates VAOs, VBOs, and EBOs for each glTF primitive with vertex positions, normals, UVs, and indices.
/// @param out ModelGPU structure to populate with primitive GPU resources
static void BuildPrimitives(const tinygltf::Model &model, ResourcesGPU &outGPU)
{
    outGPU.primitives.clear();
    outGPU.meshToPrimitiveIndices.clear();
    outGPU.meshToPrimitiveIndices.resize(model.meshes.size());

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;

    for (int meshIndex = 0; meshIndex < (int)model.meshes.size(); meshIndex++)
    {
        const tinygltf::Mesh &mesh = model.meshes[meshIndex];

        for (int primIndex = 0; primIndex < (int)mesh.primitives.size(); primIndex++)
        {
            const tinygltf::Primitive &prim = mesh.primitives[primIndex];
            if (prim.mode != TINYGLTF_MODE_TRIANGLES)
                continue;

            auto itPos = prim.attributes.find("POSITION");
            if (itPos == prim.attributes.end())
                continue;

            int accPos = itPos->second;
            int accNrm = -1;
            int accUv0 = -1;

            auto itNrm = prim.attributes.find("NORMAL");
            if (itNrm != prim.attributes.end())
                accNrm = itNrm->second;

            auto itUv = prim.attributes.find("TEXCOORD_0");
            if (itUv != prim.attributes.end())
                accUv0 = itUv->second;

            ReadAccessorFloatVec(model, accPos, TINYGLTF_TYPE_VEC3, positions);
            if (accNrm >= 0)
                ReadAccessorFloatVec(model, accNrm, TINYGLTF_TYPE_VEC3, normals);
            else
                normals.assign(positions.size(), glm::vec3(0, 1, 0));

            if (accUv0 >= 0)
                ReadAccessorFloatVec(model, accUv0, TINYGLTF_TYPE_VEC2, uvs);
            else
                uvs.assign(positions.size(), glm::vec2(0, 0));

            if (prim.indices >= 0)
                ReadIndicesU32(model, prim.indices, indices);
            else
            {
                indices.resize(positions.size());
                for (size_t i = 0; i < positions.size(); i++)
                    indices[i] = (uint32_t)i;
            }

            std::vector<Vertex> verts;
            verts.resize(positions.size());
            for (size_t i = 0; i < positions.size(); i++)
            {
                verts[i].pos = positions[i];
                verts[i].nrm = (i < normals.size()) ? normals[i] : glm::vec3(0, 1, 0);
                verts[i].uv = (i < uvs.size()) ? uvs[i] : glm::vec2(0, 0);
            }

            PrimitiveGPU gpu;
            gpu.meshIndex = meshIndex;
            gpu.meshPrimitiveIndex = primIndex;

            glGenVertexArrays(1, &gpu.vao);
            glBindVertexArray(gpu.vao);

            glGenBuffers(1, &gpu.vbo);
            glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &gpu.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, nrm));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));

            glBindVertexArray(0);

            gpu.indexCount = (GLsizei)indices.size();

            int newIndex = (int)outGPU.primitives.size();
            outGPU.primitives.push_back(gpu);
            outGPU.meshToPrimitiveIndices[meshIndex].push_back(newIndex);
        }
    }
}

/// @brief Load a glTF model file and initialize its GPU resources.
/// Parses a glTF file and converts its data to GPU-optimized structures including
/// vertex buffers, index buffers, textures, and materials.
/// @param out ModelGPU structure to populate with loaded model data and GPU resources
/// @param path File path to the glTF model file (.gltf or .glb)
/// @return true if the model was loaded and GPU resources initialized successfully, false otherwise
bool LoadGLTFModel(tinygltf::Model &outModel, const std::filesystem::path &path)
{
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ok = false;
    std::string ext = path.extension().string();
    if (ext == ".glb")
        ok = loader.LoadBinaryFromFile(&outModel, &err, &warn, path.string());
    else
        ok = loader.LoadASCIIFromFile(&outModel, &err, &warn, path.string());

    if (!warn.empty())
        std::cerr << "[tinygltf] warn: " << warn << "\n";
    if (!err.empty())
        std::cerr << "[tinygltf] err: " << err << "\n";
    if (!ok)
        return false;

    return true;
}

bool BuildGLTFGPUResources(const tinygltf::Model &model, ResourcesGPU &outGPU)
{
    outGPU.DestroyGL();
    BuildTextures(model, outGPU);
    BuildPrimitives(model, outGPU);
    return true;
}

bool LoadGLTFToGPU(tinygltf::Model &outModel,
                   ResourcesGPU &outGPU,
                   const std::filesystem::path &path)
{
    if (!LoadGLTFModel(outModel, path))
        return false;
    return BuildGLTFGPUResources(outModel, outGPU);
}

static AlphaMode GetAlphaMode(const tinygltf::Material &material)
{
    if (material.alphaMode == "MASK")
        return AlphaMode::Mask;
    if (material.alphaMode == "BLEND")
        return AlphaMode::Blend;
    return AlphaMode::Opaque;
}

static void DrawNodeRecursive(const tinygltf::Model &model,
                              const ResourcesGPU &gpu,
                              int nodeIndex,
                              const glm::mat4 &parentWorld,
                              const glm::mat4 &modelTransform,
                              const Shader &sh)
{
    if (nodeIndex < 0 || nodeIndex >= (int)model.nodes.size())
        return;

    const tinygltf::Node &node = model.nodes[nodeIndex];
    glm::mat4 world = parentWorld * NodeLocalMatrix(node);

    if (node.mesh >= 0 && node.mesh < (int)gpu.meshToPrimitiveIndices.size())
    {
        for (int primitiveIndex : gpu.meshToPrimitiveIndices[node.mesh])
        {
            if (primitiveIndex < 0 || primitiveIndex >= (int)gpu.primitives.size())
                continue;

            const PrimitiveGPU &prim = gpu.primitives[primitiveIndex];
            if (prim.meshIndex < 0 || prim.meshIndex >= (int)model.meshes.size())
                continue;

            const tinygltf::Mesh &mesh = model.meshes[prim.meshIndex];
            if (prim.meshPrimitiveIndex < 0 || prim.meshPrimitiveIndex >= (int)mesh.primitives.size())
                continue;

            const tinygltf::Primitive &gltfPrim = mesh.primitives[prim.meshPrimitiveIndex];

            AlphaMode alphaMode = AlphaMode::Opaque;
            float alphaCutoff = 0.5f;
            bool doubleSided = false;
            glm::vec4 baseColor(1, 1, 1, 1);
            int baseColorTextureIndex = -1;

            if (gltfPrim.material >= 0 && gltfPrim.material < (int)model.materials.size())
            {
                const tinygltf::Material &material = model.materials[gltfPrim.material];
                alphaMode = GetAlphaMode(material);
                alphaCutoff = (float)material.alphaCutoff;
                doubleSided = material.doubleSided;

                if (material.pbrMetallicRoughness.baseColorFactor.size() == 4)
                {
                    baseColor = glm::vec4(
                        (float)material.pbrMetallicRoughness.baseColorFactor[0],
                        (float)material.pbrMetallicRoughness.baseColorFactor[1],
                        (float)material.pbrMetallicRoughness.baseColorFactor[2],
                        (float)material.pbrMetallicRoughness.baseColorFactor[3]);
                }

                baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
            }

            if (doubleSided)
                glDisable(GL_CULL_FACE);
            else
            {
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
            }

            if (alphaMode == AlphaMode::Blend)
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
            }
            else
            {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            glm::mat4 M = modelTransform * world;
            glUniformMatrix4fv(sh.uModel, 1, GL_FALSE, &M[0][0]);
            glUniform1i(sh.uAlphaMode, (int)alphaMode);
            glUniform1f(sh.uAlphaCutoff, alphaCutoff);
            glUniform4fv(sh.uBaseColorFactor, 1, &baseColor[0]);

            GLuint glTex = 0;
            if (baseColorTextureIndex >= 0 && baseColorTextureIndex < (int)gpu.glTextures.size())
                glTex = gpu.glTextures[baseColorTextureIndex];

            if (glTex != 0)
            {
                glBindTexture(GL_TEXTURE_2D, glTex);
                glUniform1i(sh.uHasBaseColorTex, 1);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(sh.uHasBaseColorTex, 0);
            }

            glBindVertexArray(prim.vao);
            glDrawElements(GL_TRIANGLES, prim.indexCount, GL_UNSIGNED_INT, (void *)0);
            glBindVertexArray(0);
        }
    }

    for (int child : node.children)
        DrawNodeRecursive(model, gpu, child, world, modelTransform, sh);
}

/// @brief Render a model with all its instances.
/// Renders all instances of a model using the provided shader, view-projection matrix,
/// and camera position. Applies per-instance transforms and material properties.
/// @param model The model to render
/// @param sh Shader program to use for rendering
/// @param viewProj Combined view-projection matrix
/// @param camPos Camera position in world space (for lighting calculations)
void DrawModel(const tinygltf::Model &model,
               const ResourcesGPU &gpu,
               const Shader &sh,
               const glm::mat4 &viewProj,
               const glm::vec3 &camPos,
               float ambientStrength,
               const std::array<glm::vec3, kMaxLights> &lightPositions,
               const std::array<glm::vec3, kMaxLights> &lightColors,
               const std::array<float, kMaxLights> &lightIntensities,
               const glm::mat4 &modelTransform)
{
    sh.Use();
    glUniformMatrix4fv(sh.uViewProj, 1, GL_FALSE, &viewProj[0][0]);
    glUniform3fv(sh.uCamPos, 1, &camPos[0]);
    glUniform1f(sh.uAmbientStrength, ambientStrength);
    glUniform1i(sh.uLightCount, kMaxLights);
    glUniform3fv(sh.uLightPos, kMaxLights, &lightPositions[0].x);
    glUniform3fv(sh.uLightColor, kMaxLights, &lightColors[0].x);
    glUniform1fv(sh.uLightIntensity, kMaxLights, lightIntensities.data());

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(sh.uBaseColorTex, 0);

    int sceneIndex = model.defaultScene;
    if (sceneIndex < 0)
        sceneIndex = 0;

    if (sceneIndex >= 0 && sceneIndex < (int)model.scenes.size())
    {
        const tinygltf::Scene &scene = model.scenes[sceneIndex];
        const glm::mat4 identity(1.0f);
        for (int rootNode : scene.nodes)
            DrawNodeRecursive(model, gpu, rootNode, identity, modelTransform, sh);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}
