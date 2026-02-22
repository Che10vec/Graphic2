#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uViewProj;

out vec3 vWorldPos;
out vec3 vWorldNrm;
out vec2 vUv;

void main()
{
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;

    mat3 nrmM = mat3(uModel);
    vWorldNrm = normalize(nrmM * aNrm);

    vUv = aUv;
    gl_Position = uViewProj * wp;
}
