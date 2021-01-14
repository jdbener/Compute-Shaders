#version 430

// In order to write to a texture, we have to introduce it as image2D.
// local_size_x/y/z layout variables define the work group size.
// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
// gl_LocalInvocationID is the local index within the work group, and
// gl_WorkGroupID is the work group's index
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding = 1) buffer inBuffer
{ uint inData[]; };

layout(std430, binding = 2) buffer outBuffer
{ uint outData[]; };

#ifdef VK_ENABLED
layout(push_constant) uniform PushConstants {
    bool shouldFlip;
};
#else
uniform bool shouldFlip;
#endif

void main() {
	if(shouldFlip)
		outData[gl_GlobalInvocationID.x] = inData[1023 - gl_GlobalInvocationID.x];
	else
		outData[gl_GlobalInvocationID.x] = inData[gl_GlobalInvocationID.x];
}
