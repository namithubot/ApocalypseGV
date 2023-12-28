#version 330

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_texture;

out vec3 LightIntensity;
out vec2 TexCoord;
out vec4 eyeCoords;
out vec3 VtNormal;
out vec4 ViewPosition;
out vec3 ambientStrength;
out vec3 specularStrength;
out float reflective_index;
out float attenuation;

vec4 LightPosition = vec4 (0.0, -50.0, -4.0, 1.0); // Light position in world coords.
vec3 LightPosition2 = vec3 (8.0, -10.0, 4.0); // Light position in world coords.
uniform vec3 Kd = vec3 (0.0, 0.6, 0.0); // green diffuse surface reflectance
vec3 Ld = vec3 (1.0, 1.0, 1.0); // Light source intensity
vec3 Ld2 = vec3(1.0, 0.7, 0.3);

uniform bool isWave = false;
uniform float deltaTime;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;
uniform vec3 Ka;
uniform vec3 Ks;
uniform float specular_exponent = 50.0f;

const float w = 1.2f;
const float amplitude = 0.6f;

vec3  wavePosition = vec3(20,0,20);

float CalculateHeight(float x, float z, vec2 direction)
{
    // Using the wave equationn now
    float kx = 1.60f * dot(vec2(x, z), direction);
    return amplitude * cos(kx - w * deltaTime) + (1 - amplitude) * cos(kx + w * deltaTime);
}

void main(){

  TexCoord = vertex_texture;
  vec3 vertexPos = vertex_position;
  if (isWave)
  {
    vec3 direction = wavePosition-vertex_position;
    vertexPos.x  += CalculateHeight(vertex_position.y,vertex_position.z,direction.yz);
    vertexPos.y  += CalculateHeight(vertex_position.z,vertex_position.x,direction.xz);
    vertexPos.z  += CalculateHeight(vertex_position.x,vertex_position.y,direction.xy);
  }
  //ObjectPos = vertex_position;
  VtNormal = vertex_normal;

  mat4 ModelViewMatrix = view * model;
  mat3 NormalMatrix =  mat3(ModelViewMatrix);
  // Convert normal and position to eye coords
  // Normal in view space
  vec3 tnorm = normalize( NormalMatrix * vertex_normal);
  // Position in view space
  eyeCoords = ModelViewMatrix * vec4(vertexPos,1.0);
  // Normalised vector towards the light source
  vec3 s = normalize(vec3(LightPosition - eyeCoords));
  
  float distance = length(LightPosition2 - vertexPos);
  attenuation = 1.0 / (1 + 0.1 * distance + 
  			     0.032 * (distance * distance));
  
  // The diffuse shading equation, dot product gives us the cosine of angle between the vectors
  LightIntensity = (Ld + attenuation * Ld2) * Kd * max( dot( s, tnorm ), 0.0 );

  // Convert position to clip coordinates and pass along
  gl_Position = proj * view * model * vec4(vertexPos,1.0);

    
  // Extract the view position from eyeCoords (without perspective division)
  ViewPosition = eyeCoords;

  ambientStrength = Ka;

  reflective_index = specular_exponent;

  if (isWave) {
    reflective_index = reflective_index * 2;
  }
}
  