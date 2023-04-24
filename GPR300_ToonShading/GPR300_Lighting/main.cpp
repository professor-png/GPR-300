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
#include "EW/EwMath.h"
#include "EW/Camera.h"
#include "EW/Mesh.h"
#include "EW/Transform.h"
#include "EW/ShapeGen.h"

#include <iostream>

GLuint createTexture(const char* filePath);
void renderObjectInScene(Shader& shader, ew::Transform& transform, ew::Mesh& mesh);
void processInput(GLFWwindow* window);
void resizeFrameBufferCallback(GLFWwindow* window, int width, int height);
void keyboardCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods);
void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void mousePosCallback(GLFWwindow* window, double xpos, double ypos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

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
const float CAMERA_MOVE_SPEED = 5.0f;
const float CAMERA_ZOOM_SPEED = 3.0f;

Camera camera((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

glm::vec3 bgColor = glm::vec3(0);
glm::vec3 materialColor = glm::vec3(1.0f);
float ambientK = 0.5;
float diffuseK = 0.5;
float specularK = 0.5;
float shininess = 250;

bool wireFrame = false;

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 color;
	float intensity = 0;
};

DirectionalLight dirLight;

const char* HATCH_1 = "Hatch01.png";
const char* HATCH_2 = "Hatch02.png";
const char* HATCH_3 = "Hatch03.png";
const char* HATCH_4 = "Hatch04.png";

int main() {
	if (!glfwInit()) {
		printf("glfw failed to init");
		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lighting", 0, 0);
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		printf("glew failed to init");
		return 1;
	}

	glfwSetFramebufferSizeCallback(window, resizeFrameBufferCallback);
	glfwSetKeyCallback(window, keyboardCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetCursorPosCallback(window, mousePosCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	//Hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Setup UI Platform/Renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	//Dark UI theme.
	ImGui::StyleColorsDark();

	//Used to draw shapes. This is the shader you will be completing.
	Shader litShader("shaders/defaultLit.vert", "shaders/defaultLit.frag");

	//Used to draw light sphere
	Shader unlitShader("shaders/defaultLit.vert", "shaders/unlit.frag");

	//outline shader
	Shader outlineShader("shaders/outline.vert", "shaders/outline.frag");

	ew::MeshData cubeMeshData;
	ew::createCube(1.0f, 1.0f, 1.0f, cubeMeshData);
	ew::MeshData sphereMeshData;
	ew::createSphere(0.5f, 64, sphereMeshData);
	ew::MeshData cylinderMeshData;
	ew::createCylinder(1.0f, 0.5f, 64, cylinderMeshData);
	ew::MeshData planeMeshData;
	ew::createPlane(1.0f, 1.0f, planeMeshData);

	ew::Mesh cubeMesh(&cubeMeshData);
	ew::Mesh sphereMesh(&sphereMeshData);
	ew::Mesh planeMesh(&planeMeshData);
	ew::Mesh cylinderMesh(&cylinderMeshData);

	//Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//initialize stencil buffer
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	//Initialize shape transforms
	ew::Transform cubeTransform;
	ew::Transform sphereTransform;
	ew::Transform planeTransform;
	ew::Transform cylinderTransform;

	cubeTransform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
	sphereTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);

	planeTransform.position = glm::vec3(0.0f, -1.0f, 0.0f);
	planeTransform.scale = glm::vec3(10.0f);

	cylinderTransform.position = glm::vec3(2.0f, 0.0f, 0.0f);

	//outline models
	ew::Transform cubeOutlineTransform;
	ew::Transform sphereOutlineTransform;
	ew::Transform planeOutlineTransform;
	ew::Transform cylinderOutlineTransform;

	cubeOutlineTransform.position = cubeTransform.position;
	cubeOutlineTransform.scale = cubeTransform.scale * 1.08f;
	sphereOutlineTransform.position = sphereTransform.position;
	sphereOutlineTransform.scale = sphereTransform.scale * 1.08f;

	planeOutlineTransform.position = planeTransform.position;
	planeOutlineTransform.scale = planeTransform.scale * 1.08f;

	cylinderOutlineTransform.position = cylinderTransform.position;
	cylinderOutlineTransform.scale = cylinderTransform.scale * 1.08f;

	dirLight.color = glm::vec3(1, 1, 1);
	dirLight.direction = glm::vec3(0, 1, 0);
	dirLight.intensity = 0.5;

	GLuint hatch1 = createTexture(HATCH_1);
	GLuint hatch2 = createTexture(HATCH_2);
	GLuint hatch3 = createTexture(HATCH_3);
	GLuint hatch4 = createTexture(HATCH_4);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hatch1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, hatch2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, hatch3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, hatch4);



	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float time = (float)glfwGetTime();
		deltaTime = time - lastFrameTime;
		lastFrameTime = time;

		//activate stencil
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);

		//Draw
		litShader.use();
		litShader.setMat4("_Projection", camera.getProjectionMatrix());
		litShader.setMat4("_View", camera.getViewMatrix());
		litShader.setVec3("_Color", materialColor);

		//Directional Light
		litShader.setVec3("_DirLight.direction", glm::normalize(dirLight.direction));
		litShader.setVec3("_DirLight.color", dirLight.color);
		litShader.setFloat("_DirLight.intensity", dirLight.intensity);

		litShader.setVec3("_CameraPos", camera.getPosition());
		litShader.setFloat("_AmbientK", ambientK);
		litShader.setFloat("_DiffuseK", diffuseK);
		litShader.setFloat("_SpecularK", specularK);
		litShader.setFloat("_Shininess", shininess);

		renderObjectInScene(litShader, cubeTransform, cubeMesh);
		renderObjectInScene(litShader, sphereTransform, sphereMesh);
		renderObjectInScene(litShader, cylinderTransform, cylinderMesh);
		renderObjectInScene(litShader, planeTransform, planeMesh);

		//deactivate stencil
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glDisable(GL_DEPTH_TEST);

		outlineShader.use();
		outlineShader.setMat4("_Projection", camera.getProjectionMatrix());
		outlineShader.setMat4("_View", camera.getViewMatrix());
		//what this do?
		/*litShader.setInt("_Hatch1", 0);
		litShader.setInt("_Hatch2", 1);
		litShader.setInt("_Hatch3", 2);
		litShader.setInt("_Hatch4", 3);*/

		renderObjectInScene(outlineShader, cubeOutlineTransform, cubeMesh);
		renderObjectInScene(outlineShader, sphereOutlineTransform, sphereMesh);
		renderObjectInScene(outlineShader, cylinderOutlineTransform, cylinderMesh);
		renderObjectInScene(outlineShader, planeOutlineTransform, planeMesh);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glEnable(GL_DEPTH_TEST);

		//Draw UI
		ImGui::Begin("Settings");
		ImGui::SliderFloat("Material Ambient K", &ambientK, 0, 1);
		ImGui::SliderFloat("Material Diffuse K", &diffuseK, 0, 1);
		ImGui::SliderFloat("Material Specular K", &specularK, 0, 1);
		ImGui::SliderFloat("Material Shininess", &shininess, 1, 500);

		if (ImGui::CollapsingHeader("Directional Light"))
		{
			ImGui::ColorEdit3("Light Color", &dirLight.color.r);
			ImGui::DragFloat3("Light Direction", &dirLight.direction.x);
			ImGui::SliderFloat("Directional Light Intensity", &dirLight.intensity, 0, 1);
		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwPollEvents();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

GLuint createTexture(const char* filePath)
{
	//texture stuff
	GLuint texture = NULL;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	//Load texture data as file
	int width, height, numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* textureData = stbi_load(filePath, &width, &height, &numComponents, 0);

	//switch statement
	switch (numComponents)
	{
	case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R, width, height, 0, GL_R, GL_UNSIGNED_BYTE, textureData);
		break;
	case 2:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, textureData);
		break;
	case 3:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
		break;
	case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		break;
	}

	//wrap horizontally
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

	//clamp vertically
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//when magnifying use nearest neighbor sampling
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//when minifying use bilinear sampling
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenerateMipmap(GL_TEXTURE_2D);

	return texture;
}


//Author: Sam Fox
void renderObjectInScene(Shader& shader, ew::Transform& transform, ew::Mesh& mesh)
{
	//Draw cube
	shader.setMat4("_Model", transform.getModelMatrix());
	mesh.draw();
}

//Author: Eric Winebrenner
void resizeFrameBufferCallback(GLFWwindow* window, int width, int height)
{
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
	camera.setAspectRatio((float)SCREEN_WIDTH / SCREEN_HEIGHT);
	glViewport(0, 0, width, height);
}
//Author: Eric Winebrenner
void keyboardCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{
	if (keycode == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	//Reset camera
	if (keycode == GLFW_KEY_R && action == GLFW_PRESS) {
		camera.setPosition(glm::vec3(0, 0, 5));
		camera.setYaw(-90.0f);
		camera.setPitch(0.0f);
		firstMouseInput = false;
	}
	if (keycode == GLFW_KEY_1 && action == GLFW_PRESS) {
		wireFrame = !wireFrame;
		glPolygonMode(GL_FRONT_AND_BACK, wireFrame ? GL_LINE : GL_FILL);
	}
}
//Author: Eric Winebrenner
void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (abs(yoffset) > 0) {
		float fov = camera.getFov() - (float)yoffset * CAMERA_ZOOM_SPEED;
		camera.setFov(fov);
	}
}
//Author: Eric Winebrenner
void mousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
		return;
	}
	if (!firstMouseInput) {
		prevMouseX = xpos;
		prevMouseY = ypos;
		firstMouseInput = true;
	}
	float yaw = camera.getYaw() + (float)(xpos - prevMouseX) * MOUSE_SENSITIVITY;
	camera.setYaw(yaw);
	float pitch = camera.getPitch() - (float)(ypos - prevMouseY) * MOUSE_SENSITIVITY;
	pitch = glm::clamp(pitch, -89.9f, 89.9f);
	camera.setPitch(pitch);
	prevMouseX = xpos;
	prevMouseY = ypos;
}
//Author: Eric Winebrenner
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//Toggle cursor lock
	if (button == MOUSE_TOGGLE_BUTTON && action == GLFW_PRESS) {
		int inputMode = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
		glfwSetInputMode(window, GLFW_CURSOR, inputMode);
		glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
	}
}

//Author: Eric Winebrenner
//Returns -1, 0, or 1 depending on keys held
float getAxis(GLFWwindow* window, int positiveKey, int negativeKey) {
	float axis = 0.0f;
	if (glfwGetKey(window, positiveKey)) {
		axis++;
	}
	if (glfwGetKey(window, negativeKey)) {
		axis--;
	}
	return axis;
}

//Author: Eric Winebrenner
//Get input every frame
void processInput(GLFWwindow* window) {

	float moveAmnt = CAMERA_MOVE_SPEED * deltaTime;

	//Get camera vectors
	glm::vec3 forward = camera.getForward();
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));

	glm::vec3 position = camera.getPosition();
	position += forward * getAxis(window, GLFW_KEY_W, GLFW_KEY_S) * moveAmnt;
	position += right * getAxis(window, GLFW_KEY_D, GLFW_KEY_A) * moveAmnt;
	position += up * getAxis(window, GLFW_KEY_Q, GLFW_KEY_E) * moveAmnt;
	camera.setPosition(position);
}
