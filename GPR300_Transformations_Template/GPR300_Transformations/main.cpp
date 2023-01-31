#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "EW/Shader.h"
#include "EW/ShapeGen.h"

void resizeFrameBufferCallback(GLFWwindow* window, int width, int height);
void keyboardCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods);

float lastFrameTime;
float deltaTime;

int SCREEN_WIDTH = 1080;
int SCREEN_HEIGHT = 720;

double prevMouseX;
double prevMouseY;
bool firstMouseInput = false;

/* Button to lock / unlock mouse
* 1 = right, 2 = middle
* Mouse will start locked. Unlock it to use UI
* */
const int MOUSE_TOGGLE_BUTTON = 1;
const float MOUSE_SENSITIVITY = 0.1f;

glm::vec3 bgColor = glm::vec3(0);
float exampleSliderFloat = 0.0f;

namespace sf
{
	glm::mat4 translate(const glm::vec3& p)
	{
		glm::mat4 m = glm::mat4(1); //identity
		m[3][0] = p.x;
		m[3][1] = p.y;
		m[3][2] = p.z;

		return m;
	}
}

/*
* (cos(rotation.y) * cos(rotation.z), sin)
*/
struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation; //euler angles
	glm::vec3 scale;

	glm::mat4 getModelMatrix()
	{
		glm::mat4 t = glm::mat4(1);
		t[3][0] = position.x;
		t[3][1] = position.y;
		t[3][2] = position.z;

		glm::mat4 s = glm::mat4(1);
		s[0][0] = scale.x;
		s[1][1] = scale.y;
		s[2][2] = scale.z;

		glm::mat4 rX = glm::mat4(
			1, 0, 0, 0,
			0, cos(rotation.x), -sin(rotation.x), 0,
			0, sin(rotation.x), cos(rotation.x), 0,
			0, 0, 0, 1);

		glm::mat4 rY = glm::mat4(
			cos(rotation.y), 0, sin(rotation.y), 0,
			0, 1, 0, 0,
			-sin(rotation.y), 0, cos(rotation.y), 0,
			0, 0, 0, 1);

		glm::mat4 rZ = glm::mat4(
			cos(rotation.z), -sin(rotation.z), 0, 0,
			sin(rotation.z), cos(rotation.z), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);

		glm::mat4 r = rZ * rY * rX;

		return t * s * r;
	}
};

struct Camera
{
	glm::vec3 position;
	glm::vec3 target = glm::vec3(0);
	float fov = 60;
	float aspectR;
	float orthographicSize = 10;
	bool orthographic = false;

	glm::mat4 getViewMatrix()
	{
		glm::vec3 up = glm::vec3(0, 1, 0);
		
		glm::mat4 view = glm::mat4(1);

		glm::vec3 f = glm::normalize(target - position);
		glm::vec3 r = glm::normalize(glm::cross(f, up));
		glm::vec3 u = glm::normalize(glm::cross(r, f));

		f = -f;

		view[0][0] = r.x;
		view[0][1] = r.y;
		view[0][2] = r.z;

		view[1][0] = u.x;
		view[1][1] = u.y;
		view[1][2] = u.z;

		view[2][0] = f.x;
		view[2][1] = f.y;
		view[2][2] = f.z;

		view = glm::transpose(view);

		glm::mat4 translation = glm::mat4(1);
		translation[3][0] = -position.x;
		translation[3][1] = -position.y;
		translation[3][2] = -position.z;

		//return glm::inverse(view * translation);
		return view * translation;
	}

	glm::mat4 getProjectionMatrix()
	{
		glm::mat4 p;
		
		float r = orthographicSize * aspectR, l = -r;
		float t = orthographicSize, b = -t;
		float n = 0.01, f = 100;

		p = glm::mat4(1);
		
		if (orthographic)
		{
			p[0][0] = 2 / (r - l);
			p[1][1] = 2 / (t - b);
			p[2][2] = -2 / (f - n);

			p[3][0] = -(r + l) / (r - l);
			p[3][1] = -(t + b) / (t - b);
			p[3][2] = -(f + n) / (f - n);
		}
		else
		{
			float a = aspectR, c = aspectR * tan(glm::radians(fov) / 2);

			p[0][0] = 1 / (a * c);
			p[1][1] = 1 / c;
			p[2][2] = -((f + n) / (f - n));
			p[2][3] = -1;

			p[3][2] = -((2 * (f * n)) / (f - n));
		}

		return p;
	}
};

const int NUM_CUBES = 6;
Transform transform[NUM_CUBES];
Camera camera;

int main()
{
	if (!glfwInit())
	{
		printf("glfw failed to init");
		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Transformations", 0, 0);
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
	{
		printf("glew failed to init");
		return 1;
	}

	glfwSetFramebufferSizeCallback(window, resizeFrameBufferCallback);
	glfwSetKeyCallback(window, keyboardCallback);

	// Setup UI Platform/Renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	//Dark UI theme.
	ImGui::StyleColorsDark();

	Shader shader("shaders/vertexShader.vert", "shaders/fragmentShader.frag");

	MeshData cubeMeshData;
	createCube(1.0f, 1.0f, 1.0f, cubeMeshData);

	Mesh cubeMesh(&cubeMeshData);

	//Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	float panRadius = 10, panSpeed = 1;

	// cube sizes
	transform[0].scale = glm::vec3(1);
	transform[1].scale = glm::vec3(2);
	transform[2].scale = glm::vec3(0.5);
	transform[3].scale = glm::vec3(3);
	transform[4].scale = glm::vec3(1);
	transform[5].scale = glm::vec3(1, 2, 0.5);

	// cube positions
	transform[0].position = glm::vec3(1, 0, 1);
	transform[1].position = glm::vec3(5, 1, -4);
	transform[2].position = glm::vec3(3, -3, 0);
	transform[3].position = glm::vec3(0, 3, 1);
	transform[4].position = glm::vec3(2, -1, 2);
	transform[5].position = glm::vec3(0);

	// cube rotations
	transform[0].rotation = glm::vec3(1, 0, 1);
	transform[1].rotation = glm::vec3(5, 1, -4);
	transform[2].rotation = glm::vec3(3, -3, 0);
	transform[3].rotation = glm::vec3(0, 3, 1);
	transform[4].rotation = glm::vec3(2, -1, 2);
	transform[5].rotation = glm::vec3(1, -.5, .3);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float time = (float)glfwGetTime();
		deltaTime = time - lastFrameTime;
		lastFrameTime = time;

		camera.aspectR = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

		camera.position.x = panRadius * sin(time * panSpeed);
		camera.position.z = panRadius * cos(time * panSpeed);

		//Draw
		shader.use();

		//glm::mat4 modelMatrix = glm::mat4(1); //identity matrix
		for (size_t i = 0; i < NUM_CUBES; i++)
		{
			if (i < NUM_CUBES / 2)
				transform[i].rotation.x += sin(time) * 0.1f;
			else
				transform[i].rotation.z += cos(time) * 0.2f;

			shader.setMat4("_Model", transform[i].getModelMatrix());
			shader.setMat4("_View", camera.getViewMatrix());
			shader.setMat4("_Perspective", camera.getProjectionMatrix());

			cubeMesh.draw();
		}

		//Draw UI
		ImGui::Begin("Settings");
		ImGui::SliderFloat("Pan Radius", &panRadius, 5.0f, 20.0f);
		ImGui::SliderFloat("Pan Speed", &panSpeed, 0.1f, 4.0f);
		ImGui::SliderFloat("Fov", &camera.fov, 10.0f, 180.0f);
		ImGui::SliderFloat("Orthographic Height", &camera.orthographicSize, 1.0f, 30.0f);
		ImGui::Checkbox("Orthographic", &camera.orthographic);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwPollEvents();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void resizeFrameBufferCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
}

void keyboardCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{
	if (keycode == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}