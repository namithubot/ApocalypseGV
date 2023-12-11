#pragma warning(disable : 5208)

// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "ocean_low.obj"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes

typedef struct
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID, shaderProgramIDUntextured;

ModelData mesh_data;
unsigned int mesh_vao = 0;
int width = 800;
int height = 600;

GLuint loc1, loc2, loc3;
GLfloat rotate_y_2 = 0.0f, rotate_y = -45.0f;
GLfloat rotate_z = -140.0f;
GLfloat rotate_x = -45.0f, camera_rotation = 0.0f;
vec3 ship1_pos = vec3(0.0f, 0.0f, -20.0f);
vec3 ship1_scale = vec3(0.003f, 0.003f, 0.003f);
vec3 ship1_uscale = vec3(1.0f, 1.0f, 1.0f);
GLfloat fovy = 45.0f;
vec3 camera_position = vec3(0.0f, 0.0f, 15.0f), target_position = vec3(0.0f, 0.0f, -20.0f);
const vec3 camera_up = vec3(0.0f, 1.0f, 0.0f);


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name,
		aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenNormals | aiProcess_OptimizeMeshes
	);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	for (unsigned int m_i = 0; m_i < scene->mNumMaterials; m_i++) {
		const aiMaterial* material = scene->mMaterials[m_i];
		std::cout << material;
	}

	aiReleaseImport(scene);
	return modelData;
}
#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(string vertexShader, string fragmentShader)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vertexShader.c_str(), GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fragmentShader.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	//glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh(const char* texture_file_name, string mesh_name = MESH_NAME) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	mesh_data = load_mesh(mesh_name.c_str());
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers (1, &vt_vbo);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glBufferData (GL_ARRAY_BUFFER, mesh_data.mTextureCoords.size() * sizeof (vec2), &mesh_data.mTextureCoords[0], GL_STATIC_DRAW);

	unsigned int vao = 0;
	glBindVertexArray(vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	int width, height, nrChannels;
	unsigned char* data = stbi_load(texture_file_name, &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc3);


	//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray (loc3);
	glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}
#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");


	// Root of the Hierarchy
//	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(fovy, (float)width / (float)height, 0.1f, 1000.0f);
	mat4 view = rotate_y_deg(look_at(camera_position, camera_position + target_position, camera_up), camera_rotation);
	// persp_proj = camera * persp_proj;

	mat4 model = identity_mat4();
	model = rotate_y_deg(model, rotate_y);
	model = rotate_z_deg(model, rotate_z);
	model = rotate_x_deg(model, rotate_x);
	view = translate(view, ship1_pos); // vec3(0.0f, 0.0f, -20.0f));
	model = scale(model, ship1_scale);

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);


	//glUseProgram(shaderProgramIDUntextured);
	//// Set up the child matrix
	//mat4 modelChild = identity_mat4();
	////modelChild = rotate_z_deg(modelChild, 180);
	//modelChild = rotate_y_deg(modelChild, rotate_y_2);
	//modelChild = translate(modelChild, vec3(ship1_pos.v[0] + 8.0f, ship1_pos.v[1], ship1_pos.v[2] + 20.0f));

	//// Apply the root matrix to the child matrix
	//modelChild = model * modelChild;

	////// Update the appropriate uniform and draw the mesh again
	//glUniformMatrix4fv(matrix_location, 1, GL_FALSE, modelChild.m);
	//glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y_2 += 20.0f * delta;
	rotate_y_2 = fmodf(rotate_y_2, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shaderProgramID = CompileShaders("simpleVertexShader.glsl", "simpleFragmentShader.glsl");
	shaderProgramIDUntextured = CompileShaders("simpleVertexShader.glsl", "unTexturedFragmentShader.glsl");


	// load mesh into a vertex buffer array
	generateObjectBufferMesh("ocean_normal.png");
	generateObjectBufferMesh("model2.jpg", "model.obj");

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	switch (key)
	{
	case 'o':
	case 'p': rotate_y += (key == 'o' ? 1 : -1) * 20.0f;
		rotate_y = fmodf(rotate_y, 360.0f); break;
	case 'k':
	case 'l': rotate_z += (key == 'k' ? 1 : -1) * 20.0f;
		rotate_z = fmodf(rotate_z, 360.0f); break;
	case 'n':
	case 'm': rotate_x += (key == 'n' ? 1 : -1) * 20.0f;
		rotate_x = fmodf(rotate_x, 360.0f); break;
	case 'w':
	case 's': ship1_pos.v[1] += (key == 'w' ? 1 : -1) * 1.0f; break;
	case 'a':
	case 'd': ship1_pos.v[0] += (key == 'a' ? 1 : -1) * 1.0f; break;
	case 'z':
	case 'e': ship1_pos.v[2] += (key == 'q' ? 1 : -1) * 1.0f; break;
	case '+': if (ship1_scale.v[1] < 9.0f && ship1_scale.v[0] < 9.0f && ship1_scale.v[2] < 9.0f)
		ship1_scale = vec3(ship1_scale.v[0] + 1.0f, ship1_scale.v[1] + 1.0f, ship1_scale.v[2] + 1.0f);
		break;
	case '<':
		if (ship1_scale.v[1] < 9.0f)
			ship1_scale = vec3(ship1_scale.v[0] + 1.0f, ship1_scale.v[1] + 1.0f, ship1_scale.v[2] + 1.0f);
		break;
	case ',':
		if (ship1_scale.v[1] < 9.0f)
			ship1_scale = vec3(ship1_scale.v[0], ship1_scale.v[1] + 1.0f, ship1_scale.v[2]);
		break;
	case '.':
		if (ship1_scale.v[1] > 1)
			ship1_scale = vec3(ship1_scale.v[0], ship1_scale.v[1] - 1.0f, ship1_scale.v[2]);
		break;
	case '-': if (ship1_scale.v[1] > 1 && ship1_scale.v[0] > 1 && ship1_scale.v[2] > 1)
		ship1_scale = vec3(ship1_scale.v[0] - 1.0f, ship1_scale.v[1] - 1.0f, ship1_scale.v[2] - 1.0f);
		break;
	case '1':
	case '3':
		camera_rotation += (key == '1' ? -1 : 1) * 20; break;
	case '8':
		camera_position += target_position;
		break;
	case '2':
		camera_position -= target_position;
		break;
	case '4':
		camera_position -= normalise(cross(target_position, camera_up));
		break;
	case '6':
		camera_position += normalise(cross(target_position, camera_up));
		break;
	default:
		break;
	}
	// Draw the next frame
	glutPostRedisplay();
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
