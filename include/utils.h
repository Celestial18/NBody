#pragma once
#include <CL/cl.h>
#include <string>

// OpenCL handles
extern cl_context context;
extern cl_command_queue queue;
extern cl_program program;
extern cl_kernel kernel_force;
extern cl_kernel kernel_integrate;

// Load kernel file from disk
std::string load_kernel_source(const std::string& filename);

// Initialize OpenCL (context, queue, kernels)
void init_opencl(const std::string& cl_file);

void cleanup_opencl(); 