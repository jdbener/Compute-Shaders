#ifndef __COMPUTE_BUFFER_VULK_H__
#define __COMPUTE_BUFFER_VULK_H__
#include "BoilerPlate.hpp"

#include <vector>
#include <cstring>
#include <cassert>

#define ensureNotCommited() if(committed) assert(0 && "Error: Cannot add fields after the buffer has been commited!")

class ComputeShader;

class ComputeBuffer {
friend class ComputeShader;
protected:
	VulkanContext& context;

	unsigned int bindingPoint;	// Variable storing where the buffer is bound
	vk::DeviceSize bufferSize;	// Variable storing the size (in size_t) of the storage buffer
	bool committed = false;		// Variable used to determine whether or not it is safe to preform opperations on the buffer

	vk::DeviceMemory memory = nullptr;
	vk::Buffer buffer = nullptr;

public:
	ComputeBuffer(VulkanContext& c, unsigned int _bindingPoint, vk::DeviceSize size, void* data = nullptr, vk::DeviceSize dataStart = 0, vk::DeviceSize dataEnd = 0)
	: context(c), bindingPoint(_bindingPoint), bufferSize(size) {
		createBuffer();
		if(data) setData(data, dataStart, dataEnd);
		//createBuffer(data);
	}

	template <class T>
	ComputeBuffer(VulkanContext& c, unsigned int _bindingPoint, std::vector<T>& data, vk::DeviceSize dataStart = 0, vk::DeviceSize dataEnd = 0)
	: context(c), bindingPoint(_bindingPoint) {
		bufferSize = data.size() * sizeof(data[0]);
		createBuffer();
		setData(data.data(), dataStart, dataEnd);
		// createBuffer(data.data());
	}

	ComputeBuffer(VulkanContext& c, unsigned int _bindingPoint) : context(c), bindingPoint(_bindingPoint) {}

	virtual ~ComputeBuffer(){
		release();
	}

	virtual void release(){
		// Free the memory associated with this buffer (if nessicary)
		if(memory){
			context.device->free(memory);
			memory = nullptr;
		}

		// Clean up the buffer (if nessicary)
		if(buffer){
			context.device->destroy(buffer);
			buffer = nullptr;
		}

		committed = false;
	}

	void getData(void* dataStorage, vk::DeviceSize start = 0, vk::DeviceSize finish = 0){
		if(finish < 1) finish = bufferSize;

		vk::DeviceSize size = finish - start;
		if(size == 0) return;

		vk::CommandBuffer cb = context.device->allocateCommandBuffers( {context.commandPool.get(), vk::CommandBufferLevel::ePrimary, 1} )[0];
		auto stagingBuffer = createStagingBuffer(size);

		// Queue up a copy from the permanent buffer to the staging buffer
		cb.begin( {vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr} ); {
			vk::BufferCopy copy(start, 0, size);
			cb.copyBuffer(buffer, stagingBuffer.second, copy);
		} cb.end();
		vk::SubmitInfo si(0, nullptr, nullptr, 1, &cb, 0, nullptr);
		context.computeQueue.submit(si);

		// Wait for the copy to complete
		context.computeQueue.waitIdle();

		// Copy the provided data from the staging buffer
		void* map = context.device->mapMemory(stagingBuffer.first, 0, size, {});
		memcpy(dataStorage, map, size);
		context.device->unmapMemory(stagingBuffer.first);

		// Free all the resources we created
		context.device->free(stagingBuffer.first);
		context.device->destroy(stagingBuffer.second);
		context.device->freeCommandBuffers(context.commandPool.get(), 1, &cb);
	}

	template <class T>
	void getData(std::vector<T>& dataStorage, vk::DeviceSize start = 0, vk::DeviceSize finish = 0){
		getData(dataStorage.data(), start, finish);
	}

	void setData(void* data, vk::DeviceSize start = 0, vk::DeviceSize finish = 0){
		if(finish < 1) finish = bufferSize;

		vk::DeviceSize size = finish - start;
		if(size == 0) return;

		vk::CommandBuffer cb = context.device->allocateCommandBuffers( {context.commandPool.get(), vk::CommandBufferLevel::ePrimary, 1} )[0];
		auto stagingBuffer = createStagingBuffer(size);

		// Copy the provided data to the staging buffer
		void* map = context.device->mapMemory(stagingBuffer.first, 0, size, {});
		memcpy(map, data, size);
		context.device->unmapMemory(stagingBuffer.first);

		// Queue up a copy from the staging buffer to the permanent buffer
		cb.begin( {vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr} ); {
			vk::BufferCopy copy(0, start, size);
			cb.copyBuffer(stagingBuffer.second, buffer, copy);
		} cb.end();
		vk::SubmitInfo si(0, nullptr, nullptr, 1, &cb, 0, nullptr);
		context.computeQueue.submit(si);

		// Wait for the copy to complete
		context.computeQueue.waitIdle();

		// Free all the resources we created
		context.device->free(stagingBuffer.first);
		context.device->destroy(stagingBuffer.second);
		context.device->freeCommandBuffers(context.commandPool.get(), 1, &cb);
	}

	template <class T>
	void setData(std::vector<T>& data, vk::DeviceSize start = 0, vk::DeviceSize finish = 0){
		setData(data.data(), start, finish);
	}

	unsigned int getBindingPoint(){
		return bindingPoint;
	}

protected:
	void createBuffer(){
		// Create the Vulkan Buffer
		buffer = context.device->createBuffer( {{}, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &context.computeQueueIndex} );
		auto requirements = context.device->getBufferMemoryRequirements(buffer);
		vk::DeviceSize actualSize = bufferSize;
		if(requirements.size > actualSize) actualSize = requirements.size;

		// Pick the memory type for this buffer
		auto properties = context.physicalDevice.getMemoryProperties();
		uint32_t memoryTypeIndex = -1;
		for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
			const VkMemoryType memoryType = properties.memoryTypes[k];
			if ( (actualSize < properties.memoryHeaps[memoryType.heapIndex].size) ) {
				memoryTypeIndex = k;
				break;
			}
		}
		// Allocate memory on the GPU for this buffer
		memory = context.device->allocateMemory( {actualSize, memoryTypeIndex} );

		// Bind it to the memory on the GPU
		context.device->bindBufferMemory(buffer, memory, 0);

		committed = true;
	}

	void createBuffer(void* data){
		createBuffer();
		setData(data);
	}

	std::pair<vk::DeviceMemory, vk::Buffer> createStagingBuffer(vk::DeviceSize size){
		// Create the staging buffer
		vk::Buffer buf = context.device->createBuffer( {{}, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &context.computeQueueIndex} );
		auto requirements = context.device->getBufferMemoryRequirements(buf);
		if(requirements.size > size) size = requirements.size;

		// Pick the memory type for this buffer
		auto properties = context.physicalDevice.getMemoryProperties();
		uint32_t memoryTypeIndex = -1;
		for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
			const VkMemoryType memoryType = properties.memoryTypes[k];
			if ( (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memoryType.propertyFlags)
			  && (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memoryType.propertyFlags)
			  && (size < properties.memoryHeaps[memoryType.heapIndex].size) ) {
				memoryTypeIndex = k;
				break;
			}
		}
		// Allocate memory on the GPU for this buffer
		vk::DeviceMemory mem = context.device->allocateMemory( {size, memoryTypeIndex} );

		// Bind it to the memory on the GPU
		context.device->bindBufferMemory(buf, mem, 0);

		return {mem, buf};
	}
};

#endif // __COMPUTE_BUFFER_VULK_H__
