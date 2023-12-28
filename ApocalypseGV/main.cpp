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
#include <assimp/Importer.hpp>

// Project includes
#include "maths_funcs.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"



/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define MESH_NAME "boat.dae"
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct
{
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
	unsigned int materialIndex;
} MeshData;

typedef struct
{
	vec3 mAmbient;
	vec3 mDiffuse;
	vec3 mSpec;
	GLfloat mReflectiveIndex;
	std::string mTextureFiles;
	bool hasTexture;

} MaterialData;

typedef struct
{
	std::vector<MeshData> mMeshData;
	std::vector<MaterialData> mMaterialData;
	unsigned int firstBuffer;
} ModelData;

#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID, shaderProgramIDUntextured;
#define MAX_NUM_BUFFER 100
unsigned int vp_vbo[MAX_NUM_BUFFER], vn_vbo[MAX_NUM_BUFFER], vt_vbo[MAX_NUM_BUFFER], vao[MAX_NUM_BUFFER];
unsigned int current_buffer_size = 0;
unsigned char* imag_data[MAX_NUM_BUFFER];
GLuint textures[MAX_NUM_BUFFER];
float time_diff = 0;

ModelData mesh_data[3];
unsigned int mesh_vao = 0;
int width = 800;
int height = 600;
int boat_direction = 1;
bool is_moving = true;
float rotate_y_2 = 0.0f;

GLuint loc1, loc2, loc3;
GLfloat rotate_x_2 = 0.0f, rotate_y = 0.0f;
GLfloat rotate_water_y = 0.0f, rotate_water_z = 0.0f, rotate_water_x = 180.0f;
GLfloat rotate_z = 0.0f;
GLfloat rotate_x = 0.0f, camera_rotation_x = 0.0f, camera_rotation_y = 0.0f;
vec3 ship1_pos = vec3(1.0f, -4.0f, -20.0f);
vec3 ship1_scale = vec3(2.0f, 2.0f, 2.0f);
vec3 boat_scale = vec3(0.03f, 0.03f, 0.03f);
vec3 water_scale = vec3(0.8f, 0.25f, 0.25f);
vec3 boat_pos_loc = vec3(-5.0f, 3.0f, 0.0f);
vec3 bird_scale = vec3(0.3f, 0.3f, 0.3f);
vec3 boat_location = vec3(35.0f, 10.0f, 2.0f);
GLfloat fovy = 45.0f;
vec3 camera_position = vec3(0.0f, 7.0f, 5.0f), target_position = vec3(0.0f, 0.0f, -5.0f);
const vec3 camera_up = vec3(0.0f, 1.0f, 0.0f);


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	std::cout << file_name;
	ModelData modelData;
	Assimp::Importer importer;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = importer.ReadFile(
		file_name,
		aiProcess_Triangulate
		| aiProcess_PreTransformVertices
		| aiProcess_GenSmoothNormals
		| aiProcess_OptimizeMeshes
		| aiProcess_FlipUVs
	);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		cout<<"Detailed error: "<<importer.GetErrorString()<< endl;;
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		cout << "Bone: " << mesh->mNumBones << std::endl;
		MeshData mesh_data;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				mesh_data.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				mesh_data.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				mesh_data.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}

			mesh_data.materialIndex = mesh->mMaterialIndex;
		}

		modelData.mMeshData.push_back(mesh_data);
	}

	for (unsigned int m_i = 0; m_i < scene->mNumMaterials; m_i++) {
		const aiMaterial* material = scene->mMaterials[m_i];
		MaterialData material_data;

		aiColor3D ambient, diffuse, specular;

		material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
		material_data.mAmbient = vec3(ambient.r, ambient.g, ambient.b);

		material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
		material_data.mSpec = vec3(specular.r, specular.g, specular.b);


		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		material_data.mDiffuse = vec3(diffuse.r, diffuse.g, diffuse.b);

		float reflective_index;
		material->Get(AI_MATKEY_SHININESS, reflective_index);
		material_data.mReflectiveIndex = reflective_index;

		aiString path;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL);
		printf(path.C_Str());
		std::string file(path.C_Str());
		material_data.mTextureFiles = file;
		material_data.hasTexture = !file.empty();

		modelData.mMaterialData.push_back(material_data);
	}

	importer.FreeScene();
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
void generateObjectBufferMesh(GLuint shaderProgramID, const char* texture_file_name, string mesh_name = MESH_NAME, string model_dir = "") {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	string file_location = model_dir != "" ? model_dir + "\\" + mesh_name : mesh_name;

	int idx = 0;
	if (mesh_name.find("sea") != std::string::npos)
		idx = 1;
	if (mesh_name.find("plane") != std::string::npos)
		idx = 2;

	mesh_data[idx] = mesh_data[idx].mMeshData.size() == 0 ? load_mesh(file_location.c_str()) : mesh_data[idx];
	mesh_data[idx].firstBuffer = current_buffer_size;
	vp_vbo[idx] = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	for (MeshData mesh : mesh_data[idx].mMeshData)
	{
		glGenBuffers(1, &vp_vbo[current_buffer_size]);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[current_buffer_size]);
		glBufferData(GL_ARRAY_BUFFER, mesh.mVertices.size() * sizeof(vec3), &mesh.mVertices[0], GL_STATIC_DRAW);
		vn_vbo[current_buffer_size] = 0;
		glGenBuffers(1, &vn_vbo[current_buffer_size]);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[current_buffer_size]);
		glBufferData(GL_ARRAY_BUFFER, mesh.mNormals.size() * sizeof(vec3), &mesh.mNormals[0], GL_STATIC_DRAW);

		//	This is for texture coordinates which you don't currently need, so I have commented it out
		vt_vbo[current_buffer_size] = 0;
		glGenBuffers(1, &vt_vbo[current_buffer_size]);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[current_buffer_size]);
		glBufferData(GL_ARRAY_BUFFER, mesh.mTextureCoords.size() * sizeof(vec2), &mesh.mTextureCoords[0], GL_STATIC_DRAW);

		vao[current_buffer_size] = 0;
		glBindVertexArray(vao[current_buffer_size]);

		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[current_buffer_size]);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[current_buffer_size]);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glGenTextures(1, &textures[current_buffer_size]);
		glBindTexture(GL_TEXTURE_2D, textures[current_buffer_size]);
		// set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load and generate the texture
		int width, height, nrChannels;
		string texture_file_location = model_dir != "" ? model_dir + "\\" + texture_file_name : texture_file_name;
		if (mesh_data[idx].mMaterialData[mesh.materialIndex].hasTexture)
		{
			texture_file_location = model_dir != ""
				? model_dir + "\\" + mesh_data[idx].mMaterialData[mesh.materialIndex].mTextureFiles.c_str()
				: mesh_data[idx].mMaterialData[mesh.materialIndex].mTextureFiles.c_str();
		}
		std::cout << texture_file_location << std::endl;
		// stbi_set_flip_vertically_on_load(false);
		imag_data[current_buffer_size] = stbi_load(texture_file_location.c_str(), &width, &height, &nrChannels, 0);
		if (imag_data[current_buffer_size])
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imag_data[current_buffer_size]);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture " << texture_file_location << std::endl;
		}
		stbi_image_free(imag_data[current_buffer_size]);

		glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(loc3);


		//	This is for texture coordinates which you don't currently need, so I have commented it out
		glEnableVertexAttribArray(loc3);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[current_buffer_size]);
		glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		current_buffer_size++;

	}
}
#pragma endregion VBO_FUNCTIONS

#pragma region LOAD_BUFFER

void bindBuffer(unsigned int id, GLuint shaderProgramID)
{
	glUniform1f(glGetUniformLocation(shaderProgramID, "id"), 1.0f);
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	int num_meshes = mesh_data[id].mMeshData.size();

	int location = mesh_data[id].firstBuffer;
	/*for (unsigned int i = 0; i < id; i++)
	{
		location += mesh_data[i].mMeshData.size();
	}*/

	for (int i = 0; i < num_meshes; i++)
	{
		size_t mat_index = mesh_data[id].mMeshData[i].materialIndex;
		if (mesh_data[id].mMaterialData[mat_index].hasTexture)
		{
			glUniform1f(glGetUniformLocation(shaderProgramID, "id"), 0.0f);
		}


		glUniform3fv(glGetUniformLocation(shaderProgramID, "Ks"), 1,
			glm::value_ptr(
				glm::vec3(mesh_data[id].mMaterialData[mat_index].mSpec.v[0],
					mesh_data[id].mMaterialData[mat_index].mSpec.v[1],
					mesh_data[id].mMaterialData[mat_index].mSpec.v[2])));
		glUniform3fv(glGetUniformLocation(shaderProgramID, "Kd"), 1,
			glm::value_ptr(
				glm::vec3(mesh_data[id].mMaterialData[mat_index].mDiffuse.v[0],
					mesh_data[id].mMaterialData[mat_index].mDiffuse.v[1],
					mesh_data[id].mMaterialData[mat_index].mDiffuse.v[2])));
		glUniform3fv(glGetUniformLocation(shaderProgramID, "Ka"), 1,
			glm::value_ptr(
				glm::vec3(mesh_data[id].mMaterialData[mat_index].mAmbient.v[0],
					mesh_data[id].mMaterialData[mat_index].mAmbient.v[1],
					mesh_data[id].mMaterialData[mat_index].mAmbient.v[2])));
		glUniform1f(glGetUniformLocation(shaderProgramID, "specular_exponent"), mesh_data[id].mMaterialData[mat_index].mReflectiveIndex);

		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[location + i]);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[id].mMeshData[i].mVertices.size() * sizeof(vec3), &mesh_data[id].mMeshData[i].mVertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[location + i]);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[id].mMeshData[i].mNormals.size() * sizeof(vec3), &mesh_data[id].mMeshData[i].mNormals[0], GL_STATIC_DRAW);



		glEnableVertexAttribArray(loc1);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo[location + i]);
		glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glEnableVertexAttribArray(loc2);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo[location + i]);
		glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);


		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[location + i]);
		glBufferData(GL_ARRAY_BUFFER, mesh_data[id].mMeshData[i].mTextureCoords.size() * sizeof(vec2), &mesh_data[id].mMeshData[i].mTextureCoords[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(loc3);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo[location + i]);
		glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindTexture(GL_TEXTURE_2D, textures[location + i]);

		glDrawArrays(GL_TRIANGLES, 0, mesh_data[id].mMeshData[i].mVertices.size());

	}
}

#pragma endregion LOAD_BUFFER


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.52f, 0.81f, 0.98f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);


	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	// Root of the Hierarchy
	// mat4 view = identity_mat4();
	mat4 persp_proj = perspective(fovy, (float)width / (float)height, 0.1f, 1000.0f);
	// persp_proj = camera * persp_proj;

	mat4 model = identity_mat4();
	model = rotate_y_deg(model, rotate_y);
	model = rotate_z_deg(model, rotate_z);
	model = rotate_x_deg(model, rotate_x);
	mat4 view = translate(view, ship1_pos); // vec3(0.0f, 0.0f, -20.0f));
	model = scale(model, ship1_scale);

	view = look_at(camera_position, camera_position + target_position, camera_up);
	view = rotate_y_deg(view, camera_rotation_y);
	view = rotate_x_deg(view, camera_rotation_x);

	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	bindBuffer(0, shaderProgramID);

	mat4 boatModel = identity_mat4();
	//boatModel = model * boatModel;
	boatModel = scale(boatModel, boat_scale);
	for (int i = 0; i < 10; i++)
	{

		boatModel = translate(boatModel,
			vec3(boat_location.v[0] + i, boat_location.v[1], boat_location.v[2]));

		// Apply the root matrix to the child matrix
		//modelChild = model * modelChild;
		boatModel = rotate_y_deg(boatModel, -1.0f * rotate_y_2);

		//// Update the appropriate uniform and draw the mesh again
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, boatModel.m);
		bindBuffer(2, shaderProgramID);

	}

	glUseProgram(shaderProgramIDUntextured);
	matrix_location = glGetUniformLocation(shaderProgramIDUntextured, "model");
	view_mat_location = glGetUniformLocation(shaderProgramIDUntextured, "view");
	proj_mat_location = glGetUniformLocation(shaderProgramIDUntextured, "proj");
	//// Set up the child matrix
	mat4 modelChild = identity_mat4();
	modelChild = translate(modelChild, boat_pos_loc);

	// Apply the root matrix to the child matrix
	//modelChild = model * modelChild;
	modelChild = scale(modelChild, water_scale);
	modelChild = rotate_x_deg(modelChild, rotate_water_x);

	//// Update the appropriate uniform and draw the mesh again
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, modelChild.m);
	glUniform1i(glGetUniformLocation(shaderProgramIDUntextured, "isWave"), true);
	glUniform1f(glGetUniformLocation(shaderProgramIDUntextured, "deltaTime"), time_diff);
	bindBuffer(1, shaderProgramIDUntextured);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	//// Rotate the model slowly around the y axis at 20 degrees per second
	if (is_moving)
	{
		rotate_y_2 += 20.0f * delta;
		rotate_y_2 = fmodf(rotate_y_2, 360.0f);
	}
	//boat_pos_loc.v[1] += glm::sin(10.0f * delta);
	// rotate_water_x = fmodf(rotate_water_x, 360.0f);
	//boat_pos_loc.v[1] = fmodf(boat_pos_loc.v[1], 3.0f);
	time_diff += delta;

	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shaderProgramID = CompileShaders("simpleVertexShader.glsl", "simpleFragmentShader.glsl");
	generateObjectBufferMesh(shaderProgramID, "ocean_normal.png", "castle_blended.obj", "castle");
	generateObjectBufferMesh(shaderProgramID, "Beriev_2048.png", "plane.obj", "Beriev_A50");

	shaderProgramIDUntextured = CompileShaders("simpleVertexShader.glsl", "simpleFragmentShader.glsl");
	std::cout << shaderProgramIDUntextured << std::endl;


	// load mesh into a vertex buffer array
	generateObjectBufferMesh(shaderProgramIDUntextured, "ocean_normal.png", "sea_tr.obj");

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	switch (key)
	{
	case 'o':
	case 'p': 
		rotate_y += (key == 'o' ? 1 : -1) * 20.0f;
		rotate_y = fmodf(rotate_y, 360.0f);
		break;
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
	case 'e': ship1_pos.v[2] += (key == 'z' ? 1 : -1) * 1.0f; break;
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
		camera_rotation_x += (key == '1' ? -1 : 1) * 10; break;
	case '7':
	case '9':
		camera_rotation_y += (key == '7' ? -1 : 1) * 10; break;
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
	case 't':
		is_moving = !is_moving;
		break;
	case 'y':
		boat_direction *= -1;
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
