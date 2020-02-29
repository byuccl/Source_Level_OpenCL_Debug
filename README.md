
## Install and Setup
Follow instructions on the ROSE compiler github page to install ROSE. https://github.com/rose-compiler/rose/wiki

$ROSE_HOME needs to be set according to the instructions for installing ROSE.

Copy DefaultTranslator.c to $ROSE_HOME/src/exampleTranslators/defaultTranslator/

Run “make install” in $ROSE_HOME/build/exampleTranslators/defaultTranslator/

## Instrumenting a Design with Debug Instructions

### Modifying the Kernel File

Add an "hlsDebugConfig.txt" file to the directory with the OpenCL file. This file contains kernel names, and a buffer size. An example would be:

KERNEL_NAME fetch

KERNEL_NAME fft1d

BUFFER_SIZE 512

The buffer size needs to be a power of two for the calculation offsets into the buffer in each thread. If you need multiple buffer sizes you can manually change the uses in the new OpenCL file .

Run “python3 trace_selection_gui/hlstool.py” with the OpenCL file as an argument. 
 
The script modifies the OpenCL code to look like C code, and replace the ".cl" extension with a ".c" extension. If then calls the defaulTranslator ROSE transformation. The transformation tool collects all of the assignment variables in the kernels, and records them and their unique identifiers to a file called "hlsDebugVariableList.txt" in the directory with the OpenCL file. After recording the data, it calls a the python gui. In the gui the user can select variables to record, and select the "Confirm" button in the top right corner. 
 
After the user confirms their selection, the selection is recorded to a file called "hlsDebugSelectedVar.txt" in the directory with the OpenCL file. The ROSE transformation then reads the "hlsDebugSelectedVar.txt" file and inserts the recording instructions after each of the selected variable assignments.
  

### Modifying the Host Code

The host file needs to be modified to retrieve the trace buffer. An extra argument needs to be added to the kernel, which will contain the trace buffer. The size of the trace buffer is the size of each threads trace buffer times the number of threads. Each device gets its own trace buffer. The variable type of the trace buffer is dependent upon the type of the trace buffer in the kernel. By default the trace buffer is of type "long." This can be manually changed.

For example, in the Intel design test Time-Domain FIR Filter, the following code would be added:

In global space:

`cl_mem dbgTraceBuffer;`

`cl_float *traceBuffer;`

In function `tdfirFPGA` before `clEnqueueNDRangeKernel`:

`dbgTraceBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_CHANNEL_2_INTEL_FPGA. sizeof(float) * 4096, NULL, &err);`

`traceBuffer = (float*) alignedMalloc(sizeof(float) * 4096);`

`err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &dbgTraceBuffer);`

In function `tdfirFPGA` after `clEnqueueNDRangeKernel`:

`err = clEnqueueReadBuffer(queue, dbgTraceBuffer, CL_TURE, 0, sizeof(float) * 4096, traceBuffer, 0, NULL, &myEvent);`


After retrieving the trace buffer kernel argument, the buffer needs to be dumped to a file. The layout is as follows, all data separated by a space:

NUM_DEVICES NUM_THREADS TB_SIZE DEVICE_ID TRACE_BUFFER_FOR_CURRENT_DEVICE DEVICE_ID  TRACE_BUFFER_FOR_CURRENT_DEVICE ...

For example if there were 2 devices, 1 thread per device, a trace buffer size of 8, the DEVICE_IDs were 101 and 102, and the values in the buffers were 1-8, then the layout would be as follows:

2 1 8 101 1 2 3 4 5 6 7 8 102 1 2 3 4 5 6 7 8

## Compiling the OpenCL Design

The design is compiled as normal using aocl in Intel OpenCL SDK and Quartus versions 18.1

## Running the OpenCL Design

Run as a normal OpenCL design.

## Viewing the Debug Trace

To run the HTML-based trace viewer:

  - Copy the produced trace buffer output file to `trace_viewer/tb.txt`
  - Copy the original configuration file to `trace_viewer/hlsDebugVariableList.txt`
  - Navigate to the `trace_viewer` directory
  - Run `./tb_to_html.py tb.txt hlsDebugVariableList.txt`
  - Browse to [http://localhost:5000/](http://localhost:5000/)
  
  
## Limitations

The python portion of the tool currently only supports a single OpenCL file.

The user should only select variables in the top level kernel, as support for recording variables in other called functions has been disabled due to excessive overhead in the generated RTL design. If the user  wants to record variables in called functions they should be inlined first.

The OpenCL Debug Trace viewer requires the device IDs to not be less than (2^28) + 1 or 268435457

One of the biggest limitations is the type of OpenCL directives and types that are currently supported. The python script "hlstool.py" replaces many of the OpenCL directives with commented out versions. For example, `__kernel` is replaced with `\*__kernel*\` and the same occurs for multiple OpenCL directives, which is shown in the "hlstool.py" file. After the ROSE transformation finished, the python script removes the comments from these directives, and adds directives to the new trace buffer argument. If there are other OpenCL types or directives, they need to be added as an extern argument by the user before running the tool. This will allow the tool to treat the extern OpenCL types or directives as normal C code. The user will need to delete their changes in the new generated ".cl" file.   

## Testing

The debugging environment has been tested five Intel OpenCL benchmarks from: 

https://www.intel.com/content/www/us/en/programmable/products/design-software/embedded-software-developers/opencl/support.html#design-examples

using the Intel OpenCL SDK and Quartus, versions 18.1, on a Xeon E6-2699 processor with a Stratix 10 SX D5005 FPGA attached over PCIe. The benchmarks we tested were Compute Score, Finite Difference Computation 3D, FFT 1D, Multithread Vector Operation, Time-Domain FIR Filter, and Vector Addition. 

Other tests that did not work were Loopback, Mandelbrot, and Sobel. They did not work due to the limitations of the ROSE compiler, or because of the extensive use of OpenCL constructs throughout the code that could not be easily replaced before running it through the ROSE compiler.
