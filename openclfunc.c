#include "openclfunc.h"
#include "CL/cl.h"

#include "windows.h"
#include <stdio.h>
#include "errorf.h"
#include "portables.h"

cl_int report_and_mark_devices(cl_device_id *devices, cl_uint num_devices, int *a_cpu, int *a_gpu, int *an_accelerator);
char *stringfromfile(char *filename);

extern char *PrgDir;

static cl_kernel ScalechunkbilinKernel = 0;
static cl_program ScalechunkbilinProgram = 0;
static cl_context ScalechunkbilinContext = 0;
static cl_device_id ScalechunkbilinDevice = 0;

int initChunkBiLinKernel(void) {
	cl_int err;
	cl_device_id device_id;
	int gpu = 1;
	
	cl_platform_id platform;
	cl_uint num_platforms;
	err = clGetPlatformIDs(1, &platform, &num_platforms);

	if (err != CL_SUCCESS) {
		errorf("Error: Failed to get a platform id!\n");
		return EXIT_FAILURE;
	}

/*
	size_t returned_size = 0;
	cl_char platform_name[1024] = {0}, platform_prof[1024] = {0}, platform_vers[1024] = {0}, platform_exts[1024] = {0};
	err  = clGetPlatformInfo(platform, CL_PLATFORM_NAME,       sizeof(platform_name), platform_name, &returned_size);
	err |= clGetPlatformInfo(platform, CL_PLATFORM_VERSION,    sizeof(platform_vers), platform_vers, &returned_size);
	err |= clGetPlatformInfo(platform, CL_PLATFORM_PROFILE,    sizeof(platform_prof), platform_prof, &returned_size);
	err |= clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS, sizeof(platform_exts), platform_exts, &returned_size);

	if (err != CL_SUCCESS) {
	errorf("Error: Failed to get platform infor!\n");
	return EXIT_FAILURE;
	}

	errorf("\nPlatform information\n");
	errorf("");
	errorf("Platform name:       %s\n", (char *)platform_name);
	errorf("Platform version:    %s\n", (char *)platform_vers);
	errorf("Platform profile:    %s\n", (char *)platform_prof);
	errorf("Platform extensions: %s\n", ((char)platform_exts[0] != '\0') ? (char *)platform_exts : "NONE");
*/

	// Get all available devices (up to 4)

	cl_uint num_devices;
	cl_device_id devices[4];
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 4, devices, &num_devices);

	if (err != CL_SUCCESS) {
		errorf("Failed to collect device list on this platform!\n");
		return EXIT_FAILURE;
	}

	int a_cpu = -1, a_gpu = -1, an_accelerator = -1;
	err = report_and_mark_devices(devices, num_devices, &a_cpu, &a_gpu, &an_accelerator);

	if (err != CL_SUCCESS) {
	errorf("Failed to report information about the devices on this platform!\n");
	return EXIT_FAILURE;
	}
	
	if (gpu == 0) {     // No accelerator or gpu, just cpu
		if (a_cpu == -1) {
		  errorf("No cpus available, weird...\n");
		  return EXIT_FAILURE;
		}
		device_id = devices[a_cpu];
		errorf("There is a cpu, using it\n");
	} else {              // Trying to find a gpu, or if that fails, an accelerator 
		if (a_gpu != -1) { // There is a gpu in our platform
		  device_id = devices[a_gpu];
//		  errorf("Found a gpu, using it\n");
		} else if (an_accelerator != -1) {
		  device_id = devices[an_accelerator];
		  errorf("No gpu but found an accelerator, using it\n");
		} else {
		  errorf("No cpu, no gpu, nor an accelerator... where am I running???\n");
		  return EXIT_FAILURE;
		}
	}
	
	cl_context context;                 // compute context
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err); //! devices could be expanded

	if (!context) {
		errorf("Error: Failed to create a compute context!\n");
		return EXIT_FAILURE;
	}

	cl_program program;                 // compute program
	
	const char *clSourceFile = "openclkernels.cl";
    char source_path[MAX_PATH*4];
	sprintf(source_path, "%s\\%s", PrgDir, clSourceFile);
	
    size_t program_length;
	
    char *source = stringfromfile(source_path);
	
	if (source == NULL) {
		errorf("source is null");
		return EXIT_FAILURE;
	}
	
	program = clCreateProgramWithSource(context, 1, (const char **) &source, NULL, &err);
	
	free(source);

	if (!program) {
		errorf("Error: Failed to create compute program!\n");
		return EXIT_FAILURE;
	}
  
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[10000];
		errorf("Error: Failed to build program executable!\n");

		// See page 98...
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		errorf("len: %llu", len);
		errorf("%s\n", buffer);
		exit(1);
	}

	cl_kernel kernel;                   // compute kernel
	char *kernel_name = "ScaleBmChunkBiLinKer";
	kernel = clCreateKernel(program, kernel_name, &err);

	if (!kernel || err != CL_SUCCESS) {
		errorf("Error: Failed to create compute kernel!\n");
		exit(1);
	}
	ScalechunkbilinKernel = kernel;
	ScalechunkbilinProgram = program;
	ScalechunkbilinContext = context;
	ScalechunkbilinDevice = device_id;
	
	return 0;
	
	
}

void freeChunkBiLinKernel(void) {
	cl_kernel kernel = ScalechunkbilinKernel;
	cl_program program = ScalechunkbilinProgram;
	cl_context context = ScalechunkbilinContext;
	
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseContext(context);
}

int ScaleBmChunkBiLinCL(unsigned char **from, unsigned char **to, const unsigned long n, const unsigned long x1, const unsigned long y1, const unsigned long x2, const unsigned long y2, const unsigned long xspos, const unsigned long yspos, const double zoom) {
	cl_int err;
	static int flag = 1;
	
	if (flag) {
		if (!initChunkBiLinKernel()) {
			flag = 0;
		} else {
			errorf("initChunkBiLinKernel failed");
			return 1;
		}
	}
	
	cl_kernel kernel = ScalechunkbilinKernel;
	cl_program program = ScalechunkbilinProgram;
	cl_context context = ScalechunkbilinContext;
	cl_device_id device_id = ScalechunkbilinDevice;
	
	unsigned long i = 0;
	
	cl_command_queue commands;
	cl_queue_properties *properties = NULL;
	commands = clCreateCommandQueueWithProperties(context, device_id, properties, &err);
	
	if (!commands) {
		errorf("Error: Failed to create a command queue!\n");
		return EXIT_FAILURE;
	}

	if (err != CL_SUCCESS) {
		errorf("Error: Failed to write to source array!\n");
		exit(1);
	}
	unsigned long long endpos = x2*y2;

	err = 0;
	err |= clSetKernelArg(kernel, 2, sizeof(unsigned long), &x1);
	err |= clSetKernelArg(kernel, 3, sizeof(unsigned long), &y1);
	err |= clSetKernelArg(kernel, 4, sizeof(unsigned long), &x2);
	err |= clSetKernelArg(kernel, 5, sizeof(unsigned long), &y2);
	err |= clSetKernelArg(kernel, 6, sizeof(unsigned long), &xspos);
	err |= clSetKernelArg(kernel, 7, sizeof(unsigned long), &yspos);
	err |= clSetKernelArg(kernel, 8, sizeof(double), &zoom);
	
	if (err != CL_SUCCESS) {
		errorf("Error: Failed to set kernel arguments! %d\n", err);
		exit(1);
	}
//errorf("spot 9");
	
	size_t local_work_sizes[2] = {1, 64};                       // local domain size for our calculation
//	err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &local_work_sizes[1], NULL);

	if (err != CL_SUCCESS) {
		errorf("Error: Failed to retrieve kernel work group info! %d\n", err);
		exit(1);
	}
	
	size_t size1 = local_work_sizes[1];
	
	size_t global_work_sizes[2] = {y2, size1*(x2/size1 + !!(x2%size1))};                      // global domain size for our calculation
//	size_t global_work_sizes[2] = {1, 32};                      // global domain size for our calculation


	cl_mem *dx = malloc(sizeof(cl_mem)*n);                       // device memory used for the input array x
	cl_mem *dy = malloc(sizeof(cl_mem)*n);                       // device memory used for the input/output array y
	for (i = 0; i < n; i++) {
		dx[i] = 0;
		dy[i] = 0;
	}
	for (i = 0; i < n; i++) {
		errorf("creating buffers: %lu / %lu", i+1, n);
		//dx[i] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * 4 * x1 * y1, from[i], &err);
		//dy[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(char) * 4 * x2 * y2, to[i], &err);
		dx[i] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(char) * 4 * x1 * y1, NULL, &err);
		dy[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(char) * 4 * x2 * y2, NULL, &err);

		if (!dx[i] || !dy[i]) {
			errorf("Error: Failed to allocate device memory -- i: %lu -- err: %u\n", i, err);
			errorf("Flags: CL_INVALID_CONTEXT: %llu", CL_INVALID_CONTEXT);
			errorf("Flags: CL_INVALID_VALUE: %llu", CL_INVALID_VALUE);
			errorf("Flags: CL_INVALID_BUFFER_SIZE : %llu", CL_INVALID_BUFFER_SIZE);
			errorf("Flags: CL_INVALID_HOST_PTR : %llu", CL_INVALID_HOST_PTR);
			errorf("Flags: CL_MEM_OBJECT_ALLOCATION_FAILURE : %llu", CL_MEM_OBJECT_ALLOCATION_FAILURE);
			errorf("Flags: CL_OUT_OF_RESOURCES : %llu", CL_OUT_OF_RESOURCES);
			errorf("Flags: CL_OUT_OF_HOST_MEMORY  : %llu", CL_OUT_OF_HOST_MEMORY);
			errorf("vals -- n: %lu, x1: %lu, x2: %lu, y1: %lu , y2: %lu, xspos: %lu, yspos: %lu, zoom: %lf\n", n, x1, x2, y1, y2, xspos, yspos, zoom);
			exit(1);
		}
	
		err  = clEnqueueWriteBuffer(commands, dx[i], CL_TRUE, 0, sizeof(char) * 4 * x1 * y1, from[i], 0, NULL, NULL);
		err |= clEnqueueWriteBuffer(commands, dy[i], CL_TRUE, 0, sizeof(char) * 4 * x2 * y2, to[i], 0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("Error: Failed to write to source array!\n");
			exit(1);
		}
	}
	
	for (i = 0; i < n; i++) {
		err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &dx[i]);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &dy[i]);
		
		if (err != CL_SUCCESS) {
			errorf("Error: Failed to set kernel arguments! %d\n", err);
			exit(1);
		}
		
		err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global_work_sizes, local_work_sizes, 0, NULL, NULL);
	  
		if (err) {
			errorf("Error: Failed to execute kernel: %d\n", err);
			return EXIT_FAILURE;
		}
	}
	
	clFinish(commands);
  
	for (i = 0; i < n; i++) {
		err = clEnqueueReadBuffer(commands, dy[i], CL_TRUE, 0, sizeof(char) * 4 * x2 * y2, to[i], 0, NULL, NULL );  

		if (err != CL_SUCCESS) {
			errorf("Error: Failed to read output array! %d\n", err);
			exit(1);
		}
	}
  
	for (i = 0; i < n; i++) {
		if (dx[i])
			clReleaseMemObject(dx[i]);
		if (dy[i])
			clReleaseMemObject(dy[i]);
	}
	clReleaseCommandQueue(commands);

	return 0;
}

cl_int report_and_mark_devices(cl_device_id *devices, cl_uint num_devices, int *a_cpu, int *a_gpu, int *an_accelerator) {
  int i, type_name_index = 0;
  cl_int err = 0;
  size_t returned_size;
  size_t max_workgroup_size = 0;
  cl_uint max_compute_units = 0, vec_width_char = 0, vec_width_short = 0, max_constant_args = 0;
  cl_uint vec_width_int = 0, vec_width_long = 0, vec_width_float = 0, vec_width_double = 0;
  char vendor_name[1024] = {0}, device_name[1024] = {0}, device_version[1024] = {0}, c_version[1024] = {0};
  cl_ulong global_mem_size, max_constant_buffer, mem_alloc;
  cl_device_type device_type;
  char type_names[3][27]={"CL_DEVICE_TYPE_CPU        " , "CL_DEVICE_TYPE_GPU        " , "CL_DEVICE_TYPE_ACCELERATOR"};
  
  for (i=0;i<num_devices;i++) {
    err = clGetDeviceInfo(devices[i], CL_DEVICE_TYPE,                          sizeof(device_type),    &device_type,        &returned_size);
/*
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR,                        sizeof(vendor_name),    vendor_name,         &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_NAME,                          sizeof(device_name),    device_name,         &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_VERSION,                       sizeof(device_version), device_version,      &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_MAX_WORK_GROUP_SIZE,           sizeof(size_t),         &max_workgroup_size, &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS,             sizeof(cl_uint),        &max_compute_units,  &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE,               sizeof(cl_ulong),       &global_mem_size,    &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,   sizeof(cl_uint),        &vec_width_char,     &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,  sizeof(cl_uint),        &vec_width_short,    &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,    sizeof(cl_uint),        &vec_width_int,      &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,   sizeof(cl_uint),        &vec_width_long,     &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,  sizeof(cl_uint),        &vec_width_float,    &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint),        &vec_width_double,   &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_OPENCL_C_VERSION,              sizeof(c_version),      c_version,           &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE ,     sizeof(cl_ulong),       &max_constant_buffer,           &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE ,           sizeof(cl_ulong),       &mem_alloc,           &returned_size);
    err|= clGetDeviceInfo(devices[i], CL_DEVICE_MAX_CONSTANT_ARGS ,           sizeof(cl_uint),         &max_constant_args,           &returned_size);
*/
    
    if (err != CL_SUCCESS) {
      errorf("Error: Failed to retrieve device info!\n");
      return EXIT_FAILURE;
    }

    if (device_type == CL_DEVICE_TYPE_CPU) {
      *a_cpu = i;
      type_name_index = 0;
    }

    if (device_type == CL_DEVICE_TYPE_GPU) {
      *a_gpu = i;
      type_name_index = 1;
    }

    if (device_type == CL_DEVICE_TYPE_ACCELERATOR) {
      *an_accelerator = i;
      type_name_index = 2;
    }

/*
    errorf("\nDevice information:\n");
    errorf("");
    errorf("Type:               %s\n", type_names[type_name_index]);
    errorf("Vendor:             %s\n", vendor_name);
    errorf("Device:             %s\n", device_name);
    errorf("Version:            %s\n", device_version);
    errorf("C Version:            %s\n", c_version);
    errorf("Max workgroup size: %d\n", (int)max_workgroup_size);
    errorf("Max compute units:  %d\n", (int)max_compute_units);
    errorf("Global mem size:    %ld\n", (long)global_mem_size);
    errorf("Max constant buffer size:    %ld\n", (long)max_constant_buffer);
    errorf("CL_DEVICE_MAX_MEM_ALLOC_SIZE:    %ld\n", (long)mem_alloc);
    errorf("CL_DEVICE_MAX_CONSTANT_ARGS:    %ld\n", (long)max_constant_args);
    errorf("");
    
    errorf("\nPreferred vector widths by type:\n");
   
    errorf("");
    errorf("Vector char:  %d\n",   (int)vec_width_char);
    errorf("Vector short: %d\n",   (int)vec_width_short);
    errorf("Vector int:   %d\n",   (int)vec_width_int);
    errorf("Vector long:  %d\n",   (int)vec_width_long);
    errorf("Vector float: %d\n",   (int)vec_width_float);
    errorf("Vector dble:  %d\n",   (int)vec_width_double);
    errorf("");
    errorf("\n");
*/
  }
  return err;
}

char *stringfromfile(char *filename) {
	char *buffer = 0;
	long length;
	FILE *f = MBfopen (filename, "rb");

	if (f) {
	  fseek(f, 0, SEEK_END);
	  length = ftell(f);
	  fseek(f, 0, SEEK_SET);
	  buffer = malloc(length+1);
	  if (buffer) {
		fread(buffer, 1, length, f);
		buffer[length] = '\0';
	  }
	  fclose (f);
	}
	return buffer;
}