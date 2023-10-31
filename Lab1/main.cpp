#include <CL/cl.h>
#include <iostream>

#pragma warning (disable: 4703)

cl_program getProgram(cl_context context, cl_device_id device, const char* path);
void task1(cl_context context, cl_device_id device, cl_command_queue queue);
void task2(cl_context context, cl_device_id device, cl_command_queue queue);

int main() {
    cl_uint platformCount = 0;
    clGetPlatformIDs(0, nullptr, &platformCount);

    cl_platform_id platform = nullptr;

    cl_platform_id *platforms = new cl_platform_id[platformCount];
    clGetPlatformIDs(platformCount, platforms, nullptr);

    for (cl_uint i = 0; i < platformCount; ++i) {
        char platformName[128];
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME,
                          128, platformName, nullptr);
        std::cout << platformName << std::endl;
    }

    platform = platforms[0];
    delete[] platforms;

    cl_context_properties properties[3] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0
    };
    
    cl_context context = clCreateContextFromType(
        (platform == nullptr) ? nullptr : properties,
        CL_DEVICE_TYPE_GPU,
        nullptr,
        nullptr,
        nullptr
    );

    size_t size = 0;
    clGetContextInfo(
        context,
        CL_CONTEXT_DEVICES,
        0,
        nullptr,
        &size
    );

    cl_device_id device;

    if (size > 0) {
        cl_device_id *devices = (cl_device_id*)alloca(size);
        
        clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            size,
            devices,
            nullptr
        );

        device = devices[0];
    }

    cl_command_queue queue = clCreateCommandQueueWithProperties(
        context,
        device,
        0,
        nullptr
    );

    task1(context, device, queue);
    task2(context, device, queue);

    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}

cl_program getProgram(cl_context context, cl_device_id device, const char *path) {
    cl_program program;
    FILE *fp;
    size_t programSize, logSize;
    char *buffer, *log;
    int err;

    fopen_s(&fp, path, "rb");
    if (fp == nullptr) {
        perror("file not found");
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    programSize = ftell(fp);
    rewind(fp);
    buffer = new char[programSize + 1];
    buffer[programSize] = '\0';
    fread(buffer, sizeof(char), programSize, fp);
    fclose(fp);

    program = clCreateProgramWithSource(
        context,
        1,
        (const char**)&buffer,
        &programSize,
        &err
    );

    if (err < 0) {
        perror("Couldn't create the program");
        return nullptr;
    }
    delete[] buffer;

    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err < 0) {
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        log = new char[logSize + 1];
        log[logSize] = '\0';
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize + 1, log, nullptr);
        printf("%s\n", log);
        delete[] log;
        return nullptr;
    }

    return program;
}

void task1(cl_context context, cl_device_id device, cl_command_queue queue) {
    printf("Start task1\n");
    cl_program program = getProgram(context, device, "task1.cl");
    cl_kernel kernel = clCreateKernel(program, "task1", nullptr);

    size_t count = 10;
    size_t group = 5;
    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &count, &group, 0, nullptr, nullptr);

    clFinish(queue);

    clReleaseProgram(program);
    clReleaseKernel(kernel);
}

void task2(cl_context context, cl_device_id device, cl_command_queue queue) {
    printf("Start task2\n");
    cl_program program = getProgram(context, device, "task2.cl");
    cl_kernel kernel = clCreateKernel(program, "task2", nullptr);

    const size_t arraySize = 10;
    int data[arraySize];
    int result[arraySize];
    printf("Initial array: ");
    for (size_t i = 0; i < arraySize; i++) {
        data[i] = i + 1;
        result[i] = 0;
        printf("%d ", data[i]);
    }

    cl_mem input = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * arraySize, nullptr, nullptr);
    cl_mem output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * arraySize, nullptr, nullptr);

    clEnqueueWriteBuffer(queue, input, CL_TRUE, 0, sizeof(int) * arraySize, data, 0, nullptr, nullptr);

    size_t count = arraySize;
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);

    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &count, nullptr, 0, nullptr, nullptr);

    clFinish(queue);

    clEnqueueReadBuffer(queue, output, CL_TRUE, 0, sizeof(int) * count, result, 0, nullptr, nullptr);

    printf("\nResult: ");
    for (size_t i = 0; i < arraySize; i++) {
        printf("%d ", result[i]);
    }

    clReleaseMemObject(input);
    clReleaseMemObject(output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
}
