// AUTHOR: Javier Garcia Santana
// DATE: 09/11/2023
// EMAIL: javier.santana@tprs.stud.vu.lt
// VERSION: 2.0
// COURSE: Parallel and Distributed Computing
// EXCERCISE Nº: 3
// COMMENTS:

#include <iostream>  
#include <string>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>


// Number of solutions found
int numofSol = 0;

double startTime, endTime;

// Board size and number of queens
#define N 12

void placeQ(int queens[], int row, int column) {
    
  for(int i = 0; i < row; i++) {
    // Vertical
    if (queens[i] == column) {
      return;
    }
      
    // Two queens in the same diagonal
    if (abs(queens[i] - column) == (row - i))  {
      return;
    }
  }
    
    // Set the queen
    queens[row] = column;
    
  if(row == N-1) {
      
    #pragma omp critical 
      numofSol++;  //Placed the final queen, found a solution
    
  } else {
    // Increment row
    for(int i = 0; i < N; i++) {
      placeQ(queens, row + 1, i);
    }
  }
} 

void solve() {
  #pragma omp parallel 
  #pragma omp single
  {
    for(int i = 0; i < N; i++) {
      // New task added for first row and each column recursion.
      #pragma omp task
      { 
        placeQ(new int[N], 0, i);
      }
    }
  }
} 

int main(int argc, char*argv[]) {
  omp_set_num_threads(1);
  double base_startTime = omp_get_wtime();   
  solve();
  double base_endTime = omp_get_wtime();

  std::cout << "Board Size: " << N << std::endl; 
  std::cout << "Number of threads: 1" << std::endl;
  std::cout << "Number of solutions: " << numofSol << std::endl; 
  std::cout << "Execution time: " << base_endTime - base_startTime << " seconds." << std::endl; 
  std::cout << "SpeedUp: 1.0" << std::endl;

  numofSol = 0;
  omp_set_num_threads(2);
  startTime = omp_get_wtime();   
  solve();
  endTime = omp_get_wtime();

  std::cout << "Board Size: " << N << std::endl; 
  std::cout << "Number of threads: 2" << std::endl;
  std::cout << "Number of solutions: " << numofSol << std::endl; 
  std::cout << "Execution time: " << endTime - startTime << " seconds." << std::endl; 
  std::cout << "SpeedUp: " <<  (base_endTime - base_startTime) / (endTime - startTime) << std::endl;

  numofSol = 0;
  omp_set_num_threads(4);
  startTime = omp_get_wtime();   
  solve();
  endTime = omp_get_wtime();

  std::cout << "Board Size: " << N << std::endl; 
  std::cout << "Number of threads: 4" << std::endl;
  std::cout << "Number of solutions: " << numofSol << std::endl; 
  std::cout << "Execution time: " << endTime - startTime << " seconds." << std::endl; 
  std::cout << "SpeedUp: " <<  (base_endTime - base_startTime) / (endTime - startTime) << std::endl;

  numofSol = 0;
  omp_set_num_threads(8);
  startTime = omp_get_wtime();   
  solve();
  endTime = omp_get_wtime();
    
  std::cout << "Board Size: " << N << std::endl; 
  std::cout << "Number of threads: 8" << std::endl;
  std::cout << "Number of solutions: " << numofSol << std::endl; 
  std::cout << "Execution time: " << endTime - startTime << " seconds." << std::endl; 
  std::cout << "SpeedUp: " <<  (base_endTime - base_startTime) / (endTime - startTime) << std::endl;

  return 0;
}


