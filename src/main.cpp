/*
	Thanks https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2
*/
#include "ComputeShader.h"

#include <iomanip>
#include <vector>

using namespace std;

error(LibraryInitalizationError, "Intializing GLFW!")
error(CreatingWindowError, "Creating Window!")
error(LoadingOpenGLError, "Loading OpenGL!")
error(ExtensionNotFoundError, "Extension \"GL_ARB_compute_shader\" not found")

GLFWwindow* initOpenGL();

int main(){
	// Create the OpenGL Context
	initOpenGL();
	cout << "Initaliztion Sucessful!" << endl;

	ifstream shaderFile("src/shader.glsl");

	// Initalize the data we will pass to the GPU
	vector<unsigned int> data;
	for(int i = 1; i <= 1024; i++)
		data.push_back(i);

	{
		ComputeShader multiply(shaderFile, data.size() * sizeof(data[0]), &data[0]);
		multiply.dispatch(1024);
		multiply.pull(&data[0]);
	}

	// Print out the data
	cout << endl << "Data:";
	for(int i = 0; i < 30; i++){
		if(i % 10 == 0)	cout << endl;
		cout << left << setw(6) << setprecision(2) << data[i];
	}

	cout << endl;

	glfwTerminate();
	cout << endl << "Terminatation Sucessful!" << endl;
	return 0;
}

void checkExtension(){
	// Determine the number of extensions
	int count;
	glGetIntegerv(GL_NUM_EXTENSIONS, &count);

	// Determine if compute shaders are supported
	bool found = false;
	for (int i = 0; i < count; ++i)
		if (string((char*)glGetStringi(GL_EXTENSIONS, i)).find("GL_ARB_compute_shader") != string::npos) {
			//cout << "Extension \"GL_ARB_compute_shader\" found" << endl;
			found = true;
			break;
		}

	// Error computer shaders are not supported
	if (!found)
		throw ExtensionNotFoundError(__FILE__, __LINE__);
}

GLFWwindow* initOpenGL(){
	GLFWwindow* window;

	// Initialize the library
	if (!glfwInit())
		throw LibraryInitalizationError(__FILE__, __LINE__);

	// Create a windowed mode window and its OpenGL context
	const double WIDTH = 960, HEIGHT = 540;
	window = glfwCreateWindow(WIDTH, HEIGHT, "Compute Shader", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		throw CreatingWindowError(__FILE__, __LINE__);
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Syncronize frame rate with monitor refresh rate
	glfwSwapInterval(1);

	// Load in the modern OpenGL functions
	if (glewInit() != GLEW_OK)
		throw LoadingOpenGLError(__FILE__, __LINE__);

	// Ensure that compute shaders are supported
	checkExtension();

	return window;
}
