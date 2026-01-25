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
#include <cstdint>
#include <cstring>
using namespace std; // Include the standard namespace

class Record
{
public:
    int64_t id, manager_id;    // Employee ID and their manager's ID
    std::string bio, name; // Fixed length string to store employee name and biography

    Record(vector<std::string> &fields)
    {
        id = stoll(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoll(fields[3]);
    }

    // You may use this for debugging / showing the record to standard output.
    void print()
    {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    // Function to get the size of the record
    int get_size() const
    {
        // sizeof(int) is for name/bio size() in serialize function
        return (int)(sizeof(int64_t) + sizeof(int64_t) + sizeof(int32_t) + name.size() + sizeof(int32_t) + bio.size());
    }

    // Take a look at Figure 9.9 and read the Section 9.7.2 [Record Organization for Variable Length Records]
    // TODO: Consider using a delimiter in the serialize function to separate these items for easier parsing.
    // tabi here: i dont think we need a delimiter if we have a slot directory approach so i wouldn't worry about this? 
    //> I dont think delimiters are needed bc directory provides exact offsets/lengths
    string serialize() const
    {
        ostringstream oss(std::ios::binary);

        oss.write(reinterpret_cast<const char *>(&id), sizeof(id));                 // Writes the binary representation of the ID.
        oss.write(reinterpret_cast<const char *>(&manager_id), sizeof(manager_id)); // Writes the binary representation of the Manager id
        int32_t name_len = name.size();
        int32_t bio_len = bio.size();
        oss.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len));     // Writes the size of the Name in binary format.
        oss.write(name.data(), name_len);                                           // writes the name in binary form
        oss.write(reinterpret_cast<const char *>(&bio_len), sizeof(bio_len));       // Writes the size of the Bio in binary format.
        oss.write(bio.data(), bio_len);                                             // writes bio in binary form

        return oss.str(); // Returns the serialized string representation of the record.
    }
};

class page
{ // Take a look at Figure 9.7 and read Section 9.6.2 [Page organization for variable length records]
public:
    vector<Record> records;                         // Data Area: Stores records.
    vector<pair<int32_t, int32_t>> slot_directory;  // This slot directory contains the starting position (offset), and size of the record.

    int32_t cur_size = 0;              // holds the current size of the page
    int32_t slot_directory_offset = 0; // offset where the slot directory starts in the page

    // Function to insert a record into the page
    bool insert_record_into_page(Record r)
    {
        int32_t record_size = (int32_t)r.get_size();
        const int32_t slot_entry_size = 8; // offset(4) + length(4)
        const int32_t metadata_size   = 8; // free_space_ptr(4) + num_slots(4)

        int32_t num_slots_after = (int32_t)slot_directory.size() + 1;

        // Total space needed after insertion:
        int32_t needed = cur_size + record_size + (num_slots_after * slot_entry_size) + metadata_size;

        if (needed > 4096)
        return false; // Not enough space in this page

            // TO_DO: update slot directory information
            // 1. Calculate the offset for this record (where it starts in the page)
            //    - This should be the sum of all previous record sizes
            //    - You can track this with a running offset variable
            // 2. Add a pair to slot_directory: (offset, record_size)
            //    Example: slot_directory.push_back(make_pair(offset, record_size));
            // 3. The slot index corresponds to the position in the records vector
            //    So slot 0 points to records[0], slot 1 to records[1], etc.

            // int offset = 0;
            // for (const auto &rec : records)
            // {
            //     offset += r.get_size();
            // }

        int32_t offset = cur_size;  // Since data grows forward, it starts at the current end of used data bytes.
        records.push_back(r);       // Add the record to the records vector
        // Add the new slot entry to stores where to find the record and how many bytes to read
        slot_directory.push_back(make_pair(offset, record_size)); 
        cur_size += record_size;    // Update the current size of the page        

        return true;
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
    [num slots: 4 bytes]
    [pointer to start of free space: 4 bytes]
 */

    // Function to write the page to a binary file, i.e., EmployeeRelation.dat file
    void write_into_data_file(ostream &out) const
    {

        char page_data[4096]; // Write the page contents (records and slot directory) into this char array so that the page can be written to the data file in one go.

        memset(page_data, 0, sizeof(page_data)); // Used as an iterator to indicate where the next item should be stored. Section 9.6.2 contains information that will help you with the implementation.

        // for (const auto &record : records)
        // { // Writing the records into the page_data
        //     string serialized = record.serialize();

        //     memcpy(page_data + offset, serialized.c_str(), serialized.size());

        //     offset += serialized.size();
        // }

        for (size_t i = 0; i < records.size(); i++)
        {
            string serialized = records[i].serialize();
            int32_t off = slot_directory[i].first; // authoritative offset
            memcpy(page_data + off, serialized.data(), serialized.size());
        }

        // TO_DO: Put a delimiter here to indicate slot directory starts from here

        // Start slot directory at offset 4096 - (slot_directory.size() * 8) and write backwards from the end
        int32_t num_slots = (int32_t)slot_directory.size();
        int32_t free_space_ptr = cur_size;

        memcpy(page_data + 4096 - 4, &free_space_ptr, sizeof(free_space_ptr)); // Pointer to start of free space
        memcpy(page_data + 4096 - 8, &num_slots, sizeof(num_slots));           // Number of slots

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
        if (bytes_read == 4096)
        {

            // TO_DO: You may process page_data (4 KB page) and put the information to the records and slot_directory (main memory).
            // TO_DO: You may modify this function to process the search for employee ID in the page you just loaded to main memory.

            // Read metadata from the end
            int num_slots, free_space_ptr;
            memcpy(&free_space_ptr, page_data + 4092, sizeof(free_space_ptr));
            memcpy(&num_slots, page_data + 4088, sizeof(num_slots));

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
                int length = slot.second;

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

        if (bytes_read > 0)
        {
            cerr << "Incomplete read: Expected " << 4096 << " bytes, but only read " << bytes_read << " bytes." << endl;
        }

        return false;
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
        buffer.clear();
        buffer.resize(3); // You can have maximum of 3 Pages.

        ifstream csvFile(csvFilename); // Open the Employee.csv file for reading
        if (!csvFile.is_open()) {
            cerr << "Failed to open CSV: " << csvFilename << endl;
            exit(EXIT_FAILURE);
        }

        string line;
        int page_number = 0;

        while (getline(csvFile, line))
        { // Read each line from the CSV file, parse it, and create Employee objects
            if (line.empty()) continue; // Skip empty lines
            stringstream ss(line);
            string item;
            vector<string> fields;

            while (getline(ss, item, ','))
            {
                fields.push_back(item);
            }

            if (fields.size() != 4) continue;
            Record r(fields);

            if (!buffer[page_number].insert_record_into_page(r))
            { // inserting that record object to the current page

                // Current page is full, move to the next page
                page_number++;

                if (page_number >= (int)buffer.size())
                { // Checking if page limit has been reached.

                    for (page &p : buffer)
                    { // using write_into_data_file() to write the pages into the data file
                        if (!p.records.empty())
                            p.write_into_data_file(data_file);
                        p = page(); // reset so we don't rewrite old records
                    }
                    page_number = 0; // Starting again from page 0
                }
                
                if (!buffer[page_number].insert_record_into_page(r)) // Reattempting the insertion of record 'r' into the newly created page
                {
                    cerr << "Record too large for page." << endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

         // Flush remaining pages at end
        for (page &p : buffer)
        {
            if (!p.records.empty())
                p.write_into_data_file(data_file);
        }

        data_file.flush();
        csvFile.close(); // Close the CSV file
    }

    // Searches for an Employee ID in EmployeeRelation.dat
    void findAndPrintEmployee(int64_t searchId)
    {
        data_file.clear(); // Clear any EOF flags
        data_file.seekg(0, ios::beg); // Rewind the data_file to the beginning for reading

        // TO_DO: Read pages from your data file (using read_from_data_file) and search for the employee ID in those pages. Be mindful of the page limit in main memory.
        page p;
        bool found = false;

        while (p.read_from_data_file(data_file))
        {
            for (const auto &r : p.records)
            {
                if (r.id == searchId)
                {
                    r.print();
                    found = true;
                }
            }
        }
        
        // TO_DO: Print "Record not found" if no records match.
        if (!found)
            cout << "Record not found\n";
    }
};
