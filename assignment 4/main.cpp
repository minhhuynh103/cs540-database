
#include <string>
#include <iostream>
#include <stdexcept>
#include "classes.h"

using namespace std;


int main(int argc, char** argv) {

    // Create the index from Employee.csv
    LinearHashIndex hashIndex("EmployeeIndex");
    hashIndex.createFromFile("Employee.csv");


    // TODO: You'll receive employee IDs as arguments, process them to retrieve the record, or display a message if not found. 
        for (int k = 1; k < argc; k++) {
            int id = stoi(string(argv[k]));
            cout << "Searching for ID=" << id << "\n";
            hashIndex.findAndPrintEmployee(id);
            cout << "---\n";
        }

    return 0;
}

