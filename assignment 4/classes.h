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
static const int32_t MAIN_INDEX_PAGES_RESERVED = 5000;   // pages 0..4999 reserved for header + primary buckets
static const char *INDEX_FILENAME = "EmployeeIndex.dat"; // output file must be EmployeeIndex.dat

class Record
{
public:
    int64_t id, manager_id; // Employee ID and their manager's ID
    std::string bio, name;  // Fixed length string to store employee name and biography

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
        return (int)sizeof(int64_t) + (int)sizeof(int64_t) + (int)sizeof(int32_t) + (int)name.size() + (int)sizeof(int32_t) + (int)bio.size();
    }

    string serialize() const
    {
        std::ostringstream oss(std::ios::out | std::ios::binary);

        oss.write(reinterpret_cast<const char *>(&id), sizeof(id));                 // 8 bytes
        oss.write(reinterpret_cast<const char *>(&manager_id), sizeof(manager_id)); // 8 bytes

        int32_t name_len = (int32_t)name.size();
        int32_t bio_len = (int32_t)bio.size();

        oss.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len)); // 4 bytes
        oss.write(name.data(), name_len);                                       // name_len bytes

        oss.write(reinterpret_cast<const char *>(&bio_len), sizeof(bio_len)); // 4 bytes
        oss.write(bio.data(), bio_len);                                       // bio_len bytes

        return oss.str(); // Returns the serialized string representation of the record.
    }

    // deserialize from raw byte, avoids scanning the page and needed for correct slot directory usage
    static bool deserialize_from_bytes(const char *page_data, int32_t offset, int32_t length, Record &out)
    {
        // Reconstruct record from bytes inside a page using (offset,length) from slot directory
        if (offset < 0 || length <= 0)
            return false;

        int32_t pos = offset;          // read pointer in record byte range
        int32_t end = offset + length; // first byte after record

        int64_t rid = 0;  // temp record id
        int64_t rmid = 0; // temp record manager id

        int32_t nl = 0;
        int32_t bl = 0;

        if (pos + (int32_t)sizeof(int64_t) * 2 + (int32_t)sizeof(int32_t) * 2 > end)
            return false; // minimum size check for id, manager_id, name_len, bio_len

        // read id
        memcpy(&rid, page_data + pos, sizeof(int64_t));
        pos += (int32_t)sizeof(int64_t);

        // read manager_id
        memcpy(&rmid, page_data + pos, sizeof(int64_t));
        pos += (int32_t)sizeof(int64_t);

        // read name length
        memcpy(&nl, page_data + pos, sizeof(int32_t));
        pos += (int32_t)sizeof(int32_t);
        if (nl < 0 || pos + nl > end)
            return false;

        // read name bytes
        std::string name(page_data + pos, page_data + pos + nl);
        pos += nl;

        // read bio length
        memcpy(&bl, page_data + pos, sizeof(int32_t));
        pos += (int32_t)sizeof(int32_t);
        if (bl < 0 || pos + bl > end)
            return false;

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
    vector<Record> records;                        // Data Area: Stores records.
    vector<pair<int32_t, int32_t>> slot_directory; // This slot directory contains the starting position (offset), and size of the record.

    int32_t cur_size = 0;              // holds the current size of the page
    int32_t slot_directory_offset = 0; // offset where the slot directory starts in the page
    int32_t overflow_page_idx = -1;    // points to overflow page index
    // Function to insert a record into the page
    bool insert_record_into_page(const Record &r)
    {
        int32_t record_size = (int32_t)r.get_size();
        const int32_t slot_entry_size = (int32_t)(sizeof(int32_t) * 2); // offset + length
        const int32_t metadata_size = (int32_t)(sizeof(int32_t) * 2);   // num_slots + overflow_page_idx

        // bytes left for record area after accounting for slot dir
        int32_t available_space = PAGE_SIZE - metadata_size - (int32_t)((slot_directory.size() + 1) * slot_entry_size);

        if (cur_size + record_size > available_space)
            // if page size limit exceeded, fail
            return false;

        int32_t offset = cur_size; // record starts at current end of record area
        records.push_back(r);
        slot_directory.push_back(std::make_pair(offset, record_size));
        cur_size += record_size; // advance end-of-record-area pointer

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
        in.read(page_data, PAGE_SIZE); // Read a page of 4 KB from the data file
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
        if (num_slots < 0 || num_slots > (PAGE_SIZE / 8))
            return false;

        // Calculate where slot directory starts based on num_slots
        int32_t slot_bytes = num_slots * (int32_t)sizeof(int32_t) * 2; // offset + length
        int32_t data_end = pos - slot_bytes;                           // first byte of slot directory
        if (data_end < 0)
            return false; // invalid slot size

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

// Metadata structure stored in page 0
struct IndexMetaData
{
    int32_t level; // current hash level i
    int32_t n;     // number of active primary buckets
    int32_t total_records;
    int32_t next_overflow_page;
};

static const int32_t META_PAGE = 0;     // page 0 stores metadata
static const int32_t DATA_START = 1;    // first primary bucket is at page 1
static const int32_t INITIAL_N = 4;     // start with 4 buckets?
static const double LOAD_FACTOR = 0.70; // for 70 percent full

// metadata goes in page 0
static void write_meta(fstream &f, const IndexMetaData &m)
{
    char buf[PAGE_SIZE];
    std::memset(buf, 0, PAGE_SIZE);

    std::memcpy(buf + 0, &m.level, sizeof(int32_t));
    std::memcpy(buf + 4, &m.n, sizeof(int32_t));
    std::memcpy(buf + 8, &m.total_records, sizeof(int32_t));
    std::memcpy(buf + 12, &m.next_overflow_page, sizeof(int32_t));

    // Write overflow=-1 in footer
    int32_t overflowp = -1;
    std::memcpy(buf + PAGE_SIZE - 4, &overflowp, sizeof(int32_t));

    f.seekp((int64_t)META_PAGE * PAGE_SIZE, std::ios::beg);
    f.write(buf, PAGE_SIZE);
}

// Write a page at a specific page index
static void write_page_at(fstream &f, int32_t idx, const page &p)
{
    f.seekp((int64_t)idx * PAGE_SIZE, std::ios::beg);
    p.write_into_data_file(f);
}

// Read a page at a specific page index
static bool read_page_at(fstream &f, int32_t idx, page &p)
{
    f.clear();
    f.seekg((int64_t)idx * PAGE_SIZE, std::ios::beg);
    return p.read_from_data_file(f);
}

// h_i(id) = id mod 2^i
static int32_t hash_level(int64_t id, int32_t level)
{
    int64_t mod = (int64_t)1 << level;
    return (int32_t)(((id % mod) + mod) % mod);
}

// Determine which bucket an id maps to given current (level, n)
static int32_t bucket_for(int64_t id, int32_t level, int32_t n)
{
    int32_t h = hash_level(id, level);
    return (h < n) ? h : hash_level(id, level - 1);
}

// chaange bucket number to page index in file
static int32_t page_index_for_bucket(int32_t bucket)
{
    return bucket + DATA_START; // offfset by 1 since page 0 is for metadata
}

// Allocate a new overflow page
static int32_t alloc_overflow_page(fstream &f, IndexMetaData &meta)
{
    if (meta.next_overflow_page < MAIN_INDEX_PAGES_RESERVED)
        meta.next_overflow_page = MAIN_INDEX_PAGES_RESERVED;

    int32_t idx = meta.next_overflow_page++;
    page empty;
    write_page_at(f, idx, empty);
    return idx;
}

// Insert a record into a bucket chain (primary + overflow pages)
static void insert_into_chain(fstream &f, int32_t page_idx, const Record &r, IndexMetaData &meta, page &p)
{
    int32_t cur = page_idx;

    while (true)
    {
        page p;
        if (!read_page_at(f, cur, p))
        {
            // Page doesn't exist yet
            p.records.clear();
            p.slot_directory.clear();
            p.cur_size = 0;
            p.overflow_page_idx = -1;
            write_page_at(f, cur, p);
            read_page_at(f, cur, p);
        }

        if (p.insert_record_into_page(r))
        {
            write_page_at(f, cur, p);
            return;
        }

        // Page is full. follow overflow pointer or create one
        if (p.overflow_page_idx != -1)
        {
            cur = p.overflow_page_idx;
        }
        else
        {
            // make new overflow page
            int32_t overflowp = alloc_overflow_page(f, meta);
            // ^ p becomes empty page, gotta read again
            read_page_at(f, cur, p);
            p.overflow_page_idx = overflowp;
            write_page_at(f, cur, p);
            cur = overflowp;
        }
    }
}

static void split_bucket(fstream &f, IndexMetaData &meta)
{
    // s = n - 2^level (the bucket that hasnt been split yet)
    int32_t s = meta.n - (1 << meta.level);
    int32_t old_page_idx = page_index_for_bucket(s);

    // Increment n and level FIRST
    meta.n += 1;
    if (meta.n == (1 << (meta.level + 1)))
        meta.level += 1;

    /*
(0) load the old page, save a local record that needs deleting.
(1) Get the index position of that record in the page's records vector
(2) erase the record from the records array at that index position
(3) erase the slot from the slot_directory at that index position
(4) write the page
(5) load the new page
(6) add the record to the new page, include overflow page if necessary (record is the local one you stored in step 0)
(7) write the new page
*/

    int32_t cur = old_page_idx;
    page p;
    while (cur != -1)
    {
        // Read old page
        read_page_at(f, cur, p);
        int32_t next_overflow = p.overflow_page_idx;

        // Collect all records from this page FIRST
        vector<Record> records_to_move;
        for (const auto &rec : p.records)
        {
            records_to_move.push_back(rec);
        }

        // Clear the old page completely
        p.records.clear();
        p.slot_directory.clear();
        p.cur_size = 0;
        p.overflow_page_idx = -1;
        write_page_at(f, cur, p); // Write once per page, not once per record!

        // Now redistribute all records
        for (const auto &local_record : records_to_move)
        {
            int32_t new_bucket = bucket_for(local_record.id, meta.level, meta.n);
            int32_t new_page_idx = page_index_for_bucket(new_bucket);
            insert_into_chain(f, new_page_idx, local_record, meta, p);
        }

        // Move to next overflow page in the old chain
        cur = next_overflow;
    }
}

// int32_t cur = old_page_idx;
// page p;
// while (cur != -1)
// {
//     // Read old page
//     read_page_at(f, cur, p);
//     int32_t next_overflow = p.overflow_page_idx;

//     // Process all records on this page
//     while (!p.records.empty())
//     {
//         // (0) Save the record locally
//         Record local_record = p.records.back();

//         // (1)(2)(3) Remove from old page
//         int32_t idx = (int32_t)p.records.size() - 1;
//         p.records.erase(p.records.begin() + idx);
//         p.slot_directory.erase(p.slot_directory.begin() + idx);

//         // update cur_size to be the max offset+length of records, free space pointer
//         p.cur_size = 0;
//         for (const auto &slot : p.slot_directory)
//         {
//             p.cur_size = std::max(p.cur_size, slot.first + slot.second);
//         }

//         // (4)
//         write_page_at(f, cur, p);

//         // new bucket for this record?
//         int32_t new_bucket = bucket_for(local_record.id, meta.level, meta.n);
//         int32_t new_page_idx = page_index_for_bucket(new_bucket);

//         // (5)(6)(7) Insert into new location (handles overflow if needed)
//         insert_into_chain(f, new_page_idx, local_record, meta, p);

//         // Re-read the old page for next iteration
//         read_page_at(f, cur, p);
//     }

//     // Clear this page completely (all records redistributed)
//     p.records.clear();
//     p.slot_directory.clear();
//     p.cur_size = 0;
//     p.overflow_page_idx = -1;
//     write_page_at(f, cur, p);

//     // Move to next overflow page in the old chain
//     cur = next_overflow;
// }

class LinearHashIndex
{
    IndexMetaData meta;
    string indexFilename;
    string filename; // Name of the file (EmployeeRelation.dat) where we will store the Pages
    fstream data_file;
    page current_page;

public:
    // Insert one record into the index
    void insert_into_index(const Record &r)
    {
        // Find destination bucket and insert
        int32_t b = bucket_for(r.id, meta.level, meta.n);
        int32_t pidx = page_index_for_bucket(b);
        insert_into_chain(data_file, pidx, r, meta, current_page);
        meta.total_records++;

        // Check if we need to split
        double avg_bytes = 400.0;
        double usable = (double)(PAGE_SIZE - 8); // 8 bytes for num_slots + overflow_page_idx
        double cap = usable / avg_bytes;
        double threshold = LOAD_FACTOR * cap * (double)meta.n;

        while ((double)meta.total_records > threshold &&
               meta.n < MAIN_INDEX_PAGES_RESERVED - DATA_START)
        {
            split_bucket(data_file, meta);
            threshold = LOAD_FACTOR * cap * (double)meta.n;
        }

        // write metadata every 100 records
        if (meta.total_records % 100 == 0)
        {
            write_meta(data_file, meta);
        }
    }

    // Constructor that opens a data file for binary input/output; truncates any existing data file
    LinearHashIndex(const string &filename) : filename(filename)
    {
        data_file.open(filename, ios::binary | ios::out | ios::in | ios::trunc);
        if (!data_file.is_open())
        { // Check if the data_file was successfully opened
            cerr << "Failed to open data_file: " << filename << endl;
            exit(EXIT_FAILURE); // Exit if the data_file cannot be opened
        }
        // write metadata
        meta.level = 1; // h_1(id) = id mod 2
        meta.n = INITIAL_N;
        meta.total_records = 0;
        meta.next_overflow_page = MAIN_INDEX_PAGES_RESERVED;

        // Pre-allocate all 5000 primary pages so bucket k is always at
        // a fixed offset. no bucket array
        {
            page empty;
            for (int32_t i = 0; i < MAIN_INDEX_PAGES_RESERVED; ++i)
            {
                data_file.seekp((int64_t)i * PAGE_SIZE, std::ios::beg);
                current_page.records.clear();
                current_page.slot_directory.clear();
                current_page.cur_size = 0;
                current_page.overflow_page_idx = -1;
                current_page.write_into_data_file(data_file);
            }
            data_file.flush();
        }

        write_meta(data_file, meta);
    }
    // Destructor closes the data file if it is still open
    ~LinearHashIndex()
    {
        if (data_file.is_open())
        {
            write_meta(data_file, meta); // persist metadata on close
            data_file.flush();
            data_file.close();
        }
    }

    // Reads data from a CSV file and writes it to EmployeeRelation.dat
    void createFromFile(const string &csvFilename)
    {

        ifstream csvFile(csvFilename); // Open the Employee.csv file for reading
        if (!csvFile.is_open())
        {
            cerr << "Failed to open CSV: " << csvFilename << endl;
            exit(EXIT_FAILURE);
        }

        string line;
        while (getline(csvFile, line))
        { // Read each line from the CSV file, parse it, and create Employee objects
            if (line.empty())
                continue; // Skip empty lines
            stringstream ss(line);
            string item;
            vector<string> fields;

            while (getline(ss, item, ','))
            {
                fields.push_back(item);
            }

            if (fields.size() != 4)
                continue;
            Record r(fields);
            insert_into_index(r);
        }

        csvFile.close(); // Close the CSV file
        cout << "Index built: " << meta.total_records << " records, "
             << meta.n << " buckets, level " << meta.level << "\n";
    }

    // Searches for an Employee ID in EmployeeRelation.dat
    void findAndPrintEmployee(int64_t searchId)
    {
        int32_t b = bucket_for((int64_t)searchId, meta.level, meta.n);
        int32_t cur = page_index_for_bucket(b);
        bool found = false;

        while (cur != -1)

        {
            page p;
            if (!read_page_at(data_file, cur, current_page))
                break;

            for (const auto &r : current_page.records)
            {
                if (r.id == searchId)
                {
                    r.print();
                    found = true;
                    return;
                }
            }
            cur = current_page.overflow_page_idx;
        }

        // TO_DO: Print "Record not found" if no records match.
        if (!found)
            cout << "Record not found\n";
    }
};
