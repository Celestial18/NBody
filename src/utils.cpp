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
    if(gpu_available()){
        err |= clGetDeviceIDS(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr); 
    } else {
        err |= clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
    }
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

void cleanup_opencl(){
        
        clReleaseKernel(kernel_force);
        clReleaseKernel(kernel_integrate);


        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
}


bool gpu_available(){
    cl_uint num_platforms; 
    cl_int err = clGetPlatformIDs(0, nullptr, &num_platforms); 
    if (err != CL_SUCCESS || num_platforms == 0){
        std::cout << "No platforms for OpenCL use found. Error: "<< err << std::endl; 
        return 1 
    }

    cl_platform_id* platforms = new cl_platform_id[num_platforms]; 

    err = clGetPlatformIDs(num_platforms, platforms, nullptr); 
    if (err != CL_SUCCES){
        std::cout << "Failed to get platform ID's. Error: " << err << std::endl; 
        return false; 
    }

    bool gpu_found = false 

    for (cl_uint i=0; i<num_platforms; i++){
        cl_uint num_devices; 

        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices); 
        if (err!= CL_SUCCESS && num_devices > 0){
           //Allocate memory for devices
            cl_device_id* devices = new cl_device_id[num_devices];
            err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, num_devices, devices, nullptr);
            if (err == CL_SUCCESS) {
                gpu_found = true;
                std::cout << "Found " << num_devices << " GPU device(s) on platform " << i << ":\n";

                //Get device details
                for (cl_uint j = 0; j < num_devices; ++j) {
                    char device_name[128];
                    clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
                    std::cout << "  GPU Device " << j << ": " << device_name << std::endl;
                }
                delete[] devices;
            }
        }
    }
    if (!gpu_found) {
        std::cout << "No GPU devices found." << std::endl;
    }

    //Cleanup
    delete[] platforms;
    return gpu_found;
}
