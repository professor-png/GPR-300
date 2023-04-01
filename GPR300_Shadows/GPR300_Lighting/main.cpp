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

void renderObjectInScene(Shader& shader, ew::Transform& transform, ew::Mesh& mesh);
GLuint createTexture(const char* filePath);
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

int SHADOW_MAP_WIDTH = 1080;
int SHADOW_MAP_HEIGHT = 720;

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
	glm::vec3 direction = glm::vec3(0,-1,0);
	glm::vec3 color;
	float intensity = 0;
};

DirectionalLight dirLight;
float lightIntensity = 1;

float normalMapIntensity = 1.0f;

//const char* TEXTURE = "./CorrugatedSteel007A_1K-JPG/CorrugatedSteel007A_1K_Color.jpg";
//const char* NORMAL_MAP = "./CorrugatedSteel007A_1K-JPG/CorrugatedSteel007A_1K_NormalGL.jpg";
const char* TEXTURE = "./PavingStones130_1K-JPG/PavingStones130_1K_Color.jpg";
const char* NORMAL_MAP = "./PavingStones130_1K-JPG/PavingStones130_1K_NormalGL.jpg";

bool usePost = false;
int currentEffect = 0;

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

	//framebuffer shader
	Shader framebufferShader("shaders/framebuffer.vert", "shaders/framebuffer.frag");

	//depth shader
	Shader depthShader("shaders/depthPass.vert", "shaders/depthPass.frag");

	ew::MeshData quadMeshData;
	ew::createQuad(2, 2, quadMeshData);
	ew::Mesh quadMesh(&quadMeshData);

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

	cubeTransform.position = glm::vec3(-2.0f, 0.0f, 0.0f);
	sphereTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);

	planeTransform.position = glm::vec3(0.0f, -1.0f, 0.0f);
	planeTransform.scale = glm::vec3(10.0f);

	cylinderTransform.position = glm::vec3(2.0f, 0.0f, 0.0f);

	dirLight.intensity = lightIntensity;
	dirLight.color = glm::vec3(1, 1, 1);

	//Bind our name to GL_TEXTURE_2D to make it a 2D texture
	GLuint texture = createTexture(TEXTURE);
	GLuint normalMap = createTexture(NORMAL_MAP);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMap);

	if (texture == NULL || normalMap == NULL)
		std::cout << "Failed to load texture!" << std::endl;

	// Create Frame Buffer Object
	unsigned int fbo;
	glGenFramebuffers(1, &fbo);

	// Create Framebuffer Texture
	unsigned int fbTexture;
	glGenTextures(1, &fbTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fbTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Create Framebuffer Texture
	unsigned int dbTexture;
	glGenTextures(1, &dbTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, dbTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Error checking framebuffer
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer error: " << fboStatus << std::endl;

	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, buffers);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		float time = (float)glfwGetTime();
		deltaTime = time - lastFrameTime;
		lastFrameTime = time;

		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f)/*dirLight.direction*/,
											glm::vec3(0),
											glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		depthShader.use();
		depthShader.setMat4("_LightSpaceMatrix", lightSpaceMatrix);

		renderObjectInScene(depthShader, cubeTransform, cubeMesh);
		renderObjectInScene(depthShader, sphereTransform, sphereMesh);
		renderObjectInScene(depthShader, cylinderTransform, cylinderMesh);
		renderObjectInScene(depthShader, planeTransform, planeMesh);

		litShader.use();
		litShader.setMat4("_Projection", camera.getProjectionMatrix());
		litShader.setMat4("_View", camera.getViewMatrix());
		litShader.setVec3("_Color", materialColor);
		litShader.setVec3("_ViewPos", camera.getPosition());
		litShader.setVec3("_LightPos", glm::vec3(-2.0f, 4.0f, -1.0f)/*dirLight.direction*/);
		litShader.setMat4("_LightSpaceMatrix", lightSpaceMatrix);
		litShader.setInt("_DiffuseTexture", 0);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, dbTexture);
		litShader.setInt("_ShadowMap", 3);

		dirLight.intensity = lightIntensity;
		litShader.setVec3("_Light.color", dirLight.color);
		litShader.setVec3("_Light.direction", glm::normalize(dirLight.direction));
		litShader.setFloat("_Light.intensity", dirLight.intensity);

		litShader.setVec3("_CameraPos", camera.getPosition());
		litShader.setFloat("_AmbientK", ambientK);
		litShader.setFloat("_DiffuseK", diffuseK);
		litShader.setFloat("_SpecularK", specularK);
		litShader.setFloat("_Shininess", shininess);
		litShader.setFloat("_NormalIntensity", normalMapIntensity);

		litShader.setFloat("_Time", time);
		litShader.setInt("_Texture", 0);
		litShader.setInt("_NormalMap", 1);

		renderObjectInScene(litShader, cubeTransform, cubeMesh);
		renderObjectInScene(litShader, sphereTransform, sphereMesh);
		renderObjectInScene(litShader, cylinderTransform, cylinderMesh);
		renderObjectInScene(litShader, planeTransform, planeMesh);

		// Bind the default framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST); // prevents framebuffer rectangle from being discarded
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw the framebuffer rectangle
		framebufferShader.use();
		framebufferShader.setInt("_ApplyEffect", (int)usePost);
		framebufferShader.setInt("_CurrentEffect", currentEffect);
		framebufferShader.setFloat("_ScreenWidth", SCREEN_WIDTH);
		framebufferShader.setFloat("_ScreenHeight", SCREEN_HEIGHT);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, fbTexture);
		framebufferShader.setInt("_ScreenTexture", 2);
		quadMesh.draw();

		//Draw UI
		ImGui::Begin("Settings");
		ImGui::SliderFloat("Material Ambient K", &ambientK, 0, 1);
		ImGui::SliderFloat("Material Diffuse K", &diffuseK, 0, 1);
		ImGui::SliderFloat("Material Specular K", &specularK, 0, 1);
		ImGui::SliderFloat("Material Shininess", &shininess, 1, 500);

		ImGui::SliderFloat("Directional Light Intensity", &dirLight.intensity, 0, 1);
		ImGui::DragFloat3("Light Direction", &dirLight.direction.x);
		ImGui::DragFloat3("Color", &dirLight.color.x);

		ImGui::SliderFloat("Normal Map Intensity", &normalMapIntensity, 0, 1);

		ImGui::Checkbox("Apply Post Processing?", &usePost);
		ImGui::SliderInt("Post Processing Effect", &currentEffect, 0, 3);
		std::string effectName;

		switch (currentEffect)
		{
		case 0:
			effectName = "Grey Scale";
			break;
		case 1:
			effectName = "Edge Detection";
			break;
		case 2:
			effectName = "Inverse";
			break;
		case 3:
			effectName = "Deep Fried Like";
			break;
		}

		ImGui::Text(effectName.c_str());

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwPollEvents();

		glfwSwapBuffers(window);
	}

	glDeleteTextures(1, &texture);
	glDeleteFramebuffers(1, &fbo);

	glfwTerminate();
	return 0;
}

//Author: Sam Fox
void renderObjectInScene(Shader& shader, ew::Transform& transform, ew::Mesh& mesh)
{
	//Draw cube
	shader.setMat4("_Model", transform.getModelMatrix());
	mesh.draw();
}

//Author: Sam Fox
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

	if (textureData == NULL)
		return NULL;

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
