#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

using namespace std;

typedef unsigned int bytes;

// Macro which creates an exception structure
#define error(_name_, _message_) \
	struct _name_ { _name_(string file, int line) { \
	cerr << file << " line #" << line << " error: " << _message_ << endl; }};

error(BufferCreationError, "Invalid Buffer Size!")

class ComputeBuffer {
private:
	unsigned int bufferID,	// Variable storing the ID of the storage buffer
			bindingPoint;	// Variable storing where the buffer is bound
	bytes bufferSize;		// Variable storing the size (in bytes) of the storage buffer

public:
	ComputeBuffer(unsigned int _bindingPoint, bytes size, void* data = nullptr)
	: bindingPoint(_bindingPoint), bufferSize(size) {
		createBuffer(data);
	}

	template <class T>
	ComputeBuffer(unsigned int _bindingPoint, vector<T>& data)
	: bindingPoint(_bindingPoint) {
		bufferSize = data.size() * sizeof(data[0]);
		createBuffer(&data[0]);
	}

	~ComputeBuffer(){
		release();
	}

	void release(){
		glDeleteBuffers(1, &bufferID);
	}

	void* getNativePointer(unsigned int accessbits = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, bytes start = 0, bytes finish = 0){
		if(finish < 1) finish = bufferSize;
		// TODO: Bounds Checking?

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		void* p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, start, finish, accessbits);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
		return p;
	}

	void getData(void* dataStorage, bytes start = 0, bytes finish = 0){
		if(finish < 1) finish = bufferSize;
		void* p = getNativePointer(GL_MAP_READ_BIT, start, finish);

		memcpy(dataStorage, p, finish - start);
	}

	template <class T>
	void getData(vector<T>& dataStorage, bytes start = 0, bytes finish = 0){
		getData(&dataStorage[0], start, finish);
	}

	void setData(void* data, bytes start = 0, bytes finish = 0){
		if(finish < 1) finish = bufferSize;
		void* p = getNativePointer(GL_MAP_WRITE_BIT, start, finish);

		memcpy(p, data, finish - start);
	}

	template <class T>
	void setData(vector<T>& data, bytes start = 0, bytes finish = 0){
		setData(&data[0], start, finish);
	}

	unsigned int getBindingPoint(){
		return bindingPoint;
	}

private:
	void createBuffer(void* data){
		// If we got an invalid size for the buffer... error
		if(!bufferSize) throw BufferCreationError(__FILE__, __LINE__);

		// Create the storage buffer
		glGenBuffers(1, &bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}
};
