#include <stdio.h>
#include <mpi.h>
#include <ctime>    // For time()
#include <cstdlib>  // For srand() and rand()

#define NUM_ROWS_A 12 //rows of input [A]
#define NUM_COLUMNS_A 12 //columns of input [A]
#define NUM_ROWS_B 12 //rows of input [B]
#define NUM_COLUMNS_B 12 //columns of input [B]
#define NUM_ROWS_C 12 //rows of input [C]
#define NUM_COLUMNS_C 12 //columns of input [C]
#define NUM_ROWS_P  NUM_ROWS_A //P is the resultant matrix of A*B
#define NUM_COLUMNS_P NUM_COLUMNS_C //P is the resultant matrix of A*B
#define MASTER_TO_SLAVE_TAG 1 //tag for messages sent from master to slaves
#define SLAVE_TO_MASTER_TAG 4 //tag for messages sent from slaves to master
#define MASTER_RANK 0 //rank of the master node
#define MATRIX_A_MAX_VAL 100 //max value of an entry in matrix A
#define MATRIX_B_MAX_VAL 10 //max value of an entry in matrix B
#define MATRIX_C_MAX_VAL 7 //max value of an entry in matrix C


//functions
void makeABC(); //makes the [A], [B], [C] matrixes
void printArray(); //print the content of output matrix [Z];
int generateRandomNumber(int max); //generate random numbers in the range of 0-max

int rank; //process rank
int size; //number of processes
int i, j, k; //helper variables
double mat_a[NUM_ROWS_A][NUM_COLUMNS_A]; //declare input [A]
double mat_b[NUM_ROWS_B][NUM_COLUMNS_B]; //declare input [B]
double mat_c[NUM_ROWS_C][NUM_COLUMNS_C]; //declare input [B]
double mat_res_1[NUM_ROWS_P][NUM_COLUMNS_P]; //resultant matrix of A*B
double mat_result[NUM_ROWS_A][NUM_COLUMNS_C]; //declare output [Z]

double start_time; //hold start time
double end_time; // hold end time
int low_bound; //low bound of the number of rows of [A] allocated to a slave
int upper_bound; //upper bound of the number of rows of [A] allocated to a slave
int portion; //portion of the number of rows of [A] allocated to a slave
MPI_Status status; // store status of a MPI_Recv
MPI_Request request; //capture request of a MPI_Isend

int main(int argc, char * argv[]) {
  MPI_Init( & argc, & argv); //initialize MPI operations
  MPI_Comm_rank(MPI_COMM_WORLD, & rank); //get the rank
  MPI_Comm_size(MPI_COMM_WORLD, & size); //get number of processes
  /* master initializes work*/
  if (rank == MASTER_RANK) {
    makeABC();
    start_time = MPI_Wtime();
    for (i = 1; i < size; i++) { //for each slave other than the master
      //First we multiply A*B
      portion = (NUM_ROWS_A / (size - 1)); // calculate portion without master
      low_bound = (i - 1) * portion;
      if (((i + 1) == size) && ((NUM_ROWS_A % (size - 1)) != 0)) { //if rows of [A] cannot be equally divided among slaves
        upper_bound = NUM_ROWS_A; //last slave gets all the remaining rows
      } else {
        upper_bound = low_bound + portion; //rows of [A] are equally divisable among slaves
      }
      //send the low bound first without blocking, to the intended slave
      MPI_Isend( & low_bound, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, & request);
      //next send the upper bound without blocking, to the intended slave
      MPI_Isend( & upper_bound, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD, & request);
      //finally send the allocated row portion of [A] without blocking, to the intended slave
      MPI_Isend( & mat_a[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_A, MPI_DOUBLE, i, MASTER_TO_SLAVE_TAG + 2, MPI_COMM_WORLD, & request);
    }
  }
  //broadcast [B] to all the slaves
  MPI_Bcast( & mat_b, NUM_ROWS_B * NUM_COLUMNS_B, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  /* work done by slaves*/
  if (rank > MASTER_RANK) {
    //receive low bound from the master
    MPI_Recv( & low_bound, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, & status);
    //next receive upper bound from the master
    MPI_Recv( & upper_bound, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD, & status);
    //finally receive row portion of [A] to be processed from the master
    MPI_Recv( & mat_a[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_A, MPI_DOUBLE, 0, MASTER_TO_SLAVE_TAG + 2, MPI_COMM_WORLD, & status);
    for (i = low_bound; i < upper_bound; i++) { //iterate through a given set of rows of [A]
      for (j = 0; j < NUM_COLUMNS_B; j++) { //iterate through columns of [B]
        for (k = 0; k < NUM_ROWS_B; k++) { //iterate through rows of [B]
          mat_res_1[i][j] += (mat_a[i][k] * mat_b[k][j]);
        }
      }
    }
    //send back the low bound first without blocking, to the master
    MPI_Isend( & low_bound, 1, MPI_INT, 0, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, & request);
    //send the upper bound next without blocking, to the master
    MPI_Isend( & upper_bound, 1, MPI_INT, 0, SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD, & request);
    //finally send the processed portion of data without blocking, to the master
    MPI_Isend( & mat_res_1[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_B, MPI_DOUBLE, 0, SLAVE_TO_MASTER_TAG + 2, MPI_COMM_WORLD, & request);
  }

  /* master gathers processed work*/
  if (rank == MASTER_RANK) {
    for (i = 1; i < size; i++) { // untill all slaves have handed back the processed data
      //receive low bound from a slave
      MPI_Recv( & low_bound, 1, MPI_INT, i, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, & status);
      //receive upper bound from a slave
      MPI_Recv( & upper_bound, 1, MPI_INT, i, SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD, & status);
      //receive processed data from a slave
      MPI_Recv( & mat_res_1[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_B, MPI_DOUBLE, i, SLAVE_TO_MASTER_TAG + 2, MPI_COMM_WORLD, & status);
    }
  }
    //prepare for second multiplication. Here we have  A*B. Now we will multiply
    //(A*B)*C
    if (rank == MASTER_RANK) {
      for (i = 1; i < size; i++) { //for each slave other than the master
        portion = (NUM_ROWS_P / (size - 1)); // calculate portion without master
        low_bound = (i - 1) * portion;
        if (((i + 1) == size) && ((NUM_ROWS_P % (size - 1)) != 0)) { //if rows of [P] cannot be equally divided among slaves
          upper_bound = NUM_ROWS_P; //last slave gets all the remaining rows
        } else {
          upper_bound = low_bound + portion; //rows of [P] are equally divisable among slaves
        }
        //send the low bound first without blocking, to the intended slave
        MPI_Isend( & low_bound, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, & request);
        //next send the upper bound without blocking, to the intended slave
        MPI_Isend( & upper_bound, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD, & request);
        //finally send the allocated row portion of [A] without blocking, to the intended slave
        MPI_Isend( & mat_res_1[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_P, MPI_DOUBLE, i, MASTER_TO_SLAVE_TAG + 2, MPI_COMM_WORLD, & request);
      }
    }

    //broadcast [C] to all the slaves
    MPI_Bcast( & mat_c, NUM_ROWS_C * NUM_COLUMNS_C, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    /* work done by slaves*/
    if (rank > MASTER_RANK) {
      //receive low bound from the master
      MPI_Recv( & low_bound, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, & status);
      //next receive upper bound from the master
      MPI_Recv( & upper_bound, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD, & status);
      //finally receive row portion of [P] to be processed from the master
      MPI_Recv( & mat_res_1[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_P, MPI_DOUBLE, 0, MASTER_TO_SLAVE_TAG + 2, MPI_COMM_WORLD, & status);
      for (i = low_bound; i < upper_bound; i++) { //iterate through a given set of rows of [P]
        for (j = 0; j < NUM_COLUMNS_C; j++) { //iterate through columns of [C]
          for (k = 0; k < NUM_ROWS_C; k++) { //iterate through rows of [C]
            mat_result[i][j] += (mat_res_1[i][k] * mat_c[k][j]);
          }
        }
      }
      //send back the low bound first without blocking, to the master
      MPI_Isend( & low_bound, 1, MPI_INT, 0, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, & request);
      //send the upper bound next without blocking, to the master
      MPI_Isend( & upper_bound, 1, MPI_INT, 0, SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD, & request);
      //finally send the processed portion of data without blocking, to the master
      MPI_Isend( & mat_result[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_C, MPI_DOUBLE, 0, SLAVE_TO_MASTER_TAG + 2, MPI_COMM_WORLD, & request);
    }

    /* master gathers processed work*/
    if (rank == MASTER_RANK) {
      for (i = 1; i < size; i++) { // untill all slaves have handed back the processed data
        //receive low bound from a slave
        MPI_Recv( & low_bound, 1, MPI_INT, i, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, & status);
        //receive upper bound from a slave
        MPI_Recv( & upper_bound, 1, MPI_INT, i, SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD, & status);
        //receive processed data from a slave
        MPI_Recv( & mat_result[low_bound][0], (upper_bound - low_bound) * NUM_COLUMNS_C, MPI_DOUBLE, i, SLAVE_TO_MASTER_TAG + 2, MPI_COMM_WORLD, & status);
      }

    end_time = MPI_Wtime();
    printf("\nRunning Time = %f\n\n", end_time - start_time);
    printArray();
  }
  MPI_Finalize(); //finalize MPI operations
  return 0;
}
void makeABC() {
  srand(time(0));  // Initialize random number generator.

  for (i = 0; i < NUM_ROWS_A; i++) {
    for (j = 0; j < NUM_COLUMNS_A; j++) {
      mat_a[i][j] = generateRandomNumber(MATRIX_A_MAX_VAL);
    }
  }
  for (i = 0; i < NUM_ROWS_B; i++) {
    for (j = 0; j < NUM_COLUMNS_B; j++) {
      mat_b[i][j] = generateRandomNumber(MATRIX_B_MAX_VAL);
    }
  }
  for (i = 0; i < NUM_ROWS_C; i++) {
    for (j = 0; j < NUM_COLUMNS_C; j++) {
      mat_c[i][j] = generateRandomNumber(MATRIX_C_MAX_VAL);
    }
  }
}

int generateRandomNumber(int max){
  return rand() % max + 1;
}
void printArray() {
  printf("Matrix A:\n");
  for (i = 0; i < NUM_ROWS_A; i++) {
    printf("\n");
    for (j = 0; j < NUM_COLUMNS_A; j++)
      printf("%8.2f  ", mat_a[i][j]);
  }
  printf("\n\n\n");
  printf("Matrix B:\n");
  for (i = 0; i < NUM_ROWS_B; i++) {
    printf("\n");
    for (j = 0; j < NUM_COLUMNS_B; j++)
      printf("%8.2f  ", mat_b[i][j]);
  }
  printf("\n\n\n");
  printf("Matrix A*B:\n");
  for (i = 0; i < NUM_ROWS_P; i++) {
    printf("\n");
    for (j = 0; j < NUM_COLUMNS_P; j++)
      printf("%8.2f  ", mat_res_1[i][j]);
  }
  printf("\n\n\n");
  printf("Matrix C:\n");
  for (i = 0; i < NUM_ROWS_C; i++) {
    printf("\n");
    for (j = 0; j < NUM_COLUMNS_C; j++)
      printf("%8.2f  ", mat_c[i][j]);
  }
  printf("\n\n\n");
  printf("Resulting Matrix:\n");
  for (i = 0; i < NUM_ROWS_A; i++) {
    printf("\n");
    for (j = 0; j < NUM_COLUMNS_B; j++)
      printf("%8.2f  ", mat_result[i][j]);
  }
  printf("\n\n");
}
