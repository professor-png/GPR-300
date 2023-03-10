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
int numPointLights = 0;

bool wireFrame = false;

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 color;
	float intensity = 0;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec3 color;
	float intensity;
	float range;
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 color;
	float intensity;
	float radius;
	float innerAngle;
	float outerAngle;
};

DirectionalLight dirLight;
SpotLight spotLight;
PointLight pointLights[3];
float pointLightIntensity = 0.5;
float range = 10;
float orbit = 3;

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

	//Initialize shape transforms
	ew::Transform cubeTransform;
	ew::Transform sphereTransform;
	ew::Transform planeTransform;
	ew::Transform cylinderTransform;
	ew::Transform lightTransform[3];

	cubeTransform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
	sphereTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);

	planeTransform.position = glm::vec3(0.0f, -1.0f, 0.0f);
	planeTransform.scale = glm::vec3(10.0f);

	cylinderTransform.position = glm::vec3(2.0f, 0.0f, 0.0f);

	lightTransform[0].scale = glm::vec3(0.5f);
	lightTransform[0].position = glm::vec3(0.0f, 5.0f, 0.0f);
	lightTransform[1].scale = glm::vec3(0.5f);
	lightTransform[1].position = glm::vec3(0.0f, 5.0f, 0.0f);
	lightTransform[2].scale = glm::vec3(0.5f);
	lightTransform[2].position = glm::vec3(0.0f, 5.0f, 0.0f);

	dirLight.color = glm::vec3(1, 1, 1);
	dirLight.direction = glm::vec3(0, 1, 0);
	dirLight.intensity = 0.5;

	pointLights[0].position = glm::vec3(0, 5, 0);
	pointLights[1].position = glm::vec3(0, 5, 0);
	pointLights[2].position = glm::vec3(0, 5, 0);

	pointLights[0].intensity = pointLightIntensity;
	pointLights[1].intensity = pointLightIntensity;
	pointLights[2].intensity = pointLightIntensity;

	pointLights[0].color = glm::vec3(1, 0, 0);
	pointLights[1].color = glm::vec3(0, 1, 0);
	pointLights[2].color = glm::vec3(0, 0, 1);

	pointLights[0].range = range;
	pointLights[1].range = range;
	pointLights[2].range = range;

	spotLight.position = glm::vec3(0, 5, 0);
	spotLight.color = glm::vec3(1);
	spotLight.intensity = 1;
	spotLight.direction = glm::vec3(0, 1, 0);
	spotLight.innerAngle = 1.0;
	spotLight.outerAngle = 10.0;

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float time = (float)glfwGetTime();
		deltaTime = time - lastFrameTime;
		lastFrameTime = time;

		//Draw
		litShader.use();
		litShader.setMat4("_Projection", camera.getProjectionMatrix());
		litShader.setMat4("_View", camera.getViewMatrix());
		litShader.setVec3("_Color", materialColor);

		//Directional Light
		litShader.setVec3("_DirLight.direction", glm::normalize(dirLight.direction));
		litShader.setVec3("_DirLight.color", dirLight.color);
		litShader.setFloat("_DirLight.intensity", dirLight.intensity);

		//Point Lights
		for (int i = 0; i < numPointLights; i++)
		{
			pointLights[i].intensity = pointLightIntensity;
			pointLights[i].range = range;
			litShader.setVec3("_PointLights[" + std::to_string(i) + "].position", pointLights[i].position);
			litShader.setVec3("_PointLights[" + std::to_string(i) + "].color", pointLights[i].color);
			litShader.setFloat("_PointLights[" + std::to_string(i) + "].intensity", pointLights[i].intensity);
			litShader.setFloat("_PointLights[" + std::to_string(i) + "].range", pointLights[i].range);
		}
		litShader.setInt("numPointLights", numPointLights);

		pointLights[0].position.x = sin(time) * orbit;
		pointLights[0].position.z = cos(time) * orbit;

		pointLights[1].position.x = -sin(time) * orbit;
		pointLights[1].position.z = -cos(time) * orbit;

		pointLights[2].position.x = cos(time) * orbit;
		pointLights[2].position.z = cos(time) * orbit;

		lightTransform[0].position = pointLights[0].position;
		lightTransform[1].position = pointLights[1].position;
		lightTransform[2].position = pointLights[2].position;

		//spot light
		litShader.setVec3("_SpotLight.position", spotLight.position);
		litShader.setVec3("_SpotLight.direction", spotLight.direction);
		litShader.setVec3("_SpotLight.color", spotLight.color);
		litShader.setFloat("_SpotLight.intensity", spotLight.intensity);
		litShader.setFloat("_SpotLight.radius", spotLight.radius);
		litShader.setFloat("_SpotLight.innerAngle", cos(glm::radians(spotLight.innerAngle)));
		litShader.setFloat("_SpotLight.outerAngle", cos(glm::radians(spotLight.outerAngle)));

		litShader.setVec3("_CameraPos", camera.getPosition());
		litShader.setFloat("_AmbientK", ambientK);
		litShader.setFloat("_DiffuseK", diffuseK);
		litShader.setFloat("_SpecularK", specularK);
		litShader.setFloat("_Shininess", shininess);

		//Draw cube
		litShader.setMat4("_Model", cubeTransform.getModelMatrix());
		cubeMesh.draw();

		//Draw sphere
		litShader.setMat4("_Model", sphereTransform.getModelMatrix());
		sphereMesh.draw();

		//Draw cylinder
		litShader.setMat4("_Model", cylinderTransform.getModelMatrix());
		cylinderMesh.draw();

		//Draw plane
		litShader.setMat4("_Model", planeTransform.getModelMatrix());
		planeMesh.draw();

		//Draw light as a small sphere using unlit shader, ironically.
		for (int i = 0; i < numPointLights; i++)
		{
			unlitShader.use();
			unlitShader.setMat4("_Projection", camera.getProjectionMatrix());
			unlitShader.setMat4("_View", camera.getViewMatrix());
			unlitShader.setMat4("_Model", lightTransform[i].getModelMatrix());
			unlitShader.setVec3("_Color", pointLights[i].color);
			sphereMesh.draw();
		}

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

		ImGui::SliderInt("Number of Point Lights", &numPointLights, 0, 3);

		if (ImGui::CollapsingHeader("Point Light"))
		{
			ImGui::SliderFloat("Point Light Intensity", &pointLightIntensity, 0, 1);
			ImGui::SliderFloat("Orbit Range", &orbit, 1, 5);
			ImGui::SliderFloat("Range", &range, 0.1, 10);
		}

		if (ImGui::CollapsingHeader("Spot Light"))
		{
			ImGui::DragFloat3("Spot Light Direction", &spotLight.direction.x);
			ImGui::DragFloat3("Spot Light Position", &spotLight.position.x);
			ImGui::SliderFloat("Spot Light Intensity", &spotLight.intensity, 0, 1);
			ImGui::SliderFloat("Inner Angle", &spotLight.innerAngle, 1, 360);
			ImGui::SliderFloat("Outer Angle", &spotLight.outerAngle, 1, 360);
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
