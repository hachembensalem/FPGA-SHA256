# PineappleExpress

## Description
System that will (hopefully) accelerate SHA-256 on an FPGA.

Amazon F1 instances will be used to test the system on real hardware. CPU and FPGA will communicate through DRAM connected to PCIe with ~~Amazon's EDMA driver~~ an XDMA driver through OpenCL. Both host code and FPGA kernel will be written in OpenCL C/C++ and compiled using Xilinx's SDAccel tool. 

A double buffer will be used to allow the CPU and FPGA to write and read buffers in parallel.

## Documentation
Code is mostly self-documenting.

Notes from daily and weekly scrum together with other various information can be found in our [blog](https://pineappleblogg.wordpress.com/).

## Building PineappleExpress
In short... From a shell, switch into the root PineappleExpress directory and run:

```
make clean
make all

```

## Run PineappleExpress
Use .PineappleExpress/main -h for more help on how to run the program.

```
Example:
./main -d -f foo.txt -s 100
```
```
Useful options:
-f  Specify which file to read. The program will read password.txt if the flag is not specified.
-s  Specify how many Megabyte to read. The whole file will be read if the flag is not specified.
-d  Activates debug mode.
```

## Code standard

### Names

Snake case for everything except class names. Class names use camel case

### Spacing

Indentation level is two spaces.

### Brackets

```

void foo() {
  if (!coffee) {
    make_coffee();
  } else {
    make_coffee();
  }
}

```


### Pointers

```

int *bar

```

NOT

```

int* bar

```

### Pull requests

Merge is used for pull requests.


### Other
Follow the Google c++ style guide
