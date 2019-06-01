#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H
#include "ComputeBuffer.h"

#include <unordered_map>

template<typename T>
struct identity { typedef T type; };

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

	template <class T>
	T getParameter(const string& name){ return getParameter(name, identity<T>()); }

	template <class T>
	vector<T> getParameter(const string& name, size_t count){ return getParameter(name, count, identity<T>()); }

	// Int
	void setParameter(const string& name, int value){
		glUseProgram(programID);
		glUniform1i(getUniformLocation(name), value);
	}

	void setParameter(const string& name, int* value, size_t count){
		glUseProgram(programID);
		glUniform1iv(getUniformLocation(name), count, value);
	}

	void setParameter(const string& name, vector<int> value){
		setParameter(name, value.data(), value.size());
	}

	// float
	void setParameter(const string& name, float value){
		glUseProgram(programID);
		glUniform1f(getUniformLocation(name), value);
	}

	void setParameter(const string& name, float* value, size_t count){
		glUseProgram(programID);
		glUniform1fv(getUniformLocation(name), count, value);
	}

	void setParameter(const string& name, vector<float> value){
		setParameter(name, value.data(), value.size());
	}

	// Bool
	void setParameter(const string& name, bool value){
		glUseProgram(programID);
		glUniform1i(getUniformLocation(name), value);
	}

	void setParameter(const string& name, bool* value, size_t count){
		glUseProgram(programID);
		glUniform1iv(getUniformLocation(name), count, (int*)value);
	}

	/*void setParameter(const string& name, vector<bool> value){
		setParameter(name, value.data(), value.size());
	}*/

	// etc...

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

	const int getUniformLocation(const string & name)
	{
		static unordered_map<string, int> uniformCache;
		// Check to see if the uniform exists in the cache...
		if (uniformCache.find(name) != uniformCache.end())
			return uniformCache[name]; // If so return the value in the cache

		// Get the location ID of the uniform
		int location = glGetUniformLocation(programID, name.c_str());
		// If the location isn't valid...
		if (location == -1)
			// Print a warning
			cout << "Warning: uniform '" << name << "' doesn't exist!" << endl;

		uniformCache[name] = location; // Cache the location
		return location; // Return the location
	}

	int getParameter(const string& name, identity<int> i){
		int out;
		glGetUniformiv(programID, getUniformLocation(name), &out);
		return out;
	}

	vector<int> getParameter(const string& name, size_t count, identity<int> i){
		vector<int> out(count);
		for(int& cur: out)
			glGetUniformiv(programID, getUniformLocation(name), &cur);
		return out;
	}

	float getParameter(const string& name, identity<float> i){
		float out;
		glGetUniformfv(programID, getUniformLocation(name), &out);
		return out;
	}

	vector<float> getParameter(const string& name, size_t count, identity<float> i){
		vector<float> out(count);
		for(float& cur: out)
			glGetUniformfv(programID, getUniformLocation(name), &cur);
		return out;
	}

	bool getParameter(const string& name, identity<bool> i){
		int out;
		glGetUniformiv(programID, getUniformLocation(name), &out);
		return out;
	}

	vector<bool> getParameter(const string& name, size_t count, identity<bool> i){
		vector<bool> out(count);
		for(int i = 0; i < count; i++){
			int holder;
			glGetUniformiv(programID, getUniformLocation(name), &holder);
			out[i] = holder;
		}

		return out;
	}
};

#endif
