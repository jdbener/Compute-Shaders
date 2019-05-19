#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

// Macro which creates an exception structure
#define error(_name_, _message_) \
	struct _name_ { _name_(string file, int line) { \
	cerr << file << " line #" << line << " error: " << _message_ << endl; }};

error(ProgramCreationError, "Creating Program!")
error(BufferCreationError, "Invalid Buffer Size!")
error(WorkGroupCountError, "Dimension greater than maximum!")
typedef unsigned int byte;

class ComputeShader {
private:
	unsigned int programID, // Variable storing the ID of the compiled GLSL program
		bufferID,			// Variable storing the ID of the storage buffer
		bufferSize;			// Variable storing the size (in bytes) of the storage buffer

public:
	ComputeShader(const string& src, byte _bufferSize, void* data = nullptr)
		: bufferSize(_bufferSize) {
		construct(src, data);
	}

	ComputeShader(ifstream& shaderFile, byte _bufferSize, void* data = nullptr)
		: bufferSize(_bufferSize) {
		const char END_OF_FILE = 26;

		string src;
		getline(shaderFile, src, END_OF_FILE);
		construct(src, data);
	}

	~ComputeShader(){
		glDeleteProgram(programID);
		glDeleteBuffers(1, &bufferID);
	}

	void dispatch(unsigned int x, unsigned int y = 1, unsigned int z = 1){
		glUseProgram(programID);
		glDispatchCompute(x, y, z);
	}

	void pull(void* data, byte min = 0, byte max = 0){
		if(max < 1) max = bufferSize;
		// Copy the storage buffer into our data buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		void* p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, min, max, GL_MAP_READ_BIT);
		memcpy(data, p, bufferSize);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

	void push(void* data, unsigned int min = 0, byte max = 0){
		if(max < 1) max = bufferSize;
		// Copy the data buffer into our storage buffer
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		void* p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, min, max, GL_MAP_WRITE_BIT);
		memcpy(p, data, max - min);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

private:
	void construct(const string& src, void* data){
		const int BUFFER_BINDING_POINT = 7;
		// If we got an invalid size for the buffer... error
		if(!bufferSize) throw BufferCreationError(__FILE__, __LINE__);

		// Create the program
		programID = createProgram(src.c_str());
		// If the shader failed to be compiled/linked... error
		if(!programID) throw ProgramCreationError(__FILE__, __LINE__);

		// Create the storage buffer
		glGenBuffers(1, &bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING_POINT, bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

	// Original code thanks to http://wili.cc/blog/opengl-cs.html
	unsigned int createProgram(const char* src) {
		// Creating the compute shader, and the program object containing the shader
	    unsigned int program = glCreateProgram();
	    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
		int status;

		// Compile the shader
	    glShaderSource(shader, 1, &src, NULL);
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
};

#endif
