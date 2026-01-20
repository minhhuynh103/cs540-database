/*
Assignment 2: Storage Manager
Minh Huynh
huynhmin
huynhmin@oregonstate.edu

Tabitha Rowland
rowlanta
rowlanta@oregonstate.edu
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;

int main(int argc, char *argv[])
{
    // Initialize the Storage Manager Class with the Binary .dat file name we want to create
    StorageManager manager("EmployeeRelation.dat");

    // Assuming the Employee.CSV file is in the same directory,
    // we want to read from the Employee.csv and write into the new data_file
    manager.createFromFile("Employee.csv");

    // Searching for Employee IDs Using [manager.findAndPrintEmployee(id)]
    /***TO_DO***/

    // If an ID was provided on the command line, search it first
    if (argc >= 2)
    {
        try
        {
            int64_t id = stoll(argv[1]);
            manager.findAndPrintEmployee(id);
        }
        catch (...)
        {
            cerr << "Invalid id argument: " << argv[1] << endl;
            return 1;
        }
    }

    // Then allow multiple searches (one id at a time) until EOF (Ctrl-D)
    int64_t q;
    while (true)
    {
        cout << "Enter Employee id to search (Ctrl-D to quit): ";
        if (!(cin >> q))
            break;
        manager.findAndPrintEmployee(q);
    }

    return 0;
}