/* This is a skeleton code for optimized sort-merge join, you can make modifications as long as you meet 
   all question requirements*/  
/* This record_class.h contains the class Records, which can be used to store tuples from Employee_p2.csv (using the EmpRecord structure) and Dept_p2.csv (using the DeptRecord structure).*/
#include <bits/stdc++.h>

using namespace std;

class Records{
    public:
    
    struct EmpRecord {
        int id;
        string name;
        string bio;
        int manager_id;
    }emp_record;

    struct DeptRecord {
        int did;
        string dname;
        int manager_id;
    }dept_record;

    /*** You can add more variables if you want below ***/
    int no_values = 0; //You can use this to check if you've don't have any more tuples

};

// Grab a single block from the Employee_p2.csv file and put it inside the EmpRecord structure of the Records Class
Records Grab_Emp_Record(fstream &empin) {
    string line, word;
    Records emp;
    if (getline(empin, line, '\n')) {  // grab entire line
        stringstream s(line); // turn line into a stream

        getline(s, word,',');
        emp.emp_record.id = stoi(word);
        getline(s, word, ',');
        emp.emp_record.name = word;
        getline(s, word, ',');
        emp.emp_record.bio = word;
        getline(s, word, ',');
        emp.emp_record.manager_id = stoi(word);

        return emp;
    } else {
        emp.no_values = -1;
        return emp;
    }
}

// Grab a single block from the Dept_p2.csv file and put it inside the DeptRecord structure of the Records Class
Records Grab_Dept_Record(fstream &deptin) {
    string line, word;
    Records dept;
    if (getline(deptin, line, '\n')) {
        stringstream s(line);
        getline(s, word,',');
        dept.dept_record.did = stoi(word);
        getline(s, word, ',');
        dept.dept_record.dname = word;
        getline(s, word, ',');
        dept.dept_record.manager_id = stoi(word);

        return dept;
    } else {
        dept.no_values = -1;
        return dept;
    }

}
