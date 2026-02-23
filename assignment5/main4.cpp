/* This is a skeleton code for two-pass multi-way sorting. You can make modifications as long as you meet 
   all question requirements. You are also free to change the return type and arguments as needed. */

#include <bits/stdc++.h>
#include "record_class4.h"

using namespace std;

#define buffer_size 250 //defines how many pages are available in the Main Memory 


Records buffers[buffer_size]; 

/***TODO: You may need to modify the return type and arguments of the following functions based on your implementation.***/
static inline void WriteCSV(ofstream &out, const Records &r) { // write record to output file 
    out << r.emp_record.id << ","
        << r.emp_record.name << ","
        << r.emp_record.bio << ","
        << r.emp_record.manager_id << "\n";
}

// Read record from input file
static inline bool ReadOneCSV (istream &in, Records &r) { 
    string line, word;

    while (getline(in, line)) {
        if (line.empty()) continue; // skip blank lines

        stringstream s(line);       // turn line into a stream

        if (!getline(s, word, ',')) continue; // id
        r.emp_record.id = stoi(word);
        getline(s, word, ',');      // name
        r.emp_record.name = word;
        getline(s, word, ',');      // bio
        r.emp_record.bio = word;
        if (!getline(s, word)) continue; // read manager_id till end 
        r.emp_record.manager_id = stoi(word);

        r.no_values = 4;
        return true;
    } 
    return false;
}

//Function for PASS 1
// TODO: Complete the following function to sort the buffers and store the sorted records into a temporary file (Runs).
// Pass 1: read chunks of employee csv up to 250 records, sort, write each chunk as run file, then return number of runs
int Sort_Buffer(istream &empin) {
    int run_count = 0;

    while (true) {
        int loaded = 0;

        // load up to buffer_size records 
        while (loaded < buffer_size) {
            Records rec;
            if (!ReadOneCSV(empin, rec)) break; 
            buffers[loaded++] = std::move(rec); // move to avoid copy
        }
        if (loaded == 0) break; // no records loaded > done

        // sort in-memory buffer by id 
        vector<int> idx(loaded);
        iota(idx.begin(), idx.end(), 0); // initialize index vector
        sort(idx.begin(), idx.end(), [&](int a, int b) { 
            return buffers[a].emp_record.id < buffers[b].emp_record.id; 
        });

        // write one sorted run file
        string run_name = "run_" + to_string(run_count) + ".tmp";
        ofstream run_out(run_name, ios::out | ios::trunc);
        if (!run_out.is_open()) {
            cerr << "Error creating run file: " << run_name << "\n";
            exit(1);
        }
        
        for (int k = 0; k < loaded; k++) {
            WriteCSV(run_out, buffers[idx[k]]);
        }
        run_out.close();
        run_count++; // increment exactly once per run
    }

    return run_count;
}

// TODO: Complete the following function to store the sorted results from PASS 2 into EmpSorted.csv.
static inline void PrintSorted(ofstream &out, const Records &r) { // write record to output file
    WriteCSV(out, r);
}

//Function for PASS 2
// TODO: Complete the following function to merge the sorted temporary files ('runs') and store the final result in EmpSorted.csv using PrintSorted().
// Pass 2: k-way merge the run files and write to EmpSorted.csv
void Merge_Runs(int run_count, const string &output_file) {
    // Build initial run filenames
    vector<string> run_files;
    run_files.reserve(run_count);
    for (int i = 0; i < run_count; i++) {
        run_files.push_back("run_" + to_string(i) + ".tmp");
    }

    // Open all run files
    vector<ifstream> ins(run_files.size());
    for (size_t i = 0; i < run_files.size(); i++) {
        ins[i].open(run_files[i]);
        if (!ins[i].is_open()) {
            cerr << "Error opening run file: " << run_files[i] << "\n";
            exit(1);
        }
    }

    // create output file for merged results
    ofstream out(output_file, ios::out | ios::trunc);
    if (!out.is_open()) {
        cerr << "Error creating output file: " << output_file << "\n";
        exit(1);
    }

    // heap entries for id and stream_index
    struct Node { int id; int si; };
    struct Cmp { bool operator()(const Node &a, const Node &b) const { return a.id > b.id; }};
    priority_queue<Node, vector<Node>, Cmp> pq;

    vector<Records> cur(run_files.size()); // current record from each run

    // initialize heap with first record from each run
    for (size_t i = 0; i < run_files.size(); i++) {
        if (ReadOneCSV(ins[i], cur[i])) {
            pq.push(Node{cur[i].emp_record.id, (int)i});
        }
    }

    // k-way merge
    while (!pq.empty()) {
        Node n = pq.top(); pq.pop();
        int si = n.si;
        PrintSorted(out, cur[si]); // write smallest record to output

        if (ReadOneCSV(ins[si], cur[si])) {
            pq.push(Node{cur[si].emp_record.id, si}); // push next record from same run
        }
    }
    out.close();
    for (auto &in : ins) in.close();

    // delete final round run files
    for (const auto &f : run_files) {
    remove(f.c_str());
    }
}

int main() {

    fstream empin;     //Open file streams to read and write the Employee_p1.csv

    empin.open("Employee_p1.csv", ios::in);  //Opening out the Employee_p1.csv that we want to sort
    if (!empin.is_open()) {
        cerr << "ERROR: Employee_p1.csv not found.\n";
        return 1;
    }
   
    //TO DO: PASS 1, Create sorted runs for Employee_p1.csv using Sort_Buffer()
    int run_count = Sort_Buffer(empin);
    empin.close();

    if (run_count > buffer_size - 1) {
        for (int i = 0; i < run_count; i++) {
            string f = "run_" + to_string(i) + ".tmp";
            remove(f.c_str());
        }
        cerr << "ERROR: Too many runs for 2-pass merge with M=250.\n";
        return 1;
    }

    //TO DO: PASS 2, Use Merge_Runs() to sort the runs and generate EmpSorted.csv
    Merge_Runs(run_count, "EmpSorted.csv");

    //Please delete the temporary files (runs) after you've sorted the Employee_p1.csv
    return 0;
}
