#version 330 core
in vec3 vWorldPos;
in vec3 vWorldNrm;
in vec2 vUv;

uniform vec3 uCamPos;
uniform vec4 uBaseColorFactor;
uniform int  uHasBaseColorTex;
uniform sampler2D uBaseColorTex;
uniform int   uAlphaMode;
uniform float uAlphaCutoff;
uniform float uAmbientStrength;
uniform int uLightCount;
uniform vec3 uLightPos[3];
uniform vec3 uLightColor[3];
uniform float uLightIntensity[3];

out vec4 FragColor;

void main()
{
    vec3 albedo = uBaseColorFactor.rgb;
    float alpha = uBaseColorFactor.a;

    if (uHasBaseColorTex == 1)
    {
        vec4 texc = texture(uBaseColorTex, vUv);
        albedo *= texc.rgb;
        alpha *= texc.a;
    }

    vec3 N = normalize(vWorldNrm);
    vec3 V = normalize(uCamPos - vWorldPos);

    vec3 ambient = albedo * uAmbientStrength;
    vec3 color = ambient;

    int lightCount = clamp(uLightCount, 0, 3);
    for (int i = 0; i < lightCount; i++)
    {
        vec3 Lvec = uLightPos[i] - vWorldPos;
        float dist = max(length(Lvec), 0.0001);
        vec3 L = Lvec / dist;

        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        vec3 lightRadiance = uLightColor[i] * uLightIntensity[i] * attenuation;

        float ndotl = max(dot(N, L), 0.0);
        vec3 diffuse = albedo * ndotl * lightRadiance;

        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.15;
        vec3 specular = vec3(spec) * lightRadiance;

        color += diffuse + specular;
    }
    if (uAlphaMode == 0) {
        alpha = 1.0;
    } else if (uAlphaMode == 1) {
        if (alpha < uAlphaCutoff) discard;
        alpha = 1.0;
    }
    FragColor = vec4(color, alpha);
}
