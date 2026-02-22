#version 330 core

uniform mat4 uViewProj;
uniform vec3 uCenter;
uniform float uPointSize;

void main()
{
    gl_Position = uViewProj * vec4(uCenter, 1.0);
    gl_PointSize = uPointSize;
}
