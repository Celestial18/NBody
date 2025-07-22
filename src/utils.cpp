#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel kernel_force;
cl_kernel kernel_integrate;

std::string load_kernel_source(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open kernel source: " << filename << "\n";
        exit(1);
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

void init_opencl(const std::string& cl_file) {
    cl_platform_id platform;
    cl_device_id device;
    cl_int err;

    err = clGetPlatformIDs(1, &platform, nullptr);
    err |= clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);

    std::string source = load_kernel_source(cl_file);
    const char* src = source.c_str();
    size_t length = source.size();
    program = clCreateProgramWithSource(context, 1, &src, &length, &err);
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);

    if (err != CL_SUCCESS) {
        // Print error log if compilation failed
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        std::cerr << "OpenCL build log:\n" << log.data() << std::endl;
        exit(1);
    }

    kernel_force = clCreateKernel(program, "compute_forces", &err);
    kernel_integrate = clCreateKernel(program, "integrate_bodies", &err);
}
