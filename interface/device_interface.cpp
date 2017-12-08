// device interface for connecting cpu to fpga using opencl code.
// initialization based on SDaccel hello world program

#include "device_interface.hpp"
#include "defs.hpp"
#include "xcl2.hpp"

// offset to the read buffer
#define READ(x) (x+2)


void check(cl_int err) {
  if (err) {
    printf("ERROR: Operation Failed: %d\n", err);
    exit(EXIT_FAILURE);
  }
}

DeviceInterface::DeviceInterface() {
  // The get_xil_devices will return vector of Xilinx Devices
  std::vector<cl::Device> devices = xcl::get_xil_devices();
  cl::Device device = devices[0];

  //Creating Context and Command Queue for selected Device
  cl::Context context(device);
  q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE);
  std::string device_name = device.getInfo<CL_DEVICE_NAME>();
  std::cout << "Found Device=" << device_name.c_str() << std::endl;

  // import_binary() command will find the OpenCL binary file created using the
  // xocc compiler load into OpenCL Binary and return as Binaries
  // OpenCL and it can contain many functions which can be executed on the
  // device.
  std::string binaryFile = xcl::find_binary_file(device_name, "device_kernel");
  cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
  devices.resize(1);
  program = cl::Program(context, devices, bins);
  // This call will extract a kernel out of the program we loaded in the
  // previous line. A kernel is an OpenCL function that is executed on the
  // FPGA. This function is defined in the interface/device_kernel.cl file.
  krnl_sha[0] = cl::Kernel(program, "hashing_kernel0");
  krnl_sha[1] = cl::Kernel(program, "hashing_kernel1");

  unsigned xcl_banks[4] = {
    XCL_MEM_DDR_BANK0,
    XCL_MEM_DDR_BANK1,
    XCL_MEM_DDR_BANK2,
    XCL_MEM_DDR_BANK3
  };

  for (int i = 0; i < BUFFER_COUNT*2; i++) {
    buffer_ext[i].flags = xcl_banks[i];
    buffer_ext[i].obj = NULL;
    buffer_ext[i].param = 0;
  }

  // first half of buffers are only read by kernel
  // second half of buffers should only be written by kernel (not atm)
  int err;
  unsigned flags = CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX;
  for (int i = 0; i < BUFFER_COUNT*2; i++) {
    if (i == BUFFER_COUNT) {
      flags = CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX;
    }

    ocl_bufs[i] = cl::Buffer(context, flags, BUFFER_SIZE, &buffer_ext[i], &err);
    if (err != CL_SUCCESS) {
      printf("Error: Failed to allocate buffer in DDR bank %d %zu\n", i, BUFFER_SIZE);
    }
  }

  for (int i = 0; i < BUFFER_COUNT*2; i++) {
    host_bufs[i] = nullptr;
  }
}

struct chunk *DeviceInterface::get_write_buffer(int active_buf) {
  int err;
  if (host_bufs[active_buf+2]) {
    q.enqueueUnmapMemObject(ocl_bufs[READ(active_buf)], host_bufs[READ(active_buf)], NULL, NULL);
  }
  
  // CL_MAP_WRITE_INVALIDATE_REGION makes it such that the returned memory
  // region doesnt have to be up to date. We will just overwrite it so it doesn't matter.
  host_bufs[active_buf] = (struct chunk *) q.enqueueMapBuffer(ocl_bufs[active_buf], CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, 0, BUFFER_SIZE, NULL, NULL, &err);
  check(err);
  return (struct chunk *) host_bufs[active_buf];
}


struct chunk *DeviceInterface::run_fpga(int num_chunks, int active_buf) {
  cl_int err;

  

  // unmap input buffer
  q.enqueueUnmapMemObject(ocl_bufs[active_buf], host_bufs[active_buf], NULL, NULL);

  q.finish();

  
  //set the kernel Arguments
  int narg = 0;
  krnl_sha[active_buf].setArg(narg++, ocl_bufs[active_buf]);
  krnl_sha[active_buf].setArg(narg++, ocl_bufs[READ(active_buf)]);
  krnl_sha[active_buf].setArg(narg++, num_chunks);


  //Launch the Kernel

  q.enqueueTask(krnl_sha[active_buf]);

  // previous computations result
  host_bufs[READ(1-active_buf)] = q.enqueueMapBuffer(ocl_bufs[READ(1-active_buf)], CL_TRUE, CL_MAP_READ, 0, BUFFER_SIZE, NULL, NULL, &err);
  check(err);

  // possibly remove this to allow for multiple kernels to run simultaneously
  q.finish();

  return (struct chunk *) host_bufs[READ(1-active_buf)];
}

struct chunk *DeviceInterface::read_last_result(int active_buf) {

  int err;
  q.enqueueUnmapMemObject(ocl_bufs[READ(active_buf)], host_bufs[READ(active_buf)], NULL, NULL);

  host_bufs[READ(1-active_buf)] = q.enqueueMapBuffer(ocl_bufs[READ(1-active_buf)], CL_TRUE, CL_MAP_READ, 0, BUFFER_SIZE, NULL, NULL, &err);
  check(err);

  return (struct chunk *) host_bufs[READ(1-active_buf)];
}

void DeviceInterface::unmap_last_result(int active_buf) {
  q.enqueueUnmapMemObject(ocl_bufs[READ(1-active_buf)], host_bufs[READ(1-active_buf)]);
  q.finish();
}


