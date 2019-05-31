#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H
#include "ComputeBuffer.h"

class ComputeShader {
private:
	unsigned int programID; // Variable storing the ID of the compiled GLSL program
public:
	ComputeShader(const string& src) {
		// Create the program
		programID = createProgram(src.c_str());
		// If the shader failed to be compiled/linked... error
		if(!programID) assert(0 && "Error: Creating program!");
	}

	ComputeShader(ifstream& shaderFile){
		const char END_OF_FILE = 26;

		string src;
		getline(shaderFile, src, END_OF_FILE);
		// Create the program
		programID = createProgram(src.c_str());
		// If the shader failed to be compiled/linked... error
		if(!programID) assert(0 && "Error: Creating program!");
	}

	~ComputeShader(){
		glDeleteProgram(programID);
	}

	void dispatch(unsigned int x, unsigned int y = 1, unsigned int z = 1){
		glUseProgram(programID);
		glDispatchCompute(x, y, z);
	}

private:
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
