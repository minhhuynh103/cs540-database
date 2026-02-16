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
#include <algorithm>
#include <cmath>
using namespace std; // Include the standard namespace

class Record
{
public:
    int64_t id, manager_id;    // Employee ID and their manager's ID
    std::string bio, name; // Fixed length string to store employee name and biography

    Record() : id(0), manager_id(0) {} // Default constructor

    Record(vector<std::string> &fields)
    {
        id = stoll(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoll(fields[3]);
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
    int get_size() const
    {
        // sizeof(int) is for name/bio size() in serialize function
        return (int)sizeof(int64_t) + (int)sizeof(int64_t) + (int)sizeof(int32_t) + (int)name.size()
        + (int)sizeof(int32_t) + (int)bio.size();
    }

    // Take a look at Figure 9.9 and read the Section 9.7.2 [Record Organization for Variable Length Records]
    // TODO: Consider using a delimiter in the serialize function to separate these items for easier parsing.
    // tabi here: i dont think we need a delimiter if we have a slot directory approach so i wouldn't worry about this? 
    //> I dont think delimiters are needed bc directory provides exact offsets/lengths
    string serialize() const
    {
        std::ostringstream oss(std::ios::out | std::ios::binary);

        oss.write(reinterpret_cast<const char*>(&id), sizeof(id));                 // 8 bytes
        oss.write(reinterpret_cast<const char*>(&manager_id), sizeof(manager_id)); // 8 bytes

        int32_t name_len = (int32_t)name.size();
        int32_t bio_len  = (int32_t)bio.size();

        oss.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));    // 4 bytes
        oss.write(name.data(), name_len);                                         // name_len bytes

        oss.write(reinterpret_cast<const char*>(&bio_len), sizeof(bio_len));      // 4 bytes
        oss.write(bio.data(), bio_len);                                           // bio_len bytes

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
        const int32_t slot_entry_size = (int32_t)(sizeof(int32_t) * 2);   // offset + length
        const int32_t metadata_size   = (int32_t)(sizeof(int32_t) * 2);   // num_slots + free_space_ptr

        // bytes left for record area after accounting for slot dir 
        int32_t available_space = 4096 - metadata_size - (int32_t)( (slot_directory.size() + 1) * slot_entry_size);

        if (cur_size + record_size > available_space)
        // if page size limit exceeded, fail
            return false;
        
        int32_t offset = cur_size;                       // record starts at current end of record area
        records.push_back(r);
        slot_directory.push_back(std::make_pair(offset, record_size));
        cur_size += record_size;                         // advance end-of-record-area pointer

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

        char page_data[4096]; // Write the page contents into this char array so that the page can be written to the data file in one go.

        std::memset(page_data, 0, sizeof(page_data));

        // write record at the offset stored in slot directory
        if (records.size() != slot_directory.size()) 
        {
            return;
        }

        for (size_t i = 0; i < records.size(); ++i) 
        {
            std::string serialized = records[i].serialize();
            int32_t off = slot_directory[i].first;
            std::memcpy(page_data + off, serialized.data(), serialized.size());
        }

        // Start slot directory at offset 4096 - (slot_directory.size() * 8) and write backwards from the end
        int32_t num_slots = (int32_t)slot_directory.size();
        int32_t free_space_ptr = cur_size;

        // End of page data: num_slots at 4088..4091, free_space_ptr @ 4092..4095
        std::memcpy(page_data + 4088, &num_slots, sizeof(num_slots));
        std::memcpy(page_data + 4092, &free_space_ptr, sizeof(free_space_ptr));

        // Write slots backward from 4088
        int slot_offset = 4088;
        for (int i = (int)slot_directory.size() - 1; i >= 0; --i)
        {
            slot_offset -= (int)sizeof(int32_t);
            memcpy(page_data + slot_offset, &slot_directory[i].second, sizeof(int32_t));

            slot_offset -= (int)sizeof(int32_t);
            memcpy(page_data + slot_offset, &slot_directory[i].first, sizeof(int32_t));
        }

        out.write(page_data, sizeof(page_data)); // Write the page_data to the EmployeeRelation.dat file
    }

    // Read a page from a binary input stream, i.e., EmployeeRelation.dat file to populate a page object
    bool read_from_data_file(istream &in)
    {
        char page_data[4096];
        in.read(page_data, 4096);   // Read a page of 4 KB from the data file
        std::streamsize bytes_read = in.gcount();

        if (bytes_read == 0)
        return false; // clean EOF

        if (bytes_read != 4096)
        return false; // reject partial page reads

        // Reset in-memory page state before populating
        records.clear();
        slot_directory.clear();
        cur_size = 0;

        // Read metadata from the end
        int32_t num_slots = 0;
        int32_t free_space_ptr = 0;
        std::memcpy(&num_slots, page_data + 4088, sizeof(num_slots));
        std::memcpy(&free_space_ptr, page_data + 4092, sizeof(free_space_ptr));
        cur_size = free_space_ptr;

        // Validate metadata to prevent corrupt pages 
        if (num_slots < 0 || num_slots > 4096 / 8) return false;       // each slot is 8 bytes (offset+len)
        if (free_space_ptr < 0 || free_space_ptr > 4088) return false; // records must end before slot/metadata 

        const int32_t data_end = 4088 - num_slots * 8;
        if (data_end < 0) return false;              // invalid slot directory
        if (free_space_ptr > data_end) return false; // ensure free space pointer is within data area

        // Read slot directory (working backward from 4088)
        int slot_offset = 4088;
        for (int i = num_slots - 1; i >= 0; --i)
        {
            int32_t length = 0;
            int32_t offset = 0;

            slot_offset -= (int)sizeof(int32_t);
            std::memcpy(&length, page_data + slot_offset, sizeof(int32_t));

            slot_offset -= (int)sizeof(int32_t);
            std::memcpy(&offset, page_data + slot_offset, sizeof(int32_t));

            if (offset < 0 || length < 0) return false;     // invalid slot entry
            if (offset + length > data_end) return false;   // record must fit in data area

            // Insert at beginning to maintain order (since we're reading backward)
            slot_directory.insert(slot_directory.begin(), std::make_pair(offset, length));
        }

        // Now deserialize each record using the slot directory
        for (const auto &slot : slot_directory)
        {
            int32_t rec_offset = slot.first;
            int32_t pos = rec_offset;

            int64_t id = 0;
            int64_t manager_id = 0;
            int32_t name_len = 0;
            int32_t bio_len = 0;

            // Read id 
            std::memcpy(&id, page_data + pos, sizeof(id));
            pos += (int32_t)sizeof(id);

            // Read manager_id 
            std::memcpy(&manager_id, page_data + pos, sizeof(manager_id));
            pos += (int32_t)sizeof(manager_id);

            // Read name_len then name
            std::memcpy(&name_len, page_data + pos, sizeof(name_len));
            pos += (int32_t)sizeof(name_len);

            std::string name(page_data + pos, page_data + pos + name_len);
            pos += name_len;

            // Read bio_len then bio
            std::memcpy(&bio_len, page_data + pos, sizeof(bio_len));
            pos += (int32_t)sizeof(bio_len);

            std::string bio(page_data + pos, page_data + pos + bio_len);
            pos += bio_len;

            // Create a Record object
            std::vector<std::string> fields = {std::to_string(id), name, bio, std::to_string(manager_id)};
            records.emplace_back(fields);
        }

        return true;
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
        data_file.seekg(0, std::ios::beg); // Rewind the data_file to the beginning for reading

        // TO_DO: Read pages from your data file (using read_from_data_file) and search for the employee ID in those pages. Be mindful of the page limit in main memory.
        page p;

        while (p.read_from_data_file(data_file))
        {
            for (const auto &r : p.records)
            {
                if (r.id == searchId)
                {
                    r.print();
                    return;
                }
            }
        }
        
        // TO_DO: Print "Record not found" if no records match.
        std::cout << "Record not found\n";
    }
};
