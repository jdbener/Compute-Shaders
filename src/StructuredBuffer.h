#ifndef STRUCTURED_BUFFER_H
#define STRUCTURED_BUFFER_H

#include "ComputeBuffer.h"

#define validateCommit() if(!committed) assert(0 && "Error: Cannot access buffer before commiting")

class StructuredBuffer {
private:
	ComputeBuffer* buffer = nullptr;
	unsigned int size = 0, bindingPoint;
	char* data = nullptr;
	bool committed = false;
public:
	StructuredBuffer(unsigned int _bindingPoint) : bindingPoint(_bindingPoint) {}

	~StructuredBuffer(){
		if(data)
			delete data;
		if(buffer)
			delete buffer;
	}

	template <class T>
	void addField(T& what){
		// Calclulate how big the new buffer should be
		unsigned int oldSize = size;
		size += sizeof(T);

		// Save pointers to the old buffer
		char* old = data;
		// And make a new block of memory
		data = new char[size];
		// Declair that we will copy the new data into that block of memory
		char* next = data;

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			next = (char*) mempcpy(data, old, oldSize);
			delete old;
		}

		// Copy our new data into the buffer
		memcpy(next, &what, size - oldSize);
	}

	template <class T>
	void addField(T&& what){
		addField(what);
	}

	template <class T>
	void addField(unsigned int elements){
		// Calclulate how big the new buffer should be
		unsigned int oldSize = size;
		size += sizeof(T) * elements;

		// Save pointers to the old buffer
		char* old = data;
		// And make a new block of memory
		data = new char[size];

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			memcpy(data, old, oldSize);
			delete old;
		}
	}

	template <class T>
	void addField(const T* what, unsigned int elements){
		// Calclulate how big the new buffer should be
		unsigned int oldSize = size;
		size += sizeof(T) * elements;

		// Save pointers to the old buffer
		char* old = data;
		// And make a new block of memory
		data = new char[size];
		// Declair that we will copy the new data into that block of memory
		char* next = data;

		// If we have an old buffer to copy...
		if(old){
			// Copy, delete, and save a pointer to the next byte of the buffer
			next = (char*) mempcpy(data, old, oldSize);
			delete old;
		}

		// Copy our new data into the buffer
		memcpy(next, &what, size - oldSize);
	}

	template <class T>
	void addField(vector<T>& what){
		addArrayField(what.data(), what.size());
	}

	void release(){
		if(buffer){
			delete buffer;
			buffer = nullptr;
		}

		committed = false;
	}

	void commit(){
		buffer = new ComputeBuffer(bindingPoint, size, data);
		committed = true;
	}

	void* getNativePointer(unsigned int accessbits = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT, bytes start = 0, bytes finish = 0){
		validateCommit();
		return buffer->getNativePointer(accessbits, start, finish);
	}

	void getData(void* dataStorage, bytes start = 0, bytes finish = 0){
		validateCommit();
		buffer->getData(dataStorage, start, finish);
	}

	template <class T>
	void getData(vector<T>& dataStorage, bytes start = 0, bytes finish = 0){
		validateCommit();
		buffer->getData(dataStorage, start, finish);
	}

	void setData(void* data, bytes start = 0, bytes finish = 0){
		validateCommit();
		buffer->setData(data, start, finish);
	}

	template <class T>
	void setData(vector<T>& data, bytes start = 0, bytes finish = 0){
		validateCommit();
		buffer->setData(data, start, finish);
	}

	unsigned int getBindingPoint(){
		validateCommit();
		return bindingPoint;
	}
};

#endif // STRUCTURED_BUFFER_H
