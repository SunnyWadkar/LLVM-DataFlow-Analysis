# LLVM-DataFlow-Analysis
Available Expressions and Liveness Analysis

1. To build the available and liveness run the build_prj bash script in the Dataflow folder:

> ./build_prj.sh

The files liveness.so and available.so will be generated.

**Instructions: The liveness pass requires the variable names. The IR may contain values which are actually registers.
These registers do not have values and hence liveness pass may not work for such values.**
To make it work, instnamer pass needs to be run on the byte code file before running the liveness pass. The command for that
pass is as follows:

> opt -instnamer <input_byte_code> -o <output_byte_code>
Example:
> opt -instnamer ./tests/liveness-test-m2r.bc  -o ./tests/liveness-test-m2r.bc 

The build script already does this for the provided example file. So, this step is not required for the provided test file.

2. To run the pass, run the following command in the Dataflow folder:

> opt -load ./available.so -available ./tests/available-test-m2r.bc -o /dev/null
> opt -load ./liveness.so -liveness ./tests/liveness-test-m2r.bc -o /dev/null
