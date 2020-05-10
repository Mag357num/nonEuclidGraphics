#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition;
uniform mat4 V;
uniform mat4 P;
uniform mat4 M;

void main(){
    
    vec3 world_pos = (M * vec4(vertexPosition, 1.0f)).xyz;
    gl_Position = P * V * vec4(world_pos, 1.0f);
}
