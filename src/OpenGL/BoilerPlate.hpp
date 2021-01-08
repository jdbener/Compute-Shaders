#ifndef __BOILER_PLATE_H__
#define __BOILER_PLATE_H__

#include "ComputeShader.hpp"

static void glCheckExtension(){
	// Determine the number of extensions
	int count;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);

	// Determine if compute shaders are supported
	bool found = false;
	for (int i = 0; i < count; ++i)
		if (std::string((char*)glGetStringi(GL_EXTENSIONS, i)).find("GL_ARB_compute_shader") != std::string::npos) {
			//cout << "Extension \"GL_ARB_compute_shader\" found" << endl;
			found = true;
			break;
		}

	// Error computer shaders are not supported
	if (!found) assert(0 && "Extension \"GL_ARB_compute_shader\" not found");
}

static GLFWwindow* initOpenGL(){
	GLFWwindow* window;

	// Initialize the library
	if (!glfwInit()) assert(0 && "Error: Intializing GLFW!");

	// Create a windowed mode window and its OpenGL context
	const double WIDTH = 960, HEIGHT = 540;
	window = glfwCreateWindow(WIDTH, HEIGHT, "Compute Shader", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		assert(0 && "Error: Creating Window!");
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Syncronize frame rate with monitor refresh rate
	glfwSwapInterval(1);

	// Load in the modern OpenGL functions
	if (glewInit() != GLEW_OK) assert(0 && "Error: Loading OpenGL!");

	// Ensure that compute shaders are supported
	glCheckExtension();

	return window;
}

#endif /* end of include guard: __BOILER_PLATE_H__ */
