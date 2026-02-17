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

static const int32_t PAGE_SIZE = 4096;
static const int32_t MAIN_INDEX_PAGES_RESERVED = 5000; // pages 0..4999 reserved for header + primary buckets
static const char*   INDEX_FILENAME = "EmployeeIndex.dat"; // output file must be EmployeeIndex.dat

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

    // deserialize from raw byte, avoids scanning the page and needed for correct slot directory usage
    static bool deserialize_from_bytes(const char* page_data, int32_t offset, int32_t length, Record& out)
    {
        // Reconstruct record from bytes inside a page using (offset,length) from slot directory
        if (offset < 0 || length <= 0) return false;

        int32_t pos = offset;           // read pointer in record byte range
        int32_t end = offset + length;  // first byte after record

        int64_t rid = 0;    // temp record id
        int64_t rmid = 0;   // temp record manager id

        int32_t nl = 0;
        int32_t bl = 0;

        if (pos + (int32_t)sizeof(int64_t)*2 + (int32_t)sizeof(int32_t)*2 > end) return false; // minimum size check for id, manager_id, name_len, bio_len
        
        // read id
        memcpy(&rid, page_data + pos, sizeof(int64_t));
        pos += (int32_t)sizeof(int64_t);

        // read manager_id
        memcpy(&rmid, page_data + pos, sizeof(int64_t));
        pos += (int32_t)sizeof(int64_t);

        // read name length 
        memcpy(&nl, page_data + pos, sizeof(int32_t));
        pos += (int32_t)sizeof(int32_t);
        if (nl < 0 || pos + nl > end) return false;

        // read name bytes
        std::string name(page_data + pos, page_data + pos + nl);
        pos += nl;

        // read bio length
        memcpy(&bl, page_data + pos, sizeof(int32_t));
        pos += (int32_t)sizeof(int32_t);
        if (bl < 0 || pos + bl > end) return false;

        // read bio bytes
        std::string bio(page_data + pos, page_data + pos + bl);
        pos += bl;

        // fill output record
        out.id = rid;
        out.manager_id = rmid;
        out.name = name;
        out.bio = bio;

    return true;
    }
};

class page
{ // Take a look at Figure 9.7 and read Section 9.6.2 [Page organization for variable length records]
public:
    vector<Record> records;                         // Data Area: Stores records.
    vector<pair<int32_t, int32_t>> slot_directory;  // This slot directory contains the starting position (offset), and size of the record.

    int32_t cur_size = 0;              // holds the current size of the page
    int32_t slot_directory_offset = 0; // offset where the slot directory starts in the page
    int32_t overflow_page_idx = -1;    // points to overflow page index
    // Function to insert a record into the page
    bool insert_record_into_page(const Record &r)
    {
        int32_t record_size = (int32_t)r.get_size();
        const int32_t slot_entry_size = (int32_t)(sizeof(int32_t) * 2);   // offset + length
        const int32_t metadata_size   = (int32_t)(sizeof(int32_t) * 2);   // num_slots + overflow_page_idx

        // bytes left for record area after accounting for slot dir 
        int32_t available_space = PAGE_SIZE - metadata_size - (int32_t)((slot_directory.size() + 1) * slot_entry_size);

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
    [overflow_page_idx: 4 bytes]
 */

    // Function to write the page to a binary file, i.e., EmployeeRelation.dat file
    void write_into_data_file(ostream &out) const
    {

        char page_data[PAGE_SIZE]; // Write the page contents into this char array so that the page can be written to the data file in one go.

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
        int32_t pos = PAGE_SIZE;

        // write overflow_page_idx into last 4 bytes
        pos -= (int32_t)sizeof(int32_t);
        std::memcpy(page_data + pos, &overflow_page_idx, sizeof(int32_t));

        // write num_slots before overflow_page_idx
        pos -= (int32_t)sizeof(int32_t);
        std::memcpy(page_data + pos, &num_slots, sizeof(int32_t));

        // Write slot directory right before num_slots, in reverse order
        for (int i = num_slots - 1; i >= 0; --i)
        {
            // length
            pos -= (int32_t)sizeof(int32_t);
            std::memcpy(page_data + pos, &slot_directory[i].second, sizeof(int32_t));

            // offset
            pos -= (int32_t)sizeof(int32_t);
            std::memcpy(page_data + pos, &slot_directory[i].first, sizeof(int32_t));
        }

        // write page to file
        out.write(page_data, sizeof(page_data));
    }

    // Read a page from a binary input stream, i.e., EmployeeRelation.dat file to populate a page object
    bool read_from_data_file(istream &in)
    {
        char page_data[PAGE_SIZE];
        in.read(page_data, PAGE_SIZE);   // Read a page of 4 KB from the data file
        std::streamsize bytes_read = in.gcount();

        if (bytes_read == 0)
        return false; // clean EOF

        if (bytes_read != PAGE_SIZE)
        return false; // reject partial page reads

        // Reset in-memory page state before populating
        records.clear();
        slot_directory.clear();
        cur_size = 0;
        overflow_page_idx = -1;

        // read metadata at end of page 
        int32_t pos = PAGE_SIZE;

        // read overflow_page_idx (last 4 bytes)
        pos -= (int32_t)sizeof(int32_t);
        std::memcpy(&overflow_page_idx, page_data + pos, sizeof(int32_t));

        // read num_slots (4 bytes before overflow_page_idx)
        pos -= (int32_t)sizeof(int32_t);
        int32_t num_slots = 0;
        std::memcpy(&num_slots, page_data + pos, sizeof(int32_t));

        // validate num_slots (each slot entry = 8 bytes)
        if (num_slots < 0 || num_slots > (PAGE_SIZE / 8)) return false;

        // Calculate where slot directory starts based on num_slots
        int32_t slot_bytes = num_slots * (int32_t)sizeof(int32_t) * 2; // offset + length
        int32_t data_end = pos - slot_bytes;            // first byte of slot directory
        if (data_end < 0) return false;                 // invalid slot size

        // read slot directory entries
        int32_t slot_pos = pos;
        for (int i = num_slots - 1; i >= 0; --i)
        {
            int32_t length = 0;
            int32_t offset = 0;

            // length
            slot_pos -= (int32_t)sizeof(int32_t);
            std::memcpy(&length, page_data + slot_pos, sizeof(int32_t));

            // offset
            slot_pos -= (int32_t)sizeof(int32_t);
            std::memcpy(&offset, page_data + slot_pos, sizeof(int32_t));

            if (offset < 0 || length <= 0) 
            return false;
            if (offset + length > data_end) 
            return false;

             // Insert at beginning to maintain order
            slot_directory.insert(slot_directory.begin(), std::make_pair(offset, length));

            // update cur_size to be the max offset+length of records, free space pointer 
            cur_size = std::max(cur_size, offset + length);
        }

        // deserialize records using slot directory
        for (const auto &slot : slot_directory)
        {
            Record r;
            if (!Record::deserialize_from_bytes(page_data, slot.first, slot.second, r))
                return false;
            records.push_back(r);
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
