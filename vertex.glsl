//vertex.glsl
#version 330

layout(location = 0) in vec3 in_Position; 
layout(location = 1) in vec4 in_Color; 
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec3 vTexCoord;	

out vec4 out_Color; 
out vec3 FragPos;
out vec3 Normal;
out vec3 TexCoord;

uniform mat4 model; 
uniform mat4 view; 
uniform mat4 projection; 

void main(void)
{
	gl_Position = projection * view * model * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);
	FragPos = vec3(model*vec4(in_Position, 1.0));
	Normal = vec3(model*vec4(vNormal, 0.0));
	out_Color = in_Color;
	TexCoord = vTexCoord;
}