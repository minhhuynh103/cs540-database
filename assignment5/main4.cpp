/* This is a skeleton code for two-pass multi-way sorting. You can make modifications as long as you meet 
   all question requirements. You are also free to change the return type and arguments as needed. */

#include <bits/stdc++.h>
#include "record_class4.h"

using namespace std;

#define buffer_size 250 //defines how many pages are available in the Main Memory 


Records buffers[buffer_size]; 

/***TODO: You may need to modify the return type and arguments of the following functions based on your implementation.***/


//Function for PASS 1
// TODO: Complete the following function to sort the buffers and store the sorted records into a temporary file (Runs).
void Sort_Buffer(){

    return;
}

//Function for PASS 2
// TODO: Complete the following function to merge the sorted temporary files ('runs') and store the final result in EmpSorted.csv using PrintSorted().
void Merge_Runs(){

    return;
}

// TODO: Complete the following function to store the sorted results from PASS 2 into EmpSorted.csv.
void PrintSorted(){

    return;
}

int main() {

    fstream empin;     //Open file streams to read and write the Employee_p1.csv

    empin.open("Employee_p1.csv", ios::in);  //Opening out the Employee_p1.csv that we want to sort

   
    fstream SortOut; //Open file streams to read and write the EmpSorted.csv
    SortOut.open("EmpSorted.csv", ios::out | ios::app);  //Creating the EmpSorted.csv file where we will store our sorted results


    //TO DO: PASS 1, Create sorted runs for Employee_p1.csv using Sort_Buffer()


    //TO DO: PASS 2, Use Merge_Runs() to sort the runs and generate EmpSorted.csv


    //Please delete the temporary files (runs) after you've sorted the Employee_p1.csv

    return 0;
}
