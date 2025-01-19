//fragment.glsl
#version 330 core

in vec4 out_Color;   
in vec3 FragPos;
in vec3 Normal;
in vec3 TexCoord;

out vec4 FragColor;   

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D outTexture;

void main(void)
{
    vec3 ambientLight = vec3(0.7f, 0.7f, 0.7f);
    vec3 ambient = ambientLight * lightColor;

    vec3 NV = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseLight = max(dot(NV, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightColor;

    int shininess = 128;
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, NV);
    float specularLight = max(dot(viewDir, reflectDir), 0.0);
    specularLight = pow(specularLight, shininess);
    vec3 specular = specularLight * lightColor;

    vec3 lighting = ambient + diffuse + specular;

    vec2 texCoord = vec2(TexCoord.x, TexCoord.y);

    vec4 texColor = texture(outTexture, texCoord);

    vec4 result = texColor * vec4(lighting, 1.0) * out_Color;

    FragColor = result;
}
