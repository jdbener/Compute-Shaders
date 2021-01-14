/*
	Thanks https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2
*/
//#define USE_GL 	// Uncomment to use OpenGL instead of Vulkan

#ifdef USE_GL
#include "OpenGL/BoilerPlate.hpp"
#include "OpenGL/DictionaryComputeBuffer.hpp"
#else
#include "Vulkan/ComputeShader.hpp"
#include "Vulkan/DictionaryComputeBuffer.hpp"
#endif

#include <iomanip>

using namespace std;

int main(){
	Dictionary d;
	d["bob"] = 21.0;
	d["england"] = {"c", "i", "t", "y"};
	d["paul"] = "is my";

	std::cout << (char) d["england"][3][0] << std::endl;
	std::cout << (int) float(d["bob"]) << std::endl;
	std::cout << d.has("bob") << " - " << d.has("texas") << std::endl;




	// Create a connection to the shader file
	ifstream multiplyFile("src/multiplyShader.glsl");
	ifstream reverseFile("src/reverseShader.glsl");

	// Initalize the data we will pass to the GPU
	vector<unsigned int> data;
	for(int i = 1; i <= 1024; i++)
		data.push_back(i);


#ifdef USE_GL // OpenGL Code
	// Create the OpenGL Context
	initOpenGL();
	cout << "Initaliztion Sucessful!" << endl;

	// Test Dictionary Buffer
	DictionaryComputeBuffer buf(3);
	buf.bindDictionary(d);
	Dictionary d2 = buf.getData();
	std::cout << (char) d2["england"][3][0] << std::endl;
	std::cout << (int) float(d2["bob"]) << std::endl;



	{
		// Create Buffers
		ComputeBuffer inBuffer(1, data);

		//ComputeBuffer outBuffer(2, data.size() * sizeof(data[0]));
		StructuredComputeBuffer outBuffer(2);
		outBuffer.addField<int>(data.size());
		outBuffer.commit();

		// Multiply our data
		ComputeShader multiply(multiplyFile);
		multiply.dispatch(1024 / 32);

		// Reverse our data
		ComputeShader reverse(reverseFile);
		reverse.setParameter("shouldFlip", true);
		reverse.dispatch(1024 / 32);

		// Pull the data from the outbuffer
		outBuffer.getData(data);

		cout << (reverse.getParameter<bool>("shouldFlip") ? "true" : "false") << endl;
	}

	// Print out the start of the data
	cout << endl << "Data:";
	for(int i = 0; i < 30; i++){
		if(i % 10 == 0)	cout << endl;
		cout << left << setw(6) << setprecision(2) << data[i];
	}

	cout << endl;

	glfwTerminate();
	multiplyFile.close();
	reverseFile.close();
	cout << endl << "Terminatation Sucessful!" << endl;
#else // VulkanCode
	{
		// Create the Vulkan Context
		VulkanContext c = initVulkan();
		cout << "Initaliztion Sucessful!" << endl;


		// Test Dictionary Buffer
		DictionaryComputeBuffer buf(c, 3);
		buf.bindDictionary(d);
		Dictionary d2 = buf.getData();
		std::cout << (char) d2["england"][3][0] << std::endl;
		std::cout << (int) float(d2["bob"]) << std::endl;



		{
			// Create Buffers
			ComputeBuffer inBuffer(c, 1, data);
			ComputeBuffer outBuffer(c, 2, data.size() * sizeof(data[0]));

			// // Multiply our data
			ComputeShader multiply(c, multiplyFile);
			multiply.bindComputeBuffer(inBuffer);
			multiply.dispatch(1024 / 32);

			// Reverse our data
			ComputeShader reverse(c, reverseFile);
			reverse.bindComputeBuffer(inBuffer);
			reverse.bindComputeBuffer(outBuffer);
			struct PushConstants {
				bool shouldFlip;
			} gpuData;
			gpuData.shouldFlip = true;
			reverse.setPushConstants(gpuData);
			reverse.dispatch(1024 / 32);

			// Pull the data from the outbuffer
			outBuffer.getData(data);
		}

		// Print out the start of the data
		cout << endl << "Data:";
		for(int i = 0; i < 30; i++){
			if(i % 10 == 0)	cout << endl;
			cout << left << setw(6) << setprecision(2) << data[i];
		}

		cout << endl;
	}
	multiplyFile.close();
	reverseFile.close();
	cout << endl << "Terminatation Sucessful!" << endl;
#endif
	return 0;
}
