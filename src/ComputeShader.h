#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H
#include "ComputeBuffer.h"

#include <map>

error(ProgramCreationError, "Creating Program!")
error(WorkGroupCountError, "Dimension greater than maximum!")

class ComputeShader {
private:
	unsigned int programID; // Variable storing the ID of the compiled GLSL program
	//map<unsigned int, ComputeBuffer*> buffers;
public:
	ComputeShader(const string& src) {
		// Create the program
		programID = createProgram(src.c_str());
		// If the shader failed to be compiled/linked... error
		if(!programID) throw ProgramCreationError(__FILE__, __LINE__);
	}

	ComputeShader(ifstream& shaderFile){
		const char END_OF_FILE = 26;

		string src;
		getline(shaderFile, src, END_OF_FILE);
		// Create the program
		programID = createProgram(src.c_str());
		// If the shader failed to be compiled/linked... error
		if(!programID) throw ProgramCreationError(__FILE__, __LINE__);
	}

	~ComputeShader(){
		glDeleteProgram(programID);
	}

	void dispatch(unsigned int x, unsigned int y = 1, unsigned int z = 1){
		glUseProgram(programID);
		glDispatchCompute(x, y, z);
	}

	/*void addBuffer(ComputeBuffer& buffer){
		buffers[buffer.getBindingPoint()] = &buffer;
	}

	void removeBuffer(unsigned int bindingPoint, bool release = true){
		if(release)
			buffers[bindingPoint]->release();
		buffers.erase(bindingPoint);
	}

	ComputeBuffer* getBuffer(unsigned int bindingPoint){
		return buffers[bindingPoint];
	}

	void* getNativePointer(unsigned int bindingPoint, unsigned int accessbits = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, bytes start = 0, bytes finish = 0){
		return getBuffer(bindingPoint)->getNativePointer(accessbits, start, finish);
	}

	void getData(unsigned int bindingPoint, void* dataStorage, bytes start = 0, bytes finish = 0){
		getBuffer(bindingPoint)->getData(dataStorage, start, finish);
	}

	template <class T>
	void getData(unsigned int bindingPoint, vector<T>& dataStorage, bytes start = 0, bytes finish = 0){
		getBuffer(bindingPoint)->getData(dataStorage, start, finish);
	}

	void setData(unsigned int bindingPoint, void* data, bytes start = 0, bytes finish = 0){
		getBuffer(bindingPoint)->setData(data, start, finish);
	}

	template <class T>
	void setData(unsigned int bindingPoint, vector<T>& data, bytes start = 0, bytes finish = 0){
		getBuffer(bindingPoint)->setData(data, start, finish);
	}*/

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
