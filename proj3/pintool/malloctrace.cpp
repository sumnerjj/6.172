// Copyright (c) 2011 Massachusetts Institute of Technology
// Author: Reid Kleckner <reid@kleckner.net>
//
// Malloc trace pin tool
//
// This tool can generate .rep trace files for the 6.172 malloc lab (originally
// from Computer Science: A Programmer's Perspective:
// http://csapp.cs.cmu.edu/public/labs.html ).  You can trace interesting
// applications by adding the tool somewhere in the pin build system and
// running it like so:
//
// path/to/pin -t path/to/malloctrace.so -o <app>.rep -- <app-cmd>
//

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <tr1/unordered_map>

#include "pin.H"

static std::ofstream *trace_stream = NULL;
static std::ostream *log_stream = NULL;

static std::ostream &trace() {
    return *trace_stream;
}

static std::ostream &log() {
    return *log_stream;
}

static KNOB<string> KnobTracefile(
    KNOB_MODE_WRITEONCE, "pintool", "o", "trace.rep",
    "trace file output name");

static KNOB<string> KnobLogfile(
    KNOB_MODE_WRITEONCE, "pintool", "log", "",
    "log file name");

static KNOB<bool> KnobDumpBuffer(
    KNOB_MODE_WRITEONCE, "pintool", "dump_buffer", "false",
    "dump the buffer of events to the log, default false");

enum EventType {
    MALLOC,
    MALLOC_RET,
    REALLOC_PTR,
    REALLOC_SIZE,
    REALLOC_RET,
    FREE
};

static const char *event_names[] = {
    "MALLOC",
    "MALLOC_RET",
    "REALLOC_PTR",
    "REALLOC_SIZE",
    "REALLOC_RET",
    "FREE"
};

struct MallocEvent {
    MallocEvent(EventType type, uintptr_t val)
        : type(type), val(val) {}

    EventType type;
    uintptr_t val;
};

static std::vector<MallocEvent> events;

static void trace_malloc(size_t size) {
    events.push_back(MallocEvent(MALLOC, size));
}

static void trace_malloc_ret(void *ptr) {
    events.push_back(MallocEvent(MALLOC_RET, (uintptr_t)ptr));
}

static void trace_realloc(void *ptr, size_t size) {
    events.push_back(MallocEvent(REALLOC_PTR, (uintptr_t)ptr));
    events.push_back(MallocEvent(REALLOC_SIZE, size));
}

static void trace_realloc_ret(void *ptr) {
    events.push_back(MallocEvent(REALLOC_RET, (uintptr_t)ptr));
}

static void trace_free(void *ptr) {
    events.push_back(MallocEvent(FREE, (uintptr_t)ptr));
}

// Helper class for playing back malloc events to different listeners.  Malloc,
// Realloc, and Free are called once with the appropriate arguments and return
// values as occurred in the program.  The data argument is whatever was in the
// addr_map.  If nothing was in the addr_map, the event is skipped.
class MallocListener {
 public:
    MallocListener() : addr_map() {}

    void Playback(std::vector<MallocEvent> &events);

 protected:
    virtual void Malloc(size_t size, uintptr_t ret) = 0;
    virtual void Realloc(size_t size, uintptr_t ptr, uintptr_t ret,
                         int data) = 0;
    virtual void Free(uintptr_t ptr, int data) = 0;

    // Called after every call to one of the above malloc operations.
    virtual void Operation() {}

    // Called after all events have been played back to the listener.
    virtual void Done() {}

    // Most visitors want to keep some data attached to each address that is
    // live at this point in the playback.
    std::tr1::unordered_map<uintptr_t, int> addr_map;
};

class HeaderListener : public MallocListener {
 public:
    HeaderListener()
        : MallocListener(), cur_heap(0), max_heap(0), num_ids(0), num_ops(0) {}

 protected:
    virtual void Malloc(size_t size, uintptr_t ret);
    virtual void Realloc(size_t size, uintptr_t ptr, uintptr_t ret,
                         int data);
    virtual void Free(uintptr_t ptr, int data);
    virtual void Operation();
    virtual void Done();

 private:

    size_t cur_heap;
    size_t max_heap;
    int num_ids;
    int num_ops;
};

class TraceListener : public MallocListener {
 public:
    TraceListener() : MallocListener(), next_index(0) {}

 protected:
    virtual void Malloc(size_t size, uintptr_t ret);
    virtual void Realloc(size_t size, uintptr_t ptr, uintptr_t ret,
                         int data);
    virtual void Free(uintptr_t ptr, int size);

 private:
    int next_index;
};

void MallocListener::Playback(std::vector<MallocEvent> &events) {
    // TODO(rnk): Make this less yucky in general.  Writing pattern matchers in
    // C++ is hard.
    for (unsigned i = 0; i < events.size(); i++) {
        if (events[i    ].type == MALLOC &&
            events[i + 1].type == MALLOC_RET) {
            // Normal malloc/ret pair.
            uintptr_t size = events[i].val;
            uintptr_t ptr = events[i + 1].val;
            i += 1;
            this->Malloc(size, ptr);
            this->Operation();
        } else if (events[i    ].type == REALLOC_PTR &&
                   events[i + 1].type == REALLOC_SIZE) {
            uintptr_t old_ptr  = events[i    ].val;
            uintptr_t new_size = events[i + 1].val;
            uintptr_t new_ptr;
            if (events[i + 2].type == REALLOC_RET) {
                // Normal return straight from realloc.
                new_ptr = events[i + 2].val;
                i += 2;
            } else if (events[i + 2].type == MALLOC &&
                       events[i + 3].type == MALLOC_RET) {
                // Tail call from realloc to malloc.
                if (new_size != events[i + 2].val) {
                    log() << "Size from realloc tail call doesn't match!\n";
                    i += 1;
                    continue;
                }
                new_ptr = events[i + 3].val;
                i += 3;
            } else if (events[i + 2].type == REALLOC_PTR &&
                       events[i + 3].type == REALLOC_SIZE &&
                       events[i + 2].val == old_ptr &&
                       events[i + 3].val == new_size) {
                // On the first call to realloc, we get called twice.  Ignore
                // the first call.
                i += 1;
                continue;
            } else {
                log() << "Unable to find realloc return value!\n";
                i += 1;
                continue;
            }
            if (old_ptr == 0) {
                // To support realloc'ing NULL, we inject a malloc of size 1
                // because the trace file format doesn't support realloc'ing
                // pointers that aren't previously allocated blocks.
                this->Malloc(1, 0);
                this->Operation();
            }
            std::tr1::unordered_map<uintptr_t, int>::iterator it =
                addr_map.find(old_ptr);
            if (it == addr_map.end()) {
                log() << "Called realloc with unallocated ptr!\n";
                continue;
            }
            size_t data = it->second;
            addr_map.erase(old_ptr);
            this->Realloc(new_size, old_ptr, new_ptr, data);
            this->Operation();
        } else if (events[i].type == FREE) {
            // Normal free.
            uintptr_t ptr = events[i].val;
            if (ptr == 0) {
                // To simulate free'ing NULL, we inject a malloc of size 0.
                this->Malloc(1, 0);
                this->Operation();
            }
            std::tr1::unordered_map<uintptr_t, int>::iterator it =
                    addr_map.find(ptr);
            if (it == addr_map.end()) {
                log() << "Freed previously unallocated pointer: "
                      << ptr << '\n';
                continue;
            }
            size_t data = it->second;
            addr_map.erase(ptr);
            this->Free(ptr, data);
            this->Operation();
        }
    }

    this->Done();
}

void HeaderListener::Malloc(size_t size, uintptr_t ret) {
    cur_heap += size;
    addr_map[ret] = size;
    num_ids++;
}

void HeaderListener::Realloc(size_t new_size, uintptr_t old_ptr,
                             uintptr_t new_ptr, int old_size) {
    cur_heap += new_size - old_size;
    addr_map[new_ptr] = new_size;
}

void HeaderListener::Free(uintptr_t ptr, int old_size) {
    cur_heap -= old_size;
}

void HeaderListener::Operation() {
    num_ops++;
    max_heap = std::max(max_heap, cur_heap);
}

void HeaderListener::Done() {
    // The first four lines of the tracefiles are:
    // - sugg_heapsize # unused
    // - num_ids
    // - num_ops
    // - weight # unused
    trace() << std::setbase(10)
            << max_heap * 2 << '\n'
            << num_ids << '\n'
            << num_ops << '\n'
            << 1 << '\n';
}

void TraceListener::Malloc(size_t size, uintptr_t ret) {
    int index = next_index++;
    addr_map[ret] = index;
    trace() << "a " << index << ' ' << size << '\n';
}

void TraceListener::Realloc(size_t new_size, uintptr_t old_ptr,
                            uintptr_t new_ptr, int index) {
    addr_map[new_ptr] = index;
    trace() << "r " << index << ' ' << new_size << '\n';
}

void TraceListener::Free(uintptr_t ptr, int index) {
    trace() << "f " << index << '\n';
}

static void WriteTraceFile() {
    HeaderListener hdr;
    hdr.Playback(events);
    TraceListener trace;
    trace.Playback(events);
    events.clear();
}

static void LogInstrument(IMG img, RTN rtn) {
    log() << std::showbase << std::setbase(16)
          << "Instrumenting RTN.  image: " << IMG_Name(img)
          << " name: " << RTN_Name(rtn)
          << " addr: " << RTN_Address(rtn) << '\n';
}

static void Image(IMG img, VOID *v) {
    RTN rtn;
    rtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(rtn)) {
        LogInstrument(img, rtn);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)trace_malloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)trace_malloc_ret,
                       IARG_FUNCRET_EXITPOINT_VALUE,
                       IARG_END);
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "realloc");
    if (RTN_Valid(rtn)) {
        LogInstrument(img, rtn);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)trace_realloc,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)trace_realloc_ret,
                       IARG_FUNCRET_EXITPOINT_VALUE,
                       IARG_END);
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "free");
    if (RTN_Valid(rtn)) {
        LogInstrument(img, rtn);
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)trace_free,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_END);
        RTN_Close(rtn);
    }
}

static void DumpBuffer() {
    for (unsigned i = 0; i < events.size(); i++) {
        MallocEvent &e = events[i];
        int base = 16;
        if (e.type == MALLOC || e.type == REALLOC_SIZE) {
            base = 10;
        }
        log() << std::showbase
              << std::left
              << std::setbase(base)
              << std::setw(12)
              << event_names[e.type] << ' '
              << e.val << '\n';
    }
}

static VOID Fini(INT32 code, VOID *v) {
    // Dump the buffer to the log, if requested.
    if (KnobDumpBuffer.Value()) {
        DumpBuffer();
    }
    WriteTraceFile();
    trace_stream->close();
}

static int Usage() {
    std::cerr << "Pintool for tracing calls to malloc library routines.\n";
    std::cerr << "Usage: pin -t memorytrace.so -o <app.rep> -- app\n";
    std::cerr << '\n' << KNOB_BASE::StringKnobSummary() << '\n';
    return -1;
}

int main(int argc, char * argv[]) {
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

    trace_stream = new std::ofstream(KnobTracefile.Value().c_str());
    if (!KnobLogfile.Value().empty()) {
        log_stream = new std::ofstream(KnobLogfile.Value().c_str());
    } else {
        log_stream = &std::cerr;
    }

    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();  // noreturn
    return 0;
}
