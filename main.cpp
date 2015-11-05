#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <map>
#include <ei.h>

using namespace std;


class ConvertorChild {

};

using ProcessTable = map<erlang_pid*, ConvertorChild>;

//save_kcachegrind_format
class Saver {
    FILE* real = nullptr; //output file
    bool merged; //options - save pid or not

    const unsigned spaces = 20; //enough to save 2^64 bits
    const char* header = "events: Time\n" \
            "creator: C++ enhanced version of virtan/eep https://github.com/xbrukner/eep_cpp\n"\
            "summary: ";
    const char* header_end = "\n\n";


    std::string kcgfile(const char* filename) {
        std::string ret("callgrind.out");
        ret.append(filename);
        return ret;
    }

    void save_header() {
        fprintf(real, "%s", header);
        for(unsigned i = 0; i < spaces; ++i) fputc(' ', real);
        fprintf(real, "%s", header_end);
    }

    //In-place header modification
    void modify_header(unsigned number) {
        string n = to_string(number);
        fseek(real, strlen(header) + spaces - n.size(), SEEK_SET);
        fprintf(real, "%s", n.c_str());
    }

    unsigned p = 1; //messages
    unsigned minTime = 0; //now
    unsigned maxTime = 0;
    unsigned startTime = 0; //now

    const unsigned buffer_size = 20*1024*1024;
    unsigned buffer_filled = 0;
    char *buffer;
    unsigned stuck = 0;

    ProcessTable &processes;
public:
    Saver(const char* filename, bool merged, ProcessTable& processes) :
        merged(merged), buffer(new char[buffer_size]), processes(processes) {
        //TODO - now into minTime and startTime
        real = fopen(kcgfile(filename).c_str(), "w+");
        if (!real) {
            cerr << "Could not open output file" << endl;
            abort();
        }
        save_header();
    }

    ~Saver() {
        fclose(real);
        delete[] buffer;
    }
};

class CallgrindConvertor {
    //Processes:
    ProcessTable processes;

    //Saver:
    Saver saver;

    //Options:
    bool waits;
public:
    CallgrindConvertor(const char* filename, bool waits, bool merged) : saver(filename, merged, processes), waits(waits) {

    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: binary waits merged_pids" << endl;
        return 0;
    }
    bool waits = strcmp(argv[1], "true") == 0;
    bool merged = strcmp(argv[1], "true") == 0;
}
