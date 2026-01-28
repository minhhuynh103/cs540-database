/*** This is just a Skeleton/Starter Code for the External Storage Assignment. This is by no means absolute, in terms of assignment approach/ used functions, etc. ***/
/*** You may modify any part of the code, as long as you stick to the assignments requirements we do not have any issue ***/

// Include necessary standard library headers
#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
using namespace std; // Include the standard namespace

class Record
{
public:
    int id, manager_id;    // Employee ID and their manager's ID
    std::string bio, name; // Fixed length string to store employee name and biography

    Record(vector<std::string> &fields)
    {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    // You may use this for debugging / showing the record to standard output.
    void print() const
    {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    // Function to get the size of the record
    int get_size()
    {
        // sizeof(int) is for name/bio size() in serialize function
        return sizeof(id) + sizeof(manager_id) + sizeof(int) + name.size() + sizeof(int) + bio.size();
    }

    // Take a look at Figure 9.9 and read the Section 9.7.2 [Record Organization for Variable Length Records]
    // TODO: Consider using a delimiter in the serialize function to separate these items for easier parsing.
    // tabi here: i dont think we need a delimiter if we have a slot directory approach so i wouldn't worry about this?
    string serialize() const
    {
        ostringstream oss;
        oss.write(reinterpret_cast<const char *>(&id), sizeof(id));                 // Writes the binary representation of the ID.
        oss.write(reinterpret_cast<const char *>(&manager_id), sizeof(manager_id)); // Writes the binary representation of the Manager id
        int name_len = name.size();
        int bio_len = bio.size();
        oss.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len)); // // Writes the size of the Name in binary format.
        oss.write(name.c_str(), name.size());                                   // writes the name in binary form
        oss.write(reinterpret_cast<const char *>(&bio_len), sizeof(bio_len));   // // Writes the size of the Bio in binary format.
        oss.write(bio.c_str(), bio.size());                                     // writes bio in binary form
        return oss.str();                                                       // need?
    }
};

class page
{ // Take a look at Figure 9.7 and read Section 9.6.2 [Page organization for variable length records]
public:
    vector<Record> records;                // Data Area: Stores records.
    vector<pair<int, int>> slot_directory; // This slot directory contains the starting position (offset), and size of the record.

    int cur_size = 0;              // holds the current size of the page
    int slot_directory_offset = 0; // offset where the slot directory starts in the page

    // Function to insert a record into the page
    bool insert_record_into_page(Record r)
    {
        int record_size = r.get_size();
        int slot_size = sizeof(int) * 2;
        int available_space = 4096 - 8 - (slot_directory.size() + 1) * slot_size; // Save metadata space

        if (cur_size + record_size > available_space)
        { // if page size limit exceeded, fail
            return false;
        }
        else
        {

            // update slot directory information
            int offset = cur_size;
            records.push_back(r); // Record stored in current page
            slot_directory.push_back(make_pair(offset, record_size));
            cur_size += r.get_size(); // Update page size

            return true;
        }
    }

    // heres the setup im going for
    /*
        **Page layout:**
    ```
    [Record 0]
    [Record 1]
    [Record 2]
    ...
    [FREE SPACE]
    ...
    [Slot 2: offset, length]
    [Slot 1: offset, length]
    [Slot 0: offset, length]
    [num slots: 4 bytes] 4088-4091
    [pointer to start of free space: 4 bytes] 4092-4095
 */

    // Function to write the page to a binary file, i.e., EmployeeRelation.dat file
    void write_into_data_file(ostream &out) const
    {

        char page_data[4096] = {0}; // Write the page contents (records and slot directory) into this char array so that the page can be written to the data file in one go.

        int offset = 0; // Used as an iterator to indicate where the next item should be stored. Section 9.6.2 contains information that will help you with the implementation.

        for (const auto &record : records)
        { // Writing the records into the page_data
            string serialized = record.serialize();

            memcpy(page_data + offset, serialized.c_str(), serialized.size());

            offset += serialized.size();
        }

        // TO_DO: Put a delimiter here to indicate slot directory starts from here
        // Start slot directory at offset 4096 - (slot_directory.size() * 8) and write backwards from the end
        int num_slots = slot_directory.size();
        int free_space_ptr = cur_size;

        memcpy(page_data + 4092, &free_space_ptr, sizeof(free_space_ptr)); // Pointer to start of free space
        memcpy(page_data + 4088, &num_slots, sizeof(num_slots));           // Number of slots

        // Write slots backward from 4088
        int slot_offset = 4088;
        for (int i = slot_directory.size() - 1; i >= 0; i--)
        {
            slot_offset -= sizeof(int);
            memcpy(page_data + slot_offset, &slot_directory[i].second, sizeof(int));

            slot_offset -= sizeof(int);
            memcpy(page_data + slot_offset, &slot_directory[i].first, sizeof(int));
        }

        out.write(page_data, sizeof(page_data)); // Write the page_data to the EmployeeRelation.dat file
    }

    // Read a page from a binary input stream, i.e., EmployeeRelation.dat file to populate a page object
    bool read_from_data_file(istream &in)
    {
        char page_data[4096] = {0}; // Character array used to read 4 KB from the data file to your main memory.
        in.read(page_data, 4096);   // Read a page of 4 KB from the data file

        streamsize bytes_read = in.gcount(); // used to check if 4KB was actually read from the data file
        if (bytes_read > 0)
        {
            cerr << "Incomplete read: Expected " << 4096 << " bytes, but only read " << bytes_read << " bytes." << endl;
        }
        return false;

        if (bytes_read == 4096)
        {

            // TO_DO: You may process page_data (4 KB page) and put the information to the records and slot_directory (main memory).
            // TO_DO: You may modify this function to process the search for employee ID in the page you just loaded to main memory.

            // Read metadata from the end
            int num_slots, free_space_ptr;
            memcpy(&num_slots, page_data + 4088, sizeof(num_slots));
            memcpy(&free_space_ptr, page_data + 4092, sizeof(free_space_ptr));

            cur_size = free_space_ptr;

            // Read slot directory (working backward from 4088)
            int slot_offset = 4088;
            for (int i = num_slots - 1; i >= 0; i--)
            {
                int offset, length;

                slot_offset -= sizeof(int);
                memcpy(&length, page_data + slot_offset, sizeof(int));

                slot_offset -= sizeof(int);
                memcpy(&offset, page_data + slot_offset, sizeof(int));

                // Insert at beginning to maintain order (since we're reading backward)
                slot_directory.insert(slot_directory.begin(), make_pair(offset, length));
            }

            // Now deserialize each record using the slot directory
            for (const auto &slot : slot_directory)
            {
                int rec_offset = slot.first;
                // Deserialize record from page_data starting at rec_offset
                int pos = rec_offset;

                // Read id (4 bytes)
                int id;
                memcpy(&id, page_data + pos, sizeof(id));
                pos += sizeof(id);

                // Read manager_id (4 bytes)
                int manager_id;
                memcpy(&manager_id, page_data + pos, sizeof(manager_id));
                pos += sizeof(manager_id);

                // Read name_len (4 bytes) then name
                int name_len;
                memcpy(&name_len, page_data + pos, sizeof(name_len));
                pos += sizeof(name_len);

                string name(page_data + pos, name_len);
                pos += name_len;

                // Read bio_len (4 bytes) then bio
                int bio_len;
                memcpy(&bio_len, page_data + pos, sizeof(bio_len));
                pos += sizeof(bio_len);

                string bio(page_data + pos, bio_len);
                pos += bio_len;

                // Create a Record object
                vector<string> fields = {to_string(id), name, bio, to_string(manager_id)};
                Record r(fields);
                records.push_back(r);
            }

            return true;
        }
    }
};

class StorageManager
{

public:
    string filename;     // Name of the file (EmployeeRelation.dat) where we will store the Pages
    fstream data_file;   // fstream to handle both input and output binary file operations
    vector<page> buffer; // You can have maximum of 3 Pages.

    // Constructor that opens a data file for binary input/output; truncates any existing data file
    StorageManager(const string &filename) : filename(filename)
    {
        data_file.open(filename, ios::binary | ios::out | ios::in | ios::trunc);
        if (!data_file.is_open())
        { // Check if the data_file was successfully opened
            cerr << "Failed to open data_file: " << filename << endl;
            exit(EXIT_FAILURE); // Exit if the data_file cannot be opened
        }
    }

    // Destructor closes the data file if it is still open
    ~StorageManager()
    {
        if (data_file.is_open())
        {
            data_file.close();
        }
    }

    // Reads data from a CSV file and writes it to EmployeeRelation.dat
    void createFromFile(const string &csvFilename)
    {
        buffer.resize(3); // You can have maximum of 3 Pages.

        ifstream csvFile(csvFilename); // Open the Employee.csv file for reading

        string line, name, bio;
        int id, manager_id;
        int page_number = 0; // Current page we are working on [at most 3 pages]

        while (getline(csvFile, line))
        { // Read each line from the CSV file, parse it, and create Employee objects
            stringstream ss(line);
            string item;
            vector<string> fields;

            while (getline(ss, item, ','))
            {
                fields.push_back(item);
            }
            Record r = Record(fields); // create a record object

            if (!buffer[page_number].insert_record_into_page(r))
            { // inserting that record object to the current page

                // Current page is full, move to the next page
                page_number++;

                if (page_number >= buffer.size())
                { // Checking if page limit has been reached.

                    for (page &p : buffer)
                    { // using write_into_data_file() to write the pages into the data file
                        p.write_into_data_file(data_file);
                    }

                    // Reset for next set of pages
                    for (page &p : buffer)
                    {
                        p.records.clear();
                        p.slot_directory.clear();
                        p.cur_size = 0;
                    }

                    page_number = 0; // Starting again from page 0
                }
                buffer[page_number].insert_record_into_page(r); // Reattempting the insertion of record 'r' into the newly created page
            }
        }
        csvFile.close(); // Close the CSV file
    }

    // Searches for an Employee ID in EmployeeRelation.dat
    void findAndPrintEmployee(int searchId)
    {
        data_file.clear();
        data_file.seekg(0, ios::beg); // Rewind the data_file to the beginning for reading

        // clear buffer pages
        for (page &p : buffer)
        {
            p.records.clear();
            p.slot_directory.clear();
            p.cur_size = 0;
        }
        // TO_DO: Read pages from your data file (using read_from_data_file) and search for the employee ID in those pages. Be mindful of the page limit in main memory.
        int page_number = 0;
        bool found = false;

        while (buffer[page_number].read_from_data_file(data_file))
        {
            for (const Record &r : buffer[page_number].records)
            {
                if (r.id == searchId)
                {
                    r.print();
                    return;
                }
            }
            page_number++;
            if (page_number >= buffer.size())
            {
                page_number = 0;
                // clear buffer pages
                for (page &p : buffer)
                {
                    p.records.clear();
                    p.slot_directory.clear();
                    p.cur_size = 0;
                }
            }
        }
        // TO_DO: Print "Record not found" if no records match.
        if (!found)
        {
            cout << "Record not found" << endl;
        }
    }
};
