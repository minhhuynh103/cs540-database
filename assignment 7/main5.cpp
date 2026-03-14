/* This is a skeleton code for Optimized sort-merge join, you can make modifications as long as you meet 
   all question requirements*/  

#include <bits/stdc++.h>
#include "record_class5.h"

using namespace std;

#define buffer_size 500 //defines how many pages are available in the Main Memory 

Records buffers[buffer_size]; 

// Compare employee records based on manager_id for sorting
bool compareEmp(const Records &a, const Records &b) {
    if (a.emp_record.id != b.emp_record.id)
        return a.emp_record.id < b.emp_record.id;
    
    // keep ordering deterministic if ids are the same
    if (a.emp_record.manager_id != b.emp_record.manager_id)
        return a.emp_record.manager_id < b.emp_record.manager_id;
    if (a. emp_record.name != b.emp_record.name)
        return a.emp_record.name < b.emp_record.name;

    return a.emp_record.bio < b.emp_record.bio;
}

// Compare department records based on manager_id for sorting
bool compareDept(const Records &a, const Records &b) {
    if (a.dept_record.manager_id != b.dept_record.manager_id)
        return a.dept_record.manager_id < b.dept_record.manager_id;

    // if manager_id is the same, sort by dept id
    if (a.dept_record.did != b.dept_record.did)
        return a.dept_record.did < b.dept_record.did;

    return a.dept_record.dname < b.dept_record.dname;
}

// Generate temp run file name
string MakeRunName(const string &prefix, int pass_no, int run_no) {
    return prefix + "_pass" + to_string(pass_no) + "_run" + to_string(run_no) + ".csv";
}

// Write employee record to a CSV file
void WriteEmpRecord(fstream &out, const Records &r) {
    out << r.emp_record.id << ","
        << r.emp_record.name << ","
        << r.emp_record.bio << ","
        << r.emp_record.manager_id << "\n";
}

// Write dept record to a CSV file
void WriteDeptRecord(fstream &out, const Records &r) {
    out << r.dept_record.did << ","
        << r.dept_record.dname << ","
        << r.dept_record.manager_id << "\n";
}

// Function to create runs for Employee and Dept
// TO DO: Complete the following function to sort the buffers in main memory and store the sorted records into temporary files (Runs).
vector<string> Sort_Buffer(fstream &input, bool isEmployee, const string &prefix) {
    vector<string> run_files;
    int run_no = 0;

    while (true) {
        int count = 0;
        // fill memory buffer with up to 500 records/pages
        for (int i = 0; i < buffer_size; i++) {
            if (isEmployee) 
                buffers[i] = Grab_Emp_Record(input);
            else
                buffers[i] = Grab_Dept_Record(input);
            
            if (buffers[i].no_values == -1) 
                break; // no more records to read
            
            count++;
        }
        // no more records
        if (count == 0) 
            break;

        // sort in memmory based on join key
        if (isEmployee)
            sort(buffers, buffers + count, compareEmp);
        else
            sort(buffers, buffers + count, compareDept);

        // write sorted buffer to a new run file
        string run_name = MakeRunName(prefix, 0, run_no++);
        fstream out(run_name, ios::out | ios::trunc);
        for (int i = 0; i < count; i++) {
            if (isEmployee)
                WriteEmpRecord(out, buffers[i]);
            else
                WriteDeptRecord(out, buffers[i]);
        }

        out.close();
        run_files.push_back(run_name);
        if (count < buffer_size) 
            break; // last run created, no more records to read
    }

    return run_files;
}


// Function Mergepass to merge sorted runs of Employee and Dept at the same time using main memory
vector<string> MergePass(const vector<string> &input_runs, bool isEmployee, const string &prefix, int pass_no) {
    vector<string> output_runs;
    const int fan_in = buffer_size - 1; // 499 input buffer + 1 output buffer

    for (int start = 0, out_no = 0; start < (int)input_runs.size(); start += fan_in, out_no++) {
        int end = min(start + fan_in, (int)input_runs.size());
        int k = end - start; // number of runs to merge

        // open input files
        vector<fstream*> ins(k, nullptr);
        vector<Records> current(k);

        for (int i = 0; i < k; i++) {
            ins[i] = new fstream(input_runs[start + i], ios::in);
            if (isEmployee)
                current[i] = Grab_Emp_Record(*ins[i]);
            else
                current[i] = Grab_Dept_Record(*ins[i]);
        }

        // create output run file
        string out_name = MakeRunName(prefix, pass_no, out_no);
        fstream out(out_name, ios::out | ios::trunc);

        while (true) {
            int min_index = -1;

            // find smallest current record among the k runs
            for (int i = 0; i < k; i++) {
                if (current[i].no_values == -1) 
                    continue; // this run is exhausted
                if (min_index == -1) {
                    min_index = i;
                } else {
                    bool take_i;
                    if (isEmployee) 
                        take_i = compareEmp(current[i], current[min_index]);
                    else
                        take_i = compareDept(current[i], current[min_index]);

                    if (take_i)
                        min_index = i;
                }
            }

            if (min_index == -1) 
                break; // all runs exhausted
            // write smallest record to output
            if (isEmployee)
                WriteEmpRecord(out, current[min_index]);
            else
                WriteDeptRecord(out, current[min_index]);

            // refill that run current record
            if (isEmployee)
                current[min_index] = Grab_Emp_Record(*ins[min_index]);
            else
                current[min_index] = Grab_Dept_Record(*ins[min_index]);
        }

        out.close();
        // close input files
        for (int i = 0; i < k; i++) {
            ins[i]->close();
            delete ins[i];
        }

        output_runs.push_back(out_name);
    }

    return output_runs;
}


// Function to merge helper to merge until only one fully sorted file remains
string FullyMergeRuns(vector<string> run_files, bool isEmployee, const string &prefix) {
    if (run_files.empty())
        return "";

    if (run_files.size() == 1)
        return run_files[0]; // fully sorted

    int pass_no = 1;

    while (run_files.size() > 1) {
        vector<string> merged = MergePass(run_files, isEmployee, prefix, pass_no);

        // delete old run files
        for (const string &file : run_files) {
            remove(file.c_str());
        }

        run_files = merged;
        pass_no++;
    }

    return run_files[0]; 
}

// TODO: Complete the following function to store the sorted results from Merge_Join_Runs into Join.csv.
void PrintJoin(fstream &joinout, const Records::DeptRecord &dept, const Records::EmpRecord &emp) {
    joinout << dept.did << ","
            << dept.dname << ","
            << dept.manager_id << ","
            << emp.id << ","
            << emp.name << ","
            << emp.bio << ","
            << emp.manager_id << "\n";
}

// Function to merge sorted runs of Employee and Dept at the same time using main memory
// TODO: Complete the following function to merge the sorted temporary files ('runs') of Employee and Dept, and store the final result in Join.csv using PrintJoin().
void Merge_Join_Runs(const string &dept_sorted_file, const string &emp_sorted_file, fstream &joinout) {
    fstream deptin(dept_sorted_file, ios::in);
    fstream empin(emp_sorted_file, ios::in);

    Records dept = Grab_Dept_Record(deptin);
    Records emp = Grab_Emp_Record(empin);

    // sort merge join
    while (dept.no_values != -1 && emp.no_values != -1) {
        if (dept.dept_record.manager_id < emp.emp_record.id) {
            dept = Grab_Dept_Record(deptin); // dept join key smaller advance dept
        }
        else if (dept.dept_record.manager_id > emp.emp_record.id) {
            emp = Grab_Emp_Record(empin); // emp join key smaller advance emp
        }
        else {
            // matching join key found
            int join_key = dept.dept_record.manager_id;

            // collect dept tuples with this join key
            vector<Records::DeptRecord> dept_group;
            while (dept.no_values != -1 && dept.dept_record.manager_id == join_key) {
                dept_group.push_back(dept.dept_record);
                dept = Grab_Dept_Record(deptin);
            }

            // collect emp tuples with this join key
            vector<Records::EmpRecord> emp_group;
            while (emp.no_values != -1 && emp.emp_record.id == join_key) {
                emp_group.push_back(emp.emp_record);
                emp = Grab_Emp_Record(empin);
            }

            // output cartesian product of dept_group and emp_group
            for (const auto &d : dept_group) {
                for (const auto &e : emp_group) {
                    PrintJoin(joinout, d, e);
                }
            }
        }
    }

    deptin.close();
    empin.close();
}



int main() {

    fstream empin; //Open file streams to read and write Employee_p2.csv
    fstream deptin; //Open file streams to read and write Dept_p2.csv

    empin.open("Employee_p2.csv", ios::in);
    deptin.open("Dept_p2.csv", ios::in);
   
    if (!empin.is_open() || !deptin.is_open()) {
        cerr << "Error: could not open input files.\n";
        return 1;
    }

    fstream joinout;
    joinout.open("Join.csv",ios::out | ios::trunc); //Creating the Join.csv file where we will store our joined results
    
    if (!joinout.is_open()) {
        cerr << "Error: could not open Join.csv\n";
        return 1;
    }

    //TODO: Create sorted runs for Dept and Employee using Sort_Buffer()
    vector<string> emp_runs = Sort_Buffer(empin, true, "Employee");
    vector<string> dept_runs = Sort_Buffer(deptin, false, "Dept");

    empin.close();
    deptin.close();

    // Merge runs until each relation becomes one fully sorted file
    string emp_sorted = FullyMergeRuns(emp_runs, true, "Employee");
    string dept_sorted = FullyMergeRuns(dept_runs, false, "Dept");

    //TODO: Use Merge_Join_Runs() to Join the runs of Dept and Employee relations and generate Join.csv
    Merge_Join_Runs(dept_sorted, emp_sorted, joinout);

    joinout.close();

    //TODO: Please delete the temporary files (runs) after you've joined both Employee_p2.csv and Dept_p2.csv
    if (!emp_sorted.empty())
        remove(emp_sorted.c_str());

    if (!dept_sorted.empty())
        remove(dept_sorted.c_str());

    return 0;
}
