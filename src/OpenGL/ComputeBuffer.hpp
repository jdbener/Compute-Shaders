#ifndef __COMPUTE_BUFFER_H__
#define __COMPUTE_BUFFER_H__

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cstring>
#include <cassert>

#define ensureNotCommited() if(committed) assert(0 && "Error: Cannot add fields after the buffer has been commited!")

class ComputeBuffer {
protected:
	unsigned int bufferID = 0,	// Variable storing the ID of the storage buffer
			bindingPoint;		// Variable storing where the buffer is bound
	size_t bufferSize;			// Variable storing the size (in size_t) of the storage buffer
	bool committed = false;		// Variable used to determine whether or not it is safe to preform opperations on the buffer

public:
	ComputeBuffer(unsigned int _bindingPoint, size_t size, void* data = nullptr)
	: bindingPoint(_bindingPoint), bufferSize(size) {
		createBuffer(data);
	}

	template <class T>
	ComputeBuffer(unsigned int _bindingPoint, std::vector<T>& data)
	: bindingPoint(_bindingPoint) {
		bufferSize = data.size() * sizeof(data[0]);
		createBuffer(data.data());
	}

	ComputeBuffer(unsigned int _bindingPoint) : bindingPoint(_bindingPoint) {}

	virtual ~ComputeBuffer(){
		release();
	}

	virtual void release(){
		if(bufferID){
			glDeleteBuffers(1, &bufferID);
			bufferID = 0;
		}

		committed = false;
	}

	void getData(void* dataStorage, size_t start = 0, size_t finish = 0){
		if(finish < 1) finish = bufferSize;
		void* p = getNativePointer(GL_MAP_READ_BIT, start, finish);

		memcpy(dataStorage, p, finish - start);
	}

	template <class T>
	void getData(std::vector<T>& dataStorage, size_t start = 0, size_t finish = 0){
		getData(dataStorage.data(), start, finish);
	}

	void setData(void* data, size_t start = 0, size_t finish = 0){
		if(finish < 1) finish = bufferSize;
		void* p = getNativePointer(GL_MAP_WRITE_BIT, start, finish);

		memcpy(p, data, finish - start);
	}

	template <class T>
	void setData(std::vector<T>& data, size_t start = 0, size_t finish = 0){
		setData(data.data(), start, finish);
	}

	unsigned int getBindingPoint(){
		return bindingPoint;
	}

protected:
	void createBuffer(void* data){
		// If we got an invalid size for the buffer... error
		if(!bufferSize) assert(0 && "Error: Invalid buffer size!");

		// Create the storage buffer
		glGenBuffers(1, &bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, data, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferID);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

		committed = true;
	}

	void* getNativePointer(unsigned int accessbits = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, size_t start = 0, size_t finish = 0){
		if(!committed) assert(0 && "Error: Cannot access buffer before commiting!");
		if(finish < start + 1) finish = bufferSize;
		// TODO: Bounds Checking?

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferID);
		void* p = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, start, finish, accessbits);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
		return p;
	}
};

class StructuredComputeBuffer: private ComputeBuffer {
private:
	size_t structuredSize = 0;			// Variable used by the structed system to keep a running total of the size of structuredData
	uint8_t* structuredData = nullptr;	// Variable used as intermediary storage of fields by the structured system

public:
	using ComputeBuffer::ComputeBuffer;
	using ComputeBuffer::getData;
	using ComputeBuffer::getBindingPoint;

	virtual ~StructuredComputeBuffer(){
		release();
	}

	template <class T>
	void addField(){
		ensureNotCommited();

		// Calclulate how big the new buffer should be
		unsigned int oldSize = structuredSize;
		structuredSize += sizeof(T);

		// Save pointers to the old buffer
		uint8_t* old = structuredData;
		// And make a new block of memory
		structuredData = new uint8_t[structuredSize];

		// If we have an old buffer to copy...
		if(old){
			// Copy and delete
			memcpy(structuredData, old, oldSize);
			delete old;
		}
	}

	template <class T>
	void addField(T& what){
		ensureNotCommited();

		// Calclulate how big the new buffer should be
		unsigned int oldSize = structuredSize;
		structuredSize += sizeof(T);

		// Save pointers to the old buffer
		uint8_t* old = structuredData;
		// And make a new block of memory
		structuredData = new uint8_t[structuredSize];
		// Declair that we will copy the new data into that block of memory
		uint8_t* next = structuredData;

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			next = (uint8_t*) mempcpy(structuredData, old, oldSize);
			delete old;
		}

		// Copy our new data into the buffer
		memcpy(next, &what, structuredSize - oldSize);
	}
	template <class T> void addField(T&& what){ addField(what); }

	template <class T>
	void addField(size_t elements){
		ensureNotCommited();

		// Calclulate how big the new buffer should be
		size_t oldSize = structuredSize;
		structuredSize += sizeof(T) * elements;

		// Save pointers to the old buffer
		uint8_t* old = structuredData;
		// And make a new block of memory
		structuredData = new uint8_t[structuredSize];

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			memcpy(structuredData, old, oldSize);
			delete old;
		}
	}

	template <class T>
	void addField(const T* what, size_t elements){
		ensureNotCommited();

		// Calclulate how big the new buffer should be
		size_t oldSize = structuredSize;
		structuredSize += sizeof(T) * elements;

		// Save pointers to the old buffer
		uint8_t* old = structuredData;
		// And make a new block of memory
		structuredData = new uint8_t[structuredSize];
		// Declair that we will copy the new data into that block of memory
		uint8_t* next = structuredData;

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			next = (uint8_t*) mempcpy(structuredData, old, oldSize);
			delete old;
		}

		// Copy our new data into the buffer
		memcpy(next, &what, structuredSize - oldSize);
	}
	template <class T> void addField(std::vector<T>& what){ addField(what.data(), what.size()); }

	void commit(){
		if(!committed){
			if(!structuredData || !structuredSize) assert(0 && "Error: No data in buffer!");

			bufferSize = structuredSize;
			createBuffer(structuredData);
		}
	}

	virtual void release(){
		if(structuredData){
			delete [] structuredData;
			structuredData = nullptr;
		}

		ComputeBuffer::release();
	}

};

#endif // __COMPUTE_BUFFER_H__
