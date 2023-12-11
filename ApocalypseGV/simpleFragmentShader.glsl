#version 330

in vec3 LightIntensity;
in vec2 TexCoord;
in vec4 eyeCoords;
in vec3 VtNormal;
in vec4 ViewPosition;
uniform sampler2D texture1;

vec4 LightPosition = vec4(-10.0, -10.0, -4.0, 1.0);
vec3 light_color = vec3(0.0, 0.0, 1.0);

void main(){
	float ambientStrength = 0.3; // Adjust the ambient strength
    vec3 ambient = ambientStrength * light_color;

	float specularStrength = 0.4;
    vec4 viewDir = normalize(ViewPosition - eyeCoords);
    vec3 lightDir = normalize(LightPosition - eyeCoords).xyz;
    vec3 norm = normalize(VtNormal);
    vec4 reflectDir = vec4(reflect(-lightDir, norm), 1.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 50); // Adjust the shininess
    vec3 specular = specularStrength * spec * light_color;
	//gl_FragColor = vec4 (LightIntensity, 1.0);
	gl_FragColor = texture(texture1, TexCoord); //* normalize(vec4((LightIntensity + ambient + specular), 1.0));
}