Summary
=======
The code generates two matrices with random numbers and distributes the  multiplication workload among the slave nodes. After each slave finished his work, the final result is printed.


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

Changing the range of random numbers
------------------------------------
The project is configured to generate random numbers for each matrix as follows.
- Matrix A : 0 - 100
- Matrix B : 0 -10

Change the following constants to change these ranges
- MATRIX_A_MAX_VAL
- MATRIX_B_MAX_VAL
