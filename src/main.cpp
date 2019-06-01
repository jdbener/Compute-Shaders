/*
	Thanks https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2
*/
#include "ComputeShader.h"
#include "ComputeBuffer.h"

#include <iomanip>

using namespace std;

GLFWwindow* initOpenGL();

int main(){
	// Create the OpenGL Context
	initOpenGL();
	cout << "Initaliztion Sucessful!" << endl;

	// Create a connection to the shader file
	ifstream multiplyFile("src/multiplyShader.glsl");
	ifstream reverseFile("src/reverseShader.glsl");

	// Initalize the data we will pass to the GPU
	vector<unsigned int> data;
	for(int i = 1; i <= 1024; i++)
		data.push_back(i);

	{
		// Create Buffers
		ComputeBuffer inBuffer(1, data);
		//ComputeBuffer outBuffer(2, data.size() * sizeof(data[0]));
		ComputeBuffer outBuffer(2);
		outBuffer.addField<int>(data.size());
		outBuffer.commit();

		// Multiply our data
		ComputeShader multiply(multiplyFile);
		multiply.dispatch(1024 / 32);

		// Reverse our data
		ComputeShader reverse(reverseFile);
		reverse.setParameter("shouldFlip", false);
		reverse.dispatch(1024 / 32);

		// Pull the data from the outbuffer
		outBuffer.getData(data);

		cout << (reverse.getParameter<bool>("shouldFlip") ? "true" : "false") << endl;
	}

	// Print out the data
	cout << endl << "Data:";
	for(int i = 0; i < 30; i++){
		if(i % 10 == 0)	cout << endl;
		cout << left << setw(6) << setprecision(2) << data[i];
	}

	cout << endl;

	glfwTerminate();
	multiplyFile.close();
	reverseFile.close();
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
	if (!found) assert(0 && "Extension \"GL_ARB_compute_shader\" not found");
}

GLFWwindow* initOpenGL(){
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
	checkExtension();

	return window;
}
