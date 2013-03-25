#include <gl/glew.h>
#include <gl/GL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
using glm::vec3;
using glm::vec4;
using glm::mat4;

#include <SDL.h>
#undef main

#include <iostream>
#include <string>
#include <array>


#define GLSL(x) "#version 330\n" #x

std::string vertexSource(GLSL(
	in vec3 position;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0); 
	}
));
std::string fragmentSource(GLSL(
	out vec4 Color;
	void main() {
		Color = vec4(1.0, 0.0, 0.0, 1.0);
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

class Plane {
public:
	Plane() {

	}
	~Plane() {

	}

	void Normalize() {
		float length = glm::length(vec3(Points));
		Points /= length;
	}

	float Dot(const vec3& right) {
		return	Points.x * right.x +
				Points.y * right.y +
				Points.z * right.z +
				Points.w;
	}

	vec4 Points;
};

float PlaneDot(const vec4& plane, const vec3& position) {
	return	plane.x * position.x +
			plane.y * position.y +
			plane.z * position.z +
			plane.w;
}
bool CubeInFrustum(vec4 planes[6], const vec3& position, const vec3& scale) {
	float x = position.x;
	float y = position.y;
	float z = position.z;
	float size = 1.0f;

	float min_x = (x - size) * scale.x;
	float min_y = (y - size) * scale.y;
	float min_z = (z - size) * scale.z;

	float max_x = (x + size) * scale.x;
	float max_y = (y + size) * scale.y;
	float max_z = (z + size) * scale.z;

	for (int i = 0; i < 6; ++i) {
		if (PlaneDot(planes[i], vec3(min_x, min_y, min_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(max_x, min_y, min_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(min_x, max_y, min_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(max_x, max_y, min_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(min_x, min_y, max_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(max_x, min_y, max_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(min_x, max_y, max_z)) >= 0.0f)
			continue;
		if (PlaneDot(planes[i], vec3(max_x, max_y, max_z)) >= 0.0f)
			continue;
		return false;
	}
	return true;
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


	vec3 vertices[] = {
		// The camera's Z value is negated when constructing our view matrix so that our
		// world space is left handed, however our model vertices are still right handed.
		vec3(-1.0f,  1.0f, -1.0f), // 0
		vec3( 1.0f,  1.0f, -1.0f), // 1
		vec3( 1.0f, -1.0f, -1.0f), // 2
		vec3(-1.0f, -1.0f, -1.0f), // 3

		vec3(-1.0f,  1.0f, 1.0f), // 4
		vec3( 1.0f,  1.0f, 1.0f), // 5
		vec3( 1.0f, -1.0f, 1.0f), // 6
		vec3(-1.0f, -1.0f, 1.0f), // 7
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	GLint position = glGetAttribLocation(program, "position");
	if (position == -1) std::cout << "failed to find attrib 'position'" << std::endl;
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


	GLushort indices[] = {
		0, 1, 2, // back
		2, 3, 0,
		4, 5, 6, // front
		6, 7, 4,
		0, 4, 7, // left
		7, 3, 0,
		5, 1, 2, // right
		2, 6, 5,
		0, 4, 5, // top
		5, 1, 0,
		3, 7, 6, // bottom
		6, 2, 3,
	};
	GLuint ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);


	vec3 camera_position(0.0f, 0.0f, -5.0f);
	vec3 camera_target(0.0f, 0.0f, 1.0f);
	vec3 camera_up(0.0f, 1.0f, 0.0f);
	vec3 negate_z(1.0f, 1.0f, -1.0f);
	glm::vec2 camera_rotation;

	vec3 model_position(0.0f, 0.0f, 0.0f);
	vec3 model_scale(10.0f, 10.0f, 10.0f);

	mat4 model = glm::translate(mat4(1.0f), model_position);
	model = glm::scale(model, model_scale);
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
		mat4 mvp = projection * view;

		vec4 planes[6];
		//*
		// right
		planes[0].x = mvp[0][3] - mvp[0][2];
		planes[0].y = mvp[1][3] - mvp[1][2];
		planes[0].z = mvp[2][3] - mvp[2][2];
		planes[0].w = mvp[3][3] - mvp[3][2];

		// left
		planes[1].x = mvp[0][3] + mvp[0][2];
		planes[1].y = mvp[1][3] + mvp[1][2];
		planes[1].z = mvp[2][3] + mvp[2][2];
		planes[1].w = mvp[3][3] + mvp[3][2];

		// bottom
		planes[2].x = mvp[0][3] - mvp[0][0];
		planes[2].y = mvp[1][3] - mvp[1][0];
		planes[2].z = mvp[2][3] - mvp[2][0];
		planes[2].w = mvp[3][3] - mvp[3][0];

		// top
		planes[3].x = mvp[0][3] + mvp[0][0];
		planes[3].y = mvp[1][3] + mvp[1][0];
		planes[3].z = mvp[2][3] + mvp[2][0];
		planes[3].w = mvp[3][3] + mvp[3][0];

		// far
		planes[4].x = mvp[0][3] + mvp[0][1];
		planes[4].y = mvp[1][3] + mvp[1][1];
		planes[4].z = mvp[2][3] + mvp[2][1];
		planes[4].w = mvp[3][3] + mvp[3][1];

		// near
		planes[5].x = mvp[0][3] - mvp[0][1];
		planes[5].y = mvp[1][3] - mvp[1][1];
		planes[5].z = mvp[2][3] - mvp[2][1];
		planes[5].w = mvp[3][3] - mvp[3][1];

		for (int i = 0; i < 6; ++i) {
			float length = glm::length(vec3(planes[i]));
			planes[i] /= length;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (CubeInFrustum(planes, model_position, model_scale)) {
			glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_SHORT, nullptr);
			//std::cout << "draw cube" << std::endl;
		} else {
			//std::cout << "cull cube" << std::endl;
		}
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
			if (event.type == SDL_MOUSEMOTION) {
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