#version 330

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_texture;

out vec3 LightIntensity;
out vec2 TexCoord;
out vec4 eyeCoords;
out vec3 VtNormal;
out vec4 ViewPosition;

vec4 LightPosition = vec4 (-10.0, -10.0, -4.0, 1.0); // Light position in world coords.
vec3 Kd = vec3 (0.0, 0.8, 0.0); // green diffuse surface reflectance
vec3 Ld = vec3 (1.0, 1.0, 1.0); // Light source intensity

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

void main(){

  TexCoord = vertex_texture;
  //ObjectPos = vertex_position;
  VtNormal = vertex_normal;

  mat4 ModelViewMatrix = view * model;
  mat3 NormalMatrix =  mat3(ModelViewMatrix);
  // Convert normal and position to eye coords
  // Normal in view space
  vec3 tnorm = normalize( NormalMatrix * vertex_normal);
  // Position in view space
  eyeCoords = ModelViewMatrix * vec4(vertex_position,1.0);
  // Normalised vector towards the light source
  vec3 s = normalize(vec3(LightPosition - eyeCoords));
  
  // The diffuse shading equation, dot product gives us the cosine of angle between the vectors
  LightIntensity = Ld * Kd * max( dot( s, tnorm ), 0.0 );
  
  // Convert position to clip coordinates and pass along
  gl_Position = proj * view * model * vec4(vertex_position,1.0);
    
  // Extract the view position from eyeCoords (without perspective division)
  ViewPosition = eyeCoords;
}


  