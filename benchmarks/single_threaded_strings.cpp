/**
 * @file                single_threaded_strings.cpp
 * @author              Aditya Agarwal (aditya.agarwal@dumblebots.com)
 * @brief               Program to run benchmarks using strings on a single thread
 *
 * Usage: single_threaded_numbers <input_file> <oper1 [oper2...]>
 *
 * input_file:     Path to file containing records
 * oper:           Number of operations of each type to perform
 *
 * Example: single_threaded_strings ../random_all.in 50000 1000000
 */

// file and std IO
#include <iostream>
#include <fstream>
#include <cstring>

// measuring time
#include <chrono>

// tie
#include <tuple>

// table printing
#include <vector>

// std::unordered_set
#include <unordered_set>

// AgHashTable
#define AG_DBG_MODE
#include "AgHashTable.h"

constexpr uint64_t      maxStrLength   = 64ULL;        /** Maximum allowed length of a string */


struct Timer {

    private:

    std::chrono::high_resolution_clock::time_point      mStart;
    std::chrono::high_resolution_clock::time_point      mEnd;

    public:

    Timer ()
    {
        reset ();
    }

    int64_t
    elapsed_ms ()
    {
        mEnd        = std::chrono::high_resolution_clock::now ();
        auto diff   = std::chrono::duration_cast<std::chrono::milliseconds> (mEnd - mStart).count ();

        return diff;
    }

    int64_t
    elapsed_us ()
    {
        mEnd        = std::chrono::high_resolution_clock::now ();
        auto diff   = std::chrono::duration_cast<std::chrono::microseconds> (mEnd - mStart).count ();

        return diff;
    }

    int64_t
    elapsed_ns ()
    {
        mEnd        = std::chrono::high_resolution_clock::now ();
        auto diff   = std::chrono::duration_cast<std::chrono::nanoseconds> (mEnd - mStart).count ();

        return diff;
    }

    void
    reset ()
    {
        mStart      = std::chrono::high_resolution_clock::now ();
    }
};

struct table {

    private:

    std::vector<std::string>                mHeaders;
    std::vector<std::vector<std::string>>   mRows;

    public:

    table ()
    {}

    void
    add_headers (std::initializer_list<std::string> pHeaders)
    {
        if (pHeaders.size () == 0)
        {
            std::cout << "ZERO COLOUMNS NOT ALLOWED IN TABLE\n";
            std::exit (1);
        }
        mHeaders            = pHeaders;
    }

    void
    add_row (std::initializer_list<std::string> pElems)
    {
        if (pElems.size () != mHeaders.size ())
        {
            std::cout << "NUMBER OF COLOUMNS IN ROW MUST MATCH NUMBER OF COLOUMNS IN HEADER\n";
            std::exit (1);
        }

        mRows.push_back (pElems);
    }

    friend std::ostream
    &operator<< (std::ostream &stream, const table &pOther)
    {
        int32_t                 cols    = (int32_t)pOther.mHeaders.size ();
        int32_t                 width   = 0;

        std::vector<int32_t>    sz (cols);

        for(int32_t col = 0; col < cols; ++col) {
            sz[col] = (int32_t)pOther.mHeaders[col].size ();
        }

        for (auto &e : pOther.mRows) {
            for (int32_t col = 0; col < cols; ++col) {

                sz[col] = std::max (sz[col], (int32_t)e[col].size ());
            }
        }

        for (auto &e : sz) {
            e       += 4;
            width   += e;
        }

        // print the headers
        for (int32_t i = 0; i < width; ++i) {
            stream << '-';
        }
        stream << '-' << '\n';

        for (int32_t i = 0; i < cols; ++i) {
            stream << "| ";
            stream << pOther.mHeaders[i];
            for (int32_t pad = (int32_t)pOther.mHeaders[i].size () + 2; pad < sz[i]; ++pad) {
                stream << ' ';
            }
        }
        stream << '|' << '\n';

        for (int32_t i = 0; i < width; ++i) {
            stream << '-';
        }
        stream << '-' << '\n';

        for (auto &row : pOther.mRows) {
            for (int32_t i = 0; i < cols; ++i) {
                stream << "| ";
                stream << row[i];
                for (int32_t pad = (int32_t)row[i].size () + 2; pad < sz[i]; ++pad) {
                    stream << ' ';
                }
            }
            stream << '|' << '\n';
        }

        if (pOther.mRows.size () == 0) {
            return stream;
        }

        for (int32_t i = 0; i < width; ++i) {
            stream << '-';
        }
        stream << '-' << '\n';

        return stream;
    }
};

bool
streq (const char *pA, const char *pB)
{
    for (size_t i = 0; ; ++i) {

        if (pA[i] == 0 && pB[i] == 0)
            break;

        if (pA[i] == pB[i])
            continue;

        else
            return 0;
    }

    return true;
}

template <typename T>
std::string
format_integer (T pNum)
{

    T           cpy {pNum};
    int32_t     len {};

    std::string res;

    if(pNum == 0) {
        return "0";
    }

    while (cpy) {
        ++len, cpy /= 10;
    }

    for (int32_t i = 0, d; i < len; ++i) {

        d = pNum % 10;
        pNum /= 10;

        res += (char) (d + '0');
        if (i % 3 == 2 && i != len - 1) {
            res += ',';
        }
    }

    for (auto i = 0; i < (int32_t)(res.size () / 2); ++i) {
        std::swap (res[i], res[res.size () - i - 1]);
    }

    return res;
}

char        **buff;

int32_t     maxN;

void
read_buffers (const char *pFilepath)
{
    std::ifstream       fin (pFilepath);

    if (!fin) {
        std::cout << "No file with name \"" << pFilepath << "\" exists" << std::endl;
        std::exit (-1);
    }
    fin.tie (NULL);

    fin >> maxN;
    fin.ignore (std::numeric_limits<std::streamsize>::max (), '\n');

    buff        = new (std::nothrow) char *[maxN];

    if (buff == nullptr) {
        std::cout << "Could not allocate buffer" << std::endl;
        std::exit (1);
    }

    std::cout << "Begin Reading File\n";
    std::cout << "Found " << format_integer (maxN) << " records each for Insert, Find and Erase\n";

    for (int32_t i = 0; i < maxN; ++i) {
        buff[i]     = new (std::nothrow) char[maxStrLength];
        fin.getline (buff[i], maxStrLength, '\n');

        // if ((i & 65'535) == 0) {
        //     std::cout << "\rReading Buffer " << ((100 * (int64_t)i) / maxN) << '%' << "  ";
        // }
    }

    std::cout << "\rDone Reading File\n";
}

uint64_t
fnv1a_string (const uint8_t *pPtr, const uint64_t &pLen)
{
    (void)pLen;
    return ag_fnv1a<uint64_t> (pPtr, strnlen ((const char *)pPtr, maxStrLength));
}

uint64_t
pearson_string (const uint8_t *pPtr, const uint64_t &pLen)
{
    (void)pLen;
    return ag_pearson_16_hash (pPtr, strnlen ((const char *)pPtr, maxStrLength));
}

void
run_benchmark (int pN)
{
    std::unordered_set<const char *>    table1;
    decltype (table1)::iterator         it1;

    AgHashTable<const char *, pearson_string>  table2;
    decltype (table2)::iterator         it2;

    Timer                               timer;
    int64_t                             measured;

    table                               results;
    table                               bucketInfo;
#ifdef AG_DBG_MODE
    table       agMetrics;
#endif

    int32_t                             flag;
    int32_t                             cntr;

    uint64_t                            memUsed;

    if (pN > maxN) {
        std::cout << "\nGiven " << format_integer (pN) << " operations exceeds the number of records supplied by the file\n";
        return;
    }

    std::cout << '\n';
    std::cout << format_integer (pN) << " Operations of each type\n";
    std::cout << '\n';

    results.add_headers ({"Operation", "Class", "Successful", "Time (ms)"});


    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        std::tie(std::ignore, flag) = table1.insert (buff[i]);
        cntr                        += flag;

        // measured                    = timer.elapsed_us ();
        // insTimeTotal1               += measured;
        // insTimeMin1                 = std::min (insTimeMin1, measured);
        // insTimeMax1                 = std::max (insTimeMax1, measured);

    }
    // results.add_row ({"Insertion", "std::unordered_set", format_integer (cntr), format_integer (insTimeTotal1 / 1'000)});
    measured    = timer.elapsed_ms ();
    results.add_row ({"Insertion", "std::unordered_set", format_integer (cntr), format_integer (measured)});

    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        flag                        = table2.insert (buff[i]);
        cntr                        += flag;

        // measured                    = timer.elapsed_us ();
        // insTimeTotal2               += measured;
        // insTimeMin2                 = std::min (insTimeMin2, measured);
        // insTimeMax2                 = std::max (insTimeMax2, measured);
    }
    // results.add_row ({"Insertion", "AgHashTable", format_integer (cntr), format_integer (insTimeTotal2 / 1'000)});
    measured    = timer.elapsed_ms ();
    results.add_row ({"Insertion", "AgHashTable", format_integer (cntr), format_integer (measured)});


    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        it1                         = table1.find (buff[i]);
        cntr                        += (int32_t)(it1 != table1.end ());

        // measured                    = timer.elapsed_us ();
        // findTimeTotal1              += measured;
        // findTimeMin1                = std::min (findTimeMin1, measured);
        // findTimeMax1                = std::max (findTimeMax1, measured);
    }
    // results.add_row ({"Find", "std::unordered_set", format_integer (cntr), format_integer (findTimeTotal1 / 1'000)});
    measured    = timer.elapsed_ms ();
    results.add_row ({"Find", "std::unordered_set", format_integer (cntr), format_integer (measured)});

    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        it2                         = table2.find (buff[i]);
        cntr                        += (int32_t)(it2 != table2.end ());

        // measured                    = timer.elapsed_us ();
        // findTimeTotal2              += measured;
        // findTimeMin2                = std::min (findTimeMin2, measured);
        // findTimeMax2                = std::max (findTimeMax2, measured);
    }
    // results.add_row ({"Find", "AgHashTable", format_integer (cntr), format_integer (findTimeTotal2 / 1'000)});
    measured    = timer.elapsed_ms ();
    results.add_row ({"Find", "AgHashTable", format_integer (cntr), format_integer (measured)});

#if defined (AG_DBG_MODE)
    memUsed     = table2.get_alloc_amount ();
#endif

    bucketInfo.add_headers ({"Bucket", "Key Count", "Unique Hash Count"});

    for (uint64_t i = 0; i < table2.get_bucket_count (); i++) {
        bucketInfo.add_row ({format_integer (i), format_integer (table2.get_bucket_key_count (i)), format_integer (table2.get_bucket_hash_count (i))});
    }


    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        flag                        = (int32_t)table1.erase (buff[i]);
        cntr                        += flag;

        // measured                    = timer.elapsed_us ();
        // delTimeTotal1               += measured;
        // delTimeMin1                 = std::min (delTimeMin1, measured);
        // delTimeMax1                 = std::max (delTimeMax1, measured);
    }
    // results.add_row ({"Erase", "std::unorderd_set", format_integer (cntr), format_integer (delTimeTotal1 / 1'000)});
    measured    = timer.elapsed_ms ();
    results.add_row ({"Erase", "std::unorderd_set", format_integer (cntr), format_integer (measured)});


    cntr = 0;
    timer.reset ();
    for (auto i = 0; i < pN; ++i) {
        // timer.reset ();

        flag                        = table2.erase (buff[i]);
        cntr                        += flag;

        // measured                    = timer.elapsed_us ();
        // delTimeTotal2               += measured;
        // delTimeMin2                 = std::min (delTimeMin2, measured);
        // delTimeMax2                 = std::max (delTimeMax2, measured);
    }
    // results.add_row ({"Erase", "AgHashTable", format_integer (cntr), format_integer (delTimeTotal2 / 1'000)});;
    measured    = timer.elapsed_ms ();
    results.add_row ({"Erase", "AgHashTable", format_integer (cntr), format_integer (measured)});;


    std::cout << results << '\n' << bucketInfo << '\n';

#if defined (AG_DBG_MODE)

    agMetrics.add_headers ({"Metric", "Count", "Unit"});
    agMetrics.add_row ({"Allocations", format_integer (table2.get_alloc_count ()), "-"});
    agMetrics.add_row ({"Frees", format_integer (table2.get_delete_count ()), "-"});
    agMetrics.add_row ({"Memory Allocated", format_integer (memUsed), "bytes"});
    agMetrics.add_row ({"Buckets", format_integer (table2.get_bucket_count ()), "-"});
    agMetrics.add_row ({"Resizes", format_integer (table2.get_resize_count ()), "-"});

    std::cout << agMetrics << '\n';

#endif
}

int
main (int argc, char *argv[])
{
    if (argc < 3) {
        std::cout << "Usage: ";
        std::cout << argv[0] << " <input_file> <oper1 [oper2...]>\n";

        std::cout << '\n';
        std::cout << "input_file:\tPath to file containing records\n";
        std::cout << "oper:\t\tNumber of operations of each type to perform\n";

        std::cout << '\n';
        std::cout << "Example: ";
        std::cout << argv[0] << " ../random_all.in 50000 1000000\n";

        return 1;
    }

    std::vector<int32_t>    args;
    args.reserve (argc - 2);

    for (int32_t i = 2, quantity; i < argc; ++i) {
        quantity    = atol (argv[i]);

        if (quantity <= 0) {
            std::cout << "Ignoring invalid quantity \"" << argv[i] << "\"\n";
            continue;
        }

        args.push_back (quantity);
    }

    if (args.size () <= 0) {
        std::cout << "No valid quantities provided\n";
        std::cout << "Exiting\n";
        return 1;
    }

    read_buffers (argv[1]);

    for (auto &quantity : args) {
        run_benchmark (quantity);
    }

    std::cout << "Exiting\n";
    return 0;
}
