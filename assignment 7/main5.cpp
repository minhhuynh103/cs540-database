/* This is a skeleton code for Optimized sort-merge join, you can make modifications as long as you meet 
   all question requirements*/  

#include <bits/stdc++.h>
#include "record_class5.h"

using namespace std;

#define buffer_size 500 //defines how many pages are available in the Main Memory 

Records buffers[buffer_size]; 

/***TODO: You may need to modify the return type and arguments of the following functions based on your implementation.***/

// Function to create runs for Employee and Dept
// TO DO: Complete the following function to sort the buffers in main memory and store the sorted records into temporary files (Runs).
void Sort_Buffer(){

    return;
}

// Function to merge sorted runs of Employee and Dept at the same time using main memory
// TODO: Complete the following function to merge the sorted temporary files ('runs') of Employee and Dept, and store the final result in Join.csv using PrintJoin().
void Merge_Join_Runs(){
   
    return;
}

// TODO: Complete the following function to store the sorted results from Merge_Join_Runs into Join.csv.
void PrintJoin() {
    
    return;
}

int main() {

    fstream empin; //Open file streams to read and write Employee_p2.csv
    fstream deptin; //Open file streams to read and write Dept_p2.csv

    empin.open("Employee_p2.csv", ios::in);
    deptin.open("Dept_p2.csv", ios::in);
   
    fstream joinout;
    joinout.open("Join.csv", ios::out | ios::app); //Creating the Join.csv file where we will store our joined results
    
    //TODO: Create sorted runs for Dept and Employee using Sort_Buffer()


    //TODO: Use Merge_Join_Runs() to Join the runs of Dept and Employee relations and generate Join.csv


    //TODO: Please delete the temporary files (runs) after you've joined both Employee_p2.csv and Dept_p2.csv

    return 0;
}