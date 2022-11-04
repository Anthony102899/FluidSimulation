#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow *window);
bool initFluidState(const char* imagePath);
bool updateFluidState(const char* imagePath, int offset);
//! global variables
float tau = 0.58;
int winWidth = 0, winHeight = 0;
double pos_x = 0.0, pos_y = 0.0;
double pos_x2 = 0.0, pos_y2 = 0.0;
int sign = 1;
int offset = 0;
//! those data will be used in shaders
unsigned int lbmBuffer[3];
//!	lbmBoundary stores boundary
unsigned int lbmBoundary;
int input = 0;


int main()
{
	//! glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//! glfw window creation
	GLFWwindow* window = glfwCreateWindow(800, 600, "CSCI5390 Course Project", NULL, NULL);
	if (window == NULL)
	{
		cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	//! set vsync, make sure the simulation won't go too fast
	glfwSwapInterval(1);
	//! glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//! ---------build and compile our shader program---------
	//! vertex shader and fragment shader
	Shader lbmProgram("./vertex.vert", "./lbm.frag");
	Shader renderProgram("./vertex.vert", "./render.frag");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		 //! positions       //! texture coordinates
		 1.0f,  1.0f, 0.0f,  1.0f, 1.0f, //! top right
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f, //! bottom right
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f, //! bottom left
		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f  //! top left 
	};
	unsigned int indices[] = {
		0, 1, 3, //! first triangle
		1, 2, 3  //! second triangle
	};
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//! vertex attribute: position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//! vertex attribute: texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//! load and create textures
	// -------------------------
	const char *image_path = "./mask.jpg";
	if (!initFluidState(image_path))
	{
		cout << "Error: state initialization failed!" << endl;
		return -1;
	}
	//! tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	//! set uniform variables for lbm.frag
	lbmProgram.use(); //! don't forget to activate/use the shader before setting uniforms!
	glUniform1i(glGetUniformLocation(lbmProgram.ID, "boundary_texture"), 0);
	glUniform1i(glGetUniformLocation(lbmProgram.ID, "state_texture1"), 1);
	glUniform1i(glGetUniformLocation(lbmProgram.ID, "state_texture2"), 2);
	glUniform1i(glGetUniformLocation(lbmProgram.ID, "mouse_active"), input);
	glUniform1i(glGetUniformLocation(lbmProgram.ID, "state_texture3"), 3);
	glUniform2f(glGetUniformLocation(lbmProgram.ID, "image_size"), winWidth, winHeight);
	glUniform1f(glGetUniformLocation(lbmProgram.ID, "tau"), tau);
	glUniform2f(glGetUniformLocation(lbmProgram.ID, "mouse_pos"), pos_x, pos_y);
	glUniform2f(glGetUniformLocation(lbmProgram.ID, "mouse_input"), pos_x2, pos_y2);
	//! set uniform variables for render.frag
	renderProgram.use(); // don't forget to activate/use the shader before setting uniforms!
	glUniform1i(glGetUniformLocation(renderProgram.ID, "boundary_texture"), 0);
	glUniform1i(glGetUniformLocation(renderProgram.ID, "state_texture3"), 1);

	//! create Frame buffer Object
	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	//! link GL_COLOR_ATTACHMENT0-2 to lbmBuffer[3]
	for (int i = 0; i < 3; i++)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, lbmBuffer[i], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//! render loop
	// ------------
	while (!glfwWindowShouldClose(window))
	{
		//! input
		processInput(window);
		offset += sign; 
		updateFluidState(image_path, offset/2);
		//! LBM iterative computation
		// --------------------------
		//! bind to frame_buffer and draw scene as we normally would to color texture 
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		lbmProgram.use();
		glUniform1i(glGetUniformLocation(lbmProgram.ID, "mouse_active"), input);
		glUniform2f(glGetUniformLocation(lbmProgram.ID, "mouse_pos"), pos_x, pos_y);
		glUniform2f(glGetUniformLocation(lbmProgram.ID, "mouse_input"), pos_x2, pos_y2);
		glBindVertexArray(VAO);
		//! bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, lbmBoundary);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lbmBuffer[0]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, lbmBuffer[1]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, lbmBuffer[2]);
		//! updating the textures statues by writing to buffers
		GLenum buffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
		glDrawBuffers(3, buffers);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		input = 0;
		//! render to screen
		// -----------------
		//! now bind back to default frame buffer and draw a quad plane with the attached frame_buffer color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(199.0/255, 237.0/255, 204.0/255, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
		glClear(GL_COLOR_BUFFER_BIT);
		renderProgram.use();
		glBindVertexArray(VAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, lbmBoundary);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, lbmBuffer[2]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//! glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//! optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteFramebuffers(1, &FBO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	//! glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}


bool updateFluidState(const char* imagePath, int offset) {
	//! load image
	int nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* maskData = stbi_load(imagePath, &winWidth, &winHeight, &nrChannels, 0);
	float* boundaryData = new float[winWidth * winHeight * 3];
	if (boundaryData == NULL) {
		cout << "Unable to allocate memory!" << endl;
		return false;
	}
	int maxIndex = (winHeight - 1) * winWidth + winWidth - 1;
	//!	Fill _boundaryData_ with image data from _boundaryBitmap_
	for (int y = 0; y < winHeight; y++)
	{
		for (int x = 0; x < winWidth; x++)
		{
			int index = y * winWidth + x;
			//! Pixels near image margin are set to be boundary 
			if ((x < 2) || (x > (winWidth - 3)) || (y < 2) || (y > (winHeight - 3)))
			{
				boundaryData[3 * index + 0] = 0.0f;
				boundaryData[3 * index + 1] = 0.0f;
				boundaryData[3 * index + 2] = 0.0f;
			}
			else
			{
				if (index + offset < maxIndex && index + offset > 0) {
					unsigned char r = maskData[3 * (index + offset) + 0];
					unsigned char g = maskData[3 * (index + offset) + 1];
					unsigned char b = maskData[3 * (index + offset) + 2];
					boundaryData[3 * index + 0] = r / 255.0;
					boundaryData[3 * index + 1] = g / 255.0;
					boundaryData[3 * index + 2] = b / 255.0;
				}
				else {
					sign = 0;
					unsigned char r = maskData[3 * (index + offset) + 0];
					unsigned char g = maskData[3 * (index + offset) + 1];
					unsigned char b = maskData[3 * (index + offset) + 2];
					boundaryData[3 * index + 0] = r / 255.0;
					boundaryData[3 * index + 1] = g / 255.0;
					boundaryData[3 * index + 2] = b / 255.0;
				}
			}
		}
	}
	stbi_image_free(maskData);
	//!	generate OpenGL texture buffer for Boundary data
	glBindTexture(GL_TEXTURE_2D, lbmBoundary);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, boundaryData);
	
}

bool initFluidState(const char* imagePath)
{
	//! load image
	int nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *maskData = stbi_load(imagePath, &winWidth, &winHeight, &nrChannels, 0);
	cout << "texture image (HxW):" << winHeight << " x " << winWidth << endl;
	float *boundaryData = new float[winWidth * winHeight * 3];
	if (boundaryData == NULL) {
		cout << "Unable to allocate memory!" << endl;
		return false;
	}
	//!	Fill _boundaryData_ with image data from _boundaryBitmap_
	for (int y = 0; y < winHeight; y++)
	{
		for (int x = 0; x < winWidth; x++)
		{
			int index = y*winWidth + x;
			//! Pixels near image margin are set to be boundary 
			if ((x < 2) || (x > (winWidth - 3)) || (y < 2) || (y > (winHeight - 3)))
			{
				boundaryData[3 * index + 0] = 0.0f;
				boundaryData[3 * index + 1] = 0.0f;
				boundaryData[3 * index + 2] = 0.0f;
			}
			else
			{
				//! pixels: 0.0 or 1.0
				unsigned char r = maskData[3 * index + 0];
				unsigned char g = maskData[3 * index + 1];
				unsigned char b = maskData[3 * index + 2];
				boundaryData[3 * index + 0] = r / 255.0;
				boundaryData[3 * index + 1] = g / 255.0;
				boundaryData[3 * index + 2] = b / 255.0;
			}
		}
	}
	stbi_image_free(maskData);
	//!	generate OpenGL texture buffer for Boundary data
	glGenTextures(1, &lbmBoundary);
	glBindTexture(GL_TEXTURE_2D, lbmBoundary);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, boundaryData);

	//! initialize the data buffers for LBM simulation use
	float *lbmData[3];
	for (int i = 0; i < 3; i++)
	{
		lbmData[i] = new float[winWidth * winHeight * 4];
		if (lbmData[i] == NULL)
			return false;
	}

	//! initial state of the lbmBuffer[] for simulation
	//!	macroscopic velocity: <ux,uy>
	//!	macroscopic density:  rho
	//! initialize distribution function
	float w[9] = { 4.0f / 9.0f,1.0f / 9.0f,1.0f / 9.0f,1.0f / 9.0f,1.0f / 9.0f,1.0f / 36.0f,1.0f / 36.0f,1.0f / 36.0f,1.0f / 36.0f };
	float e[9][2] = { { 0,0 },{ 1,0 },{ 0,1 },{ -1,0 },{ 0,-1 },{ 1,1 },{ -1,1 },{ -1,-1 },{ 1,-1 } };
	//!	initialize values of f0-f8, rho, ux, uy for each pixel
	for (int y = 0; y < winHeight; y++)
	{
		for (int x = 0; x < winWidth; x++)
		{
			float ux = 0.3;
			float uy = 0.06;
			float rho = 1.0;
			float uu_dot = (ux*ux + uy*uy);
			float f[9];
			for (int i = 0; i < 9; i++)
			{
				float eu_dot = (e[i][0] * ux + e[i][1] * uy);
				f[i] = w[i] * rho * (1.0f + 3.0f * eu_dot + 4.5f * eu_dot * eu_dot - 1.5f * uu_dot);
			}
			int index = y*winWidth + x;
			//! f1~f4
			lbmData[0][4 * index + 0] = f[1];
			lbmData[0][4 * index + 1] = f[2];
			lbmData[0][4 * index + 2] = f[3];
			lbmData[0][4 * index + 3] = f[4];
			//! f5~f8
			lbmData[1][4 * index + 0] = f[5];
			lbmData[1][4 * index + 1] = f[6];
			lbmData[1][4 * index + 2] = f[7];
			lbmData[1][4 * index + 3] = f[8];
			//! f0, rho, and (ux,uy)
			lbmData[2][4 * index + 0] = f[0];
			lbmData[2][4 * index + 1] = rho;
			lbmData[2][4 * index + 2] = ux;
			lbmData[2][4 * index + 3] = uy;
		}
	}

	//! generate OpenGL texture buffer for lbmBuffer[3]
	for (int i = 0; i < 3; i++)
	{
		glGenTextures(1, &lbmBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, lbmBuffer[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//! set GL_RGBA as internal format for data storage
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, lbmData[i]);
	}
	delete[]boundaryData;
	delete[]lbmData[0];
	delete[]lbmData[1];
	delete[]lbmData[2];
	return true;
}


void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	//! make sure the viewport matches the new window dimensions; note that width and 
	//! height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
		glfwGetCursorPos(window, &pos_x, &pos_y);
	}
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
		glfwGetCursorPos(window, &pos_x2, &pos_y2);
		pos_x2 = pos_x2 - pos_x;
		pos_y2 = pos_y2 - pos_y;
		input = 1;
	}
}