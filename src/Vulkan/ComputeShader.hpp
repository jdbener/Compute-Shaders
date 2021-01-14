#ifndef __COMPUTE_SHADER_VULK_H__
#define __COMPUTE_SHADER_VULK_H__
#include "ComputeBuffer.hpp"

#include <unordered_map>
#include <iostream>
#include <fstream>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include "DirStackFileIncluder.h"

template<typename T>
struct identity { typedef T type; };

class ComputeShader {
protected:
	VulkanContext& context;

	vk::UniqueShaderModule program;
	vk::UniqueDescriptorSetLayout descriptorSetLayout;
	vk::UniqueDescriptorPool descriptorPool;
	vk::DescriptorSet descriptorSet;
	vk::UniquePipelineLayout pipelineLayout;
	vk::UniquePipeline pipeline;

	std::vector<uint8_t> pushConstants;
protected:
	struct CBWrapper {
		ComputeBuffer* buffer = nullptr;
		bool owned = false;
	};
	std::vector<CBWrapper> buffers;
public:
	ComputeShader(VulkanContext& _context, std::ifstream& shaderFile) : context(_context) {
		const char END_OF_FILE = 26;

		std::string src;
		getline(shaderFile, src, END_OF_FILE);

		// Create the program
		program = compileShaderModule(src);
	}

	~ComputeShader(){
		for(CBWrapper& wrap: buffers)
			if(wrap.owned)
				delete wrap.buffer;
	}

	void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1){
		// Make sure the pipeline has been created before we submit the shader
		if(!pipeline) finalizePipeline();

		// Create a new CommandBuffer (Should the buffer be stored instead of recreated every time?)
		vk::CommandBuffer cb = context.device->allocateCommandBuffers( {context.commandPool.get(), vk::CommandBufferLevel::ePrimary, 1} )[0];
		cb.begin( {vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr} ); {
			// Bind pipeline and buffers
			cb.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.get());
			/*if(!descriptorSets.empty())*/ cb.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), /*firstSet*/ 0, descriptorSet, {});
			if(!pushConstants.empty()) cb.pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eCompute, 0, (uint32_t) pushConstants.size(), pushConstants.data());

			// Dispatch compute shader
			cb.dispatch(x, y, z);
		} cb.end();

		// Submit the command buffer
		context.computeQueue.submit( {{0, nullptr, nullptr, 1, &cb, 0, nullptr}} );
		// Wait for the command buffer to finish
		context.computeQueue.waitIdle();

		// Release resources
		context.device->freeCommandBuffers(context.commandPool.get(), cb);
	}


	/////  Compute Buffers  /////

	ComputeBuffer& createComputeBuffer(vk::DeviceSize size){
		CBWrapper _new;
		_new.buffer = new ComputeBuffer(context, buffers.size() + 1, size);
		_new.owned = true;
		buffers.push_back(_new);

		return *_new.buffer;
	}

	ComputeBuffer& createComputeBuffer(vk::DeviceSize size, size_t bindPoint){
		// Make sure the array is big enouph
		if(bindPoint + 1 > buffers.size()) buffers.resize(bindPoint + 1);

		// Delete the old buffer if it exists
		CBWrapper& wrap = buffers[bindPoint];
		if(wrap.buffer && wrap.owned) delete wrap.buffer;

		// Create the new buffer
		wrap.buffer = new ComputeBuffer(context, bindPoint, size);
		wrap.owned = true;

		return *wrap.buffer;
	}

	void bindComputeBuffer(ComputeBuffer& _new){
		size_t bindPoint = _new.getBindingPoint();

		// Make sure the array is big enouph
		if(bindPoint + 1 > buffers.size()) buffers.resize(bindPoint + 1);

		// Delete the old buffer if it exists
		CBWrapper& wrap = buffers[bindPoint];
		if(wrap.buffer && wrap.owned) delete wrap.buffer;

		wrap.buffer = &_new;
		wrap.owned = false;
	}

	void releaseBuffer(size_t bindPoint) {
		if (bindPoint > buffers.size()) return;

		CBWrapper& wrap = buffers[bindPoint];
		if(wrap.buffer && wrap.owned) delete wrap.buffer;
		wrap.buffer = nullptr;
		wrap.owned = false;

		// Remove any unessicary buffers from the end of the array
		while(buffers[buffers.size() - 1].buffer == nullptr)
			buffers.pop_back();
	}


	/////  Push Constants  /////

	void setPushConstants(std::vector<uint8_t> data) {
		if(data.size() > 128) assert(0 && "Data is too large to fit in the push constant buffer");
		pushConstants = data;

		// Ensure that the buffer's size is always divisible by 4
		if(pushConstants.size() % 4 != 0) pushConstants.resize(pushConstants.size() + 4 - pushConstants.size() % 4);
	}

	template<class T>
	void setPushConstants(T strct){
		// Copy the struct into a byte array
		std::vector<uint8_t> data;
		data.resize(sizeof(T));
		memcpy(data.data(), &strct, sizeof(T));

		// Store the byte array (validating its size)
		setPushConstants(data);
	}

	const std::vector<uint8_t>& getPushConstants(){
		return pushConstants;
	}

	template<class T>
	T getPushConstants() {
		T out;
		memcpy(&out, pushConstants.data(), sizeof(T));
		return out;
	}

private:
	// Create the Pipeline with all nessicary bindings
	void finalizePipeline() {
		// Create the Descriptor Set Layout
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for(uint32_t bindPoint = 0; bindPoint < buffers.size(); bindPoint++)
			if(buffers[bindPoint].buffer)
				bindings.emplace_back(bindPoint, vk::DescriptorType::eStorageBuffer, /*descriptorCount*/ 1, vk::ShaderStageFlagBits::eCompute, nullptr);
		if(!bindings.empty()){
			descriptorSetLayout = context.device->createDescriptorSetLayoutUnique( {{}, (uint32_t) bindings.size(), bindings.data()} );

			// Create the Descriptor Pool
			vk::DescriptorPoolSize size(vk::DescriptorType::eStorageBuffer, /*should just be 1?*/ uint32_t(bindings.size() > 0 ? bindings.size() : 1));
			descriptorPool = context.device->createDescriptorPoolUnique( {{}, uint32_t(buffers.size() > 0 ? buffers.size() : 1), size} );

			// Create the Descriptor Sets
			descriptorSet = context.device->allocateDescriptorSets( {descriptorPool.get(), 1, &descriptorSetLayout.get()} )[0];
			// Bind the buffers we have created to the descriptor sets
			std::vector<vk::DescriptorBufferInfo> buffInfo(bindings.size());
			std::vector<vk::WriteDescriptorSet> writes(bindings.size());
			for(uint32_t i = 0; i < bindings.size(); i++){
				uint32_t bindPoint = bindings[i].binding;
				buffInfo[i] = vk::DescriptorBufferInfo(buffers[bindPoint].buffer->buffer, 0, buffers[bindPoint].buffer->bufferSize);
				writes[i] = vk::WriteDescriptorSet(descriptorSet, bindPoint, /*dstArrayElement*/ 0, 1, vk::DescriptorType::eStorageBuffer, /*image*/ nullptr, &buffInfo[i], /*texelBuffer*/ nullptr);
			}
			context.device->updateDescriptorSets(writes, /*copies*/ {});
		}

		// Create the pipeline layout
		vk::PushConstantRange constantRange(vk::ShaderStageFlagBits::eCompute, /*offset*/ 0, (uint32_t) pushConstants.size());
		pipelineLayout = context.device->createPipelineLayoutUnique( {{}, uint32_t(bindings.empty() ? 0 : 1), &descriptorSetLayout.get(), uint32_t(pushConstants.empty() ? 0 : 1), &constantRange} );

		// Create the pipeline
		vk::PipelineShaderStageCreateInfo stage({}, vk::ShaderStageFlagBits::eCompute, program.get(), "main", nullptr);
		pipeline = context.device->createComputePipelineUnique(/*cache*/{}, {vk::PipelineCreateFlagBits::eDispatchBase, stage, pipelineLayout.get(), {}, {}}).value;
	}

	// Compiles the provided GLSL source code into a SPIR-V based vulkan shader module
	//  Saves the resulting binary array if the debugging mode is turned on
	//  NOTE: Slightly modified from: https://forestsharp.com/glslang-cpp/
	vk::UniqueShaderModule compileShaderModule(std::string& sourceCode, std::string entryPoint = "main"){

	    // TODO: Look at what all is in this monolithic beast
	    TBuiltInResource DefaultTBuiltInResource = {
	        /* .MaxLights = */ 32,
	        /* .MaxClipPlanes = */ 6,
	        /* .MaxTextureUnits = */ 32,
	        /* .MaxTextureCoords = */ 32,
	        /* .MaxVertexAttribs = */ 64,
	        /* .MaxVertexUniformComponents = */ 4096,
	        /* .MaxVaryingFloats = */ 64,
	        /* .MaxVertexTextureImageUnits = */ 32,
	        /* .MaxCombinedTextureImageUnits = */ 80,
	        /* .MaxTextureImageUnits = */ 32,
	        /* .MaxFragmentUniformComponents = */ 4096,
	        /* .MaxDrawBuffers = */ 32,
	        /* .MaxVertexUniformVectors = */ 128,
	        /* .MaxVaryingVectors = */ 8,
	        /* .MaxFragmentUniformVectors = */ 16,
	        /* .MaxVertexOutputVectors = */ 16,
	        /* .MaxFragmentInputVectors = */ 15,
	        /* .MinProgramTexelOffset = */ -8,
	        /* .MaxProgramTexelOffset = */ 7,
	        /* .MaxClipDistances = */ 8,
	        /* .MaxComputeWorkGroupCountX = */ 65535,
	        /* .MaxComputeWorkGroupCountY = */ 65535,
	        /* .MaxComputeWorkGroupCountZ = */ 65535,
	        /* .MaxComputeWorkGroupSizeX = */ 1024,
	        /* .MaxComputeWorkGroupSizeY = */ 1024,
	        /* .MaxComputeWorkGroupSizeZ = */ 64,
	        /* .MaxComputeUniformComponents = */ 1024,
	        /* .MaxComputeTextureImageUnits = */ 16,
	        /* .MaxComputeImageUniforms = */ 8,
	        /* .MaxComputeAtomicCounters = */ 8,
	        /* .MaxComputeAtomicCounterBuffers = */ 1,
	        /* .MaxVaryingComponents = */ 60,
	        /* .MaxVertexOutputComponents = */ 64,
	        /* .MaxGeometryInputComponents = */ 64,
	        /* .MaxGeometryOutputComponents = */ 128,
	        /* .MaxFragmentInputComponents = */ 128,
	        /* .MaxImageUnits = */ 8,
	        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	        /* .MaxCombinedShaderOutputResources = */ 8,
	        /* .MaxImageSamples = */ 0,
	        /* .MaxVertexImageUniforms = */ 0,
	        /* .MaxTessControlImageUniforms = */ 0,
	        /* .MaxTessEvaluationImageUniforms = */ 0,
	        /* .MaxGeometryImageUniforms = */ 0,
	        /* .MaxFragmentImageUniforms = */ 8,
	        /* .MaxCombinedImageUniforms = */ 8,
	        /* .MaxGeometryTextureImageUnits = */ 16,
	        /* .MaxGeometryOutputVertices = */ 256,
	        /* .MaxGeometryTotalOutputComponents = */ 1024,
	        /* .MaxGeometryUniformComponents = */ 1024,
	        /* .MaxGeometryVaryingComponents = */ 64,
	        /* .MaxTessControlInputComponents = */ 128,
	        /* .MaxTessControlOutputComponents = */ 128,
	        /* .MaxTessControlTextureImageUnits = */ 16,
	        /* .MaxTessControlUniformComponents = */ 1024,
	        /* .MaxTessControlTotalOutputComponents = */ 4096,
	        /* .MaxTessEvaluationInputComponents = */ 128,
	        /* .MaxTessEvaluationOutputComponents = */ 128,
	        /* .MaxTessEvaluationTextureImageUnits = */ 16,
	        /* .MaxTessEvaluationUniformComponents = */ 1024,
	        /* .MaxTessPatchComponents = */ 120,
	        /* .MaxPatchVertices = */ 32,
	        /* .MaxTessGenLevel = */ 64,
	        /* .MaxViewports = */ 16,
	        /* .MaxVertexAtomicCounters = */ 0,
	        /* .MaxTessControlAtomicCounters = */ 0,
	        /* .MaxTessEvaluationAtomicCounters = */ 0,
	        /* .MaxGeometryAtomicCounters = */ 0,
	        /* .MaxFragmentAtomicCounters = */ 8,
	        /* .MaxCombinedAtomicCounters = */ 8,
	        /* .MaxAtomicCounterBindings = */ 1,
	        /* .MaxVertexAtomicCounterBuffers = */ 0,
	        /* .MaxTessControlAtomicCounterBuffers = */ 0,
	        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	        /* .MaxGeometryAtomicCounterBuffers = */ 0,
	        /* .MaxFragmentAtomicCounterBuffers = */ 1,
	        /* .MaxCombinedAtomicCounterBuffers = */ 1,
	        /* .MaxAtomicCounterBufferSize = */ 16384,
	        /* .MaxTransformFeedbackBuffers = */ 4,
	        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
	        /* .MaxCullDistances = */ 8,
	        /* .MaxCombinedClipAndCullDistances = */ 8,
	        /* .MaxSamples = */ 4,
	    };
	    DefaultTBuiltInResource.limits = {
	        /* .nonInductiveForLoops = */ 1,
	        /* .whileLoops = */ 1,
	        /* .doWhileLoops = */ 1,
	        /* .generalUniformIndexing = */ 1,
	        /* .generalAttributeMatrixVectorIndexing = */ 1,
	        /* .generalVaryingIndexing = */ 1,
	        /* .generalSamplerIndexing = */ 1,
	        /* .generalVariableIndexing = */ 1,
	        /* .generalConstantMatrixVectorIndexing = */ 1,
	    };

		// Add a define to the source code specifiying that the shader is being compiled for vulkan
		std::string vulkanDefine = "#version 430\n#define VK_ENABLED\n";
		sourceCode = sourceCode.replace(0, 13, vulkanDefine);

	    // Specify that we are providing a compute shader
	    const EShLanguage stage = EShLangCompute;

	    // Initialize glslang if it hasn't been initialized yet
	    static bool glslangInitalized = false;
	    if(!glslangInitalized) glslangInitalized = glslang::InitializeProcess();
	    if(!glslangInitalized) throw std::runtime_error("Failed to initialize glslang");

	    // Create the shader
	    glslang::TShader shader(stage);
	    const char* sourceCString = sourceCode.c_str();
	    shader.setStrings(&sourceCString, 1);
	    shader.setEnvInput(glslang::EShSource::EShSourceGlsl, stage, glslang::EShClient::EShClientVulkan, glslang::EShTargetVulkan_1_2);
	    shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_2);
	    shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
	    shader.setEntryPoint(entryPoint.c_str());

	    // Preprocess the shader
	    DirStackFileIncluder includer; // #include preprocessor
	    std::string preprocessedCode;
	    EShMessages messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);
	    if(!shader.preprocess(&DefaultTBuiltInResource, /*default version*/ 100, EProfile::ENoProfile, false, false, messages, &preprocessedCode, includer)){
	        if(sourceCode.size() > 100) { std::cerr << "Preprocessing failed for:\n" << sourceCode.substr(0, 100) << "..." << std::endl; }
	        else { std::cerr << "Preprocessing failed for:\n"  << sourceCode << std::endl; }
	        std::cerr << shader.getInfoLog() << std::endl;
	        std::cerr << shader.getInfoDebugLog() << std::endl;
	    }
	    const char* preprocessedCString = preprocessedCode.c_str();
	    shader.setStrings(&preprocessedCString, 1);

	    // Parse the shader
	    if(!shader.parse(&DefaultTBuiltInResource, 100, false, messages)){
	        if(sourceCode.size() > 100) { std::cerr << "GLSL Parsing failed for:\n" << sourceCode.substr(0, 100) << "..." << std::endl; }
	        else { std::cerr << "GLSL Parsing failed for:\n" << sourceCode << std::endl; }
	        std::cerr << shader.getInfoLog() << std::endl;
	        std::cerr << shader.getInfoDebugLog() << std::endl;
	    }

	    // Link the shader into a program
	    glslang::TProgram program;
	    program.addShader(&shader);
	    if(!program.link(messages)){
	        if(sourceCode.size() > 100) { std::cerr << "Linking failed for:\n" << sourceCode.substr(0, 100) << "..." << std::endl; }
	        else { std::cerr << "Linking failed for:\n" << sourceCode << std::endl; }
	        std::cerr << program.getInfoLog() << std::endl;
	        std::cerr << program.getInfoDebugLog() << std::endl;
	    }

	    // Convert the program to SPIR-V
	    std::vector<uint32_t> spirV;
	    glslang::GlslangToSpv(*program.getIntermediate(stage), spirV);

		// Create Shader Module
		return context.device->createShaderModuleUnique( {{}, (uint32_t) spirV.size() * sizeof(uint32_t), spirV.data()} );
	}
};

#endif // __COMPUTE_SHADER_VULK_H__
