Summary
=======

This is an [MPICH](https://www.mpich.org/) based Matrix multiplication code. It code generates three matrices with random numbers and distributes the  multiplication workload among the slave nodes. After each slave finished his work, the final result is printed.

The code is pretty self explanatory. Loads of comments has been added as well.

Building and Running the project
================================
Compiling the project
---------------------
Run `mpic mm.cpp`

Running the project
-------------------
Run `mpirun -n 4 ./a.out`

Parameter `-n` specifies the number of nodes to run. If n = 4, that means one master and 3 slaves.

Configurations
==============
Changing matrix sizes
---------------------
Change the following variables. Names are self explanatory.
 - NUM_ROWS_A
 - NUM_COLUMNS_A
 - NUM_ROWS_B
 - NUM_COLUMNS_B
 - NUM_ROWS_C
 - NUM_COLUMNS_C

Changing the range of random numbers
------------------------------------
The project is configured to generate random numbers for each matrix as follows.
- Matrix A : 0 - 100
- Matrix B : 0 - 10
- Matrix C : 0 - 7

Change the following constants to change these ranges
- MATRIX_A_MAX_VAL
- MATRIX_B_MAX_VAL
- MATRIX_C_MAX_VAL
