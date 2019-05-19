/*
	Thanks to http://wili.cc/blog/opengl-cs.html and https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2
*/
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>

using namespace std;

// Macro which creates an exception structure
#define error(_name_, _message_) \
	struct _name_ { _name_(string file, int line) { \
	cerr << file << " line #" << line << " error: " << _message_ << endl; }};

error(GenericError, "OpenGL Error!")
error(LibraryInitalizationError, "Intializing GLFW!")
error(CreatingWindowError, "Creating Window!")
error(LoadingOpenGLError, "Loading OpenGL!")
error(ProgramCreationError, "Creating Program!")
error(ExtensionNotFoundError, "Extension \"GL_ARB_compute_shader\" not found")

GLFWwindow* initOpenGL();
unsigned int createProgram(const char* source);

int main()
{
	// Create the OpenGL Context
	initOpenGL();
	cout << "Initaliztion Sucessful!" << endl;

	// In order to write to a texture, we have to introduce it as image2D.
	// local_size_x/y/z layout variables define the work group size.
	// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
	// gl_LocalInvocationID is the local index within the work group, and
	// gl_WorkGroupID is the work group's index
	const char csSrc[] =
		"#version 430\n\n"
		"layout (local_size_x = 1024) in;\n"
		"layout(std430, binding = 7) buffer bufferLayout\n"
		"{ uint data[]; };\n\n"
		"void main() {\n"
		"	data[gl_GlobalInvocationID.x] = data[gl_GlobalInvocationID.x] * 5;\n"
		"}";

	// Create the program
	unsigned int program = createProgram(csSrc);
	// The exception wound up here because it causing weird behavior inside of createProgram
	if(!program) throw ProgramCreationError(__FILE__, __LINE__);
	cout << "Program Created! id: " << program << endl;

	// Initalize the data we will pass to the GPU
	vector<unsigned int> data;
	for(int i = 1; i <= 1024; i++)
		data.push_back(i);


	const unsigned int SIZE = data.size() * sizeof(data[0]);
	// Create the storage buffer
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, SIZE, &data[0], GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	cout << "Buffers created!" << endl;

	// Compute call
	glUseProgram(program);
	glDispatchCompute(1024, 1, 1);

	cout << "Compute Call Preformed!" << endl;

	// Copy the storage buffer into our data buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	GLvoid* p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, SIZE, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
	memcpy(&data[0], p, SIZE);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

	// Print out the data
	cout << endl << "Data:";
	for(int i = 0; i < 1024; i++){
		if(i % 10 == 0)	cout << endl;
		cout << left << setw(5) << data[i];
	}

	// Print out the shader program
	cout << endl << endl << "Shader:" << endl << csSrc;

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

	cout << (char*) glGetString(GL_VERSION) << endl;

	return window;
}

unsigned int createProgram(const char* source) {
	// Creating the compute shader, and the program object containing the shader
    unsigned int program = glCreateProgram();
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
	int status;

	// Compile the shader
    glShaderSource(shader, 2, &source, NULL);
	glCompileShader(shader);
	// Ensure the shader compiled sucessfully
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	// If it didn't compile sucessfully...
    if (!status) {
		// Determine how long the error message is
		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		// Allocate enouph memory to hold the error message
        char* log = (char*) alloca(length * sizeof(char));
		// Display the error message
        glGetShaderInfoLog(shader, length, &length, log);
        cerr << "Compiler log:" << endl << log << endl;
		return 0;
    }

	// Link the shader
	glAttachShader(program, shader);
    glLinkProgram(program);
	// Ensure the shader linked sucessfully
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
		// Determine how long the error message is
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

		// Allocate enouph memory to hold the error message
        char* log = (char*) alloca(length * sizeof(char));
		// Display the error message
        glGetProgramInfoLog(program, length, &length, log);
		cerr << "Linker log: " << endl << log << endl;
		return 0;
    }
	// Cleanup
	glUseProgram(program);
	glDeleteShader(shader);

	return program;
}
