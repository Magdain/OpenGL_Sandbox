#include <gl/glew.h>
#include <gl/GL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
using glm::vec2;
using glm::vec3;
using glm::mat4;

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <SDL.h>
#undef main

#include <vector>
#include <iostream>
#include <string>


#define GLSL(x) "#version 330\n" #x

std::string vertexSource(GLSL(
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	in vec3 position;
	in vec3 normal;

	out vec3 frag_normal;
	out vec3 frag_vertex;

	void main() {
		frag_normal = normal;
		frag_vertex = position;

		gl_Position = projection * view * model * vec4(position, 1.0); 
	}
));
std::string fragmentSource(GLSL(
	uniform mat4 model;

	in vec3 frag_normal;
	in vec3 frag_vertex;

	vec3 lightPosition = vec3(0.0, 150.0, 0.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	out vec4 Color;

	void main() {
		mat3 normalMatrix = transpose(inverse(mat3(model)));
		vec3 normal = normalize(normalMatrix * frag_normal);

		vec3 frag_position = vec3(model * vec4(frag_vertex, 1));

		vec3 surfaceToLight = lightPosition - frag_position;

		float brightness = dot(normal, surfaceToLight) / length(surfaceToLight);
		brightness = clamp(brightness, 0, 1);

		Color = brightness * vec4(lightColor, 1) * vec4(1.0, 0.0, 0.0, 1.0);
		//Color = vec4(1.0, 0.0, 0.0, 1.0);
	}
));


void ErrorCheckShader(GLuint shader) {
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		std::string log(length, ' ');
		glGetShaderInfoLog(shader, length, nullptr, &log[0]);

		std::cout << log << std::endl;
	}
}
void ErrorCheckProgram(GLuint program) {
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		std::string log(length, ' ');
		glGetProgramInfoLog(program, length, nullptr, &log[0]);

		std::cout << log << std::endl;
	}
}


struct MeshData {
	std::vector<vec3> Vertices;
	std::vector<vec3> Normals;
	std::vector<vec2> TextureCoords;
	std::vector<GLushort> Indices;
};

void LoadMeshData(const std::string& filename, MeshData& data) {
	const aiScene* scene = aiImportFile(filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
	for (unsigned int meshNumber = 0; meshNumber < scene->mNumMeshes; ++meshNumber) {
		const aiMesh* mesh = scene->mMeshes[meshNumber];
		for (unsigned int vertNumber = 0; vertNumber < mesh->mNumVertices; ++vertNumber) {
			const aiVector3D verts = mesh->mVertices[vertNumber];
			data.Vertices.push_back(vec3(verts.x, verts.y, verts.z));

			if (mesh->HasNormals()) {
				const aiVector3D normals = mesh->mNormals[vertNumber];
				data.Normals.push_back(vec3(normals.x, normals.y, normals.z));
			}

			if (mesh->HasTextureCoords(0)) {
				const aiVector3D uvs = mesh->mTextureCoords[0][vertNumber];
				data.TextureCoords.push_back(vec2(uvs.x, uvs.y));
			}
		}
		for (unsigned int faceNumber = 0; faceNumber < mesh->mNumFaces; ++faceNumber) {
			const aiFace* face = &mesh->mFaces[faceNumber];
			for (unsigned int faceIndex = 0; faceIndex < face->mNumIndices; ++faceIndex) {
				data.Indices.push_back(face->mIndices[faceIndex]);
			}
		}
	}

	// Assimp's GenUVCoords post process doesn't seem to work, but that's fine.
	// If a mesh doesn't have UV coordinates then practically we're just going
	// to apply a 1x1 white pixel texture to it, which works just fine with
	// a garbage uv coordinate.
	if (data.TextureCoords.size() == 0)
		data.TextureCoords.push_back(vec2(0.0f, 0.0f));
}


int main() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetVideoMode(800, 600, 16, SDL_OPENGL);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(false);
	SDL_Event event;


	GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK)
		std::cout << "failed glewInit" << std::endl;


	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char* vs_str = vertexSource.c_str();
	glShaderSource(vertexShader, 1, &vs_str, nullptr);
	glCompileShader(vertexShader);
	ErrorCheckShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_str = fragmentSource.c_str();
	glShaderSource(fragmentShader, 1, &fs_str, nullptr);
	glCompileShader(fragmentShader);
	ErrorCheckShader(fragmentShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glUseProgram(program);
	ErrorCheckProgram(program);


	MeshData mesh;
	LoadMeshData("Assets/Sphere.ply", mesh);


	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(mesh.Vertices[0]), &mesh.Vertices[0], GL_STATIC_DRAW);

	GLint position = glGetAttribLocation(program, "position");
	if (position == -1) std::cout << "failed to find attrib 'position'" << std::endl;
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


	GLuint nbo;
	glGenBuffers(1, &nbo);
	glBindBuffer(GL_ARRAY_BUFFER, nbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.Normals.size() * sizeof(mesh.Normals[0]), &mesh.Normals[0], GL_STATIC_DRAW);

	GLint normal = glGetAttribLocation(program, "normal");
	if (normal == -1) std::cout << "failed to find attrib 'normal'" << std::endl;
	glEnableVertexAttribArray(normal);
	glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.Indices.size() * sizeof(mesh.Indices[0]), &mesh.Indices[0], GL_STATIC_DRAW);


	vec3 camera_position(0.0f, 0.0f, -5.0f);
	vec3 camera_target(0.0f, 0.0f, 1.0f);
	vec3 camera_up(0.0f, 1.0f, 0.0f);
	vec3 negate_z(1.0f, 1.0f, -1.0f);
	glm::vec2 camera_rotation;

	mat4 model(1.0f);
	mat4 view = glm::lookAt(camera_position, camera_target, camera_up);
	mat4 projection = glm::perspective(70.0f, 800.0f/600.0f, 0.1f, 42000.0f);

	GLint u_model = glGetUniformLocation(program, "model");
	if (u_model == -1) std::cout << "failed to find uniform 'model'" << std::endl;
	glUniformMatrix4fv(u_model, 1, GL_FALSE, glm::value_ptr(model));

	GLint u_view = glGetUniformLocation(program, "view");
	if (u_view == -1) std::cout << "failed to find uniform 'view'" << std::endl;
	
	GLint u_projection = glGetUniformLocation(program, "projection");
	if (u_projection == -1) std::cout << "failed to find uniform 'projection'" << std::endl;
	glUniformMatrix4fv(u_projection, 1, GL_FALSE, glm::value_ptr(projection));


	glEnable(GL_DEPTH_TEST);


	bool running = true;
	while (running) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, mesh.Indices.size(), GL_UNSIGNED_SHORT, nullptr);
		SDL_GL_SwapBuffers();


		vec3 movement;
		vec3 norm_target = camera_target - camera_position;

		Uint8* keys = SDL_GetKeyState(nullptr);
		if (keys[SDLK_w])
			movement += norm_target;
		if (keys[SDLK_s])
			movement -= norm_target;
		if (keys[SDLK_a])
			movement += glm::cross(norm_target, camera_up);
		if (keys[SDLK_d])
			movement -= glm::cross(norm_target, camera_up);

		glm::normalize(movement);
		movement *= 6.0f * 0.02f;
		camera_position += movement;

		vec3 new_target(0.0f, 0.0f, 1.0f);
		new_target = glm::rotate(new_target, camera_rotation.x, vec3(1.0f, 0.0f, 0.0f));
		new_target = glm::rotate(new_target, camera_rotation.y, vec3(0.0f, 1.0f, 0.0f));
		camera_target = camera_position + new_target;

		view = glm::lookAt(camera_position * negate_z, camera_target * negate_z, camera_up);
		glUniformMatrix4fv(u_view, 1, GL_FALSE, glm::value_ptr(view));


		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				running = false;
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE)
					running = false;
			}
			if (event.type == SDL_MOUSEMOTION && SDL_GetAppState() & SDL_APPINPUTFOCUS) {
				if (event.motion.x != event.motion.xrel && event.motion.y != event.motion.yrel) {
					camera_rotation.y += event.motion.xrel * 0.2f;
					camera_rotation.x += event.motion.yrel * 0.2f;

					if (camera_rotation.x >= 60.0f)
						camera_rotation.x = 60.0f;
					else if (camera_rotation.x <= -60.0f)
						camera_rotation.x = -60.0f;
				}
			}
		}
	}
}