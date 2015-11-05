#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <map>
#include <ei.h>
#include <chrono>
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace std;


class ConvertorChild {
    bool finalized = false;
public:
    bool finalized_check() {
        if (finalized) return false;
        return finalized = true;
    }

    void finalize() {

    }
};

using ProcessTable = map<erlang_pid*, ConvertorChild>;

unsigned now_microseconds() {
    auto now = chrono::high_resolution_clock::now();
    chrono::time_point<chrono::high_resolution_clock, chrono::microseconds> tp(chrono::time_point_cast<chrono::microseconds>(now));
    return tp.time_since_epoch().count();
}

unsigned td(unsigned from, unsigned to) {
    return to - from;
}

void finalize_processes(ProcessTable& table) {
    for(auto &i : table) {
        i.second.finalize();
    }
}

//save_kcachegrind_format
class Saver {
    FILE* real = nullptr; //output file

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
    unsigned finalizedCount = 0; //When finalized is called, this counter is increased

    void flush_buffer() {
        fwrite(buffer, buffer_filled, 1, real);
        buffer[0] = '\0';
        buffer_filled = 0;
    }

public:
    bool done = false;

    Saver(const char* filename, ProcessTable& processes) :
        buffer(new char[buffer_size]), processes(processes) {
        minTime = startTime = now_microseconds();
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

    void receive_bytes(const string &bytes, unsigned ts) {
        if (bytes.size() + buffer_filled >= buffer_size) {
            flush_buffer();
        }
        assert(bytes.size() + buffer_filled < buffer_size); //Packet is not too big
        strcpy(buffer + buffer_filled, bytes.c_str());
        buffer_filled += bytes.size();

        p++;
        minTime = min(minTime, ts);
        maxTime = max(maxTime, ts);
        stuck = 0;
    }

    void finalize(erlang_pid* pid, unsigned minTime1, unsigned maxTime1) {
        minTime = min(minTime, minTime1);
        maxTime = max(maxTime, maxTime1);
        if (processes[pid].finalized_check()) finalizedCount++;
        if (finalizedCount == processes.size()) {
            working_status(maxTime);
            flush_buffer();
            modify_header(td(minTime, maxTime));
            done = true;
        }
    }

    void status() {
        if (stuck >= 2) {
            working_status(max(minTime, maxTime));
            std::cerr << "No end_of_trace and no data, finishing forcibly ("
                      << processes.size() << ")" << endl;
            finalize_processes(processes);
        }
        else {
            working_status(max(minTime, maxTime));
        }
        stuck++;
    }

    void working_status(unsigned max_time) {
        unsigned now = now_microseconds();
        cerr << p << " msgs ("
             << round(p / (td(startTime, now) / 1000000)) << " msgs/sec), "
             << td(minTime, max_time) / 1000000 << " secs ("
             << round(td(startTime, now) / max(td(minTime, max_time), 1u)) << "x slowdown)"
             << endl;

    }
};

class CallgrindConvertor {
    //Processes:
    ProcessTable processes;

    //Saver:
    Saver saver;

    //Options:
    bool waits;
    bool merged;
public:
    CallgrindConvertor(const char* filename, bool waits, bool merged) :
        saver(filename, processes), waits(waits), merged(merged) {

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
