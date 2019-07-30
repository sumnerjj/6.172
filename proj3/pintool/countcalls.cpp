// Copyright (c) 2011 Massachusetts Institute of Technology
// Author: Reid Kleckner <reid@kleckner.net>
//
// Tool for counting calls to various functions.  You can use this to do things
// like count calls to your favorite routines like malloc or whatever else you
// care about.

#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "pin.H"

static std::ofstream *out_stream = NULL;

static std::ostream &out() {
    return *out_stream;
}

static int num_rtns;

static std::vector<std::string> rtn_names;

static int *call_counts;

static void do_count(int *count_addr)
{
    (*count_addr)++;
}

static void Image(IMG img, VOID *v)
{
    for (int i = 0; i < num_rtns; i++) {
        RTN rtn = RTN_FindByName(img, rtn_names[i].c_str());
        if (!RTN_Valid(rtn)) continue;

        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)do_count,
                       IARG_PTR, &call_counts[i],
                       IARG_END);
        RTN_Close(rtn);
    }
}

static void split(const std::string &s, char delim,
                  std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

static VOID Fini(INT32 code, VOID *v)
{
    for (int i = 0; i < num_rtns; i++) {
        out() << rtn_names[i] << ": "  << call_counts[i] << '\n';
    }
    delete[] call_counts;
    out_stream->close();
}

KNOB<string> KnobOutputFile(
    KNOB_MODE_WRITEONCE, "pintool", "o", "countcalls.out",
    "specify output file name");

KNOB<string> KnobCountRoutines(
    KNOB_MODE_WRITEONCE, "pintool", "r", "malloc",
    "comma-separated list of routines to count");

static int Usage() {
    std::cerr << "Pintool for counting calls to listed routines.\n";
    std::cerr << "Usage: pin -t countcalls.so -r rtn1,rtn2... -- app\n";
    std::cerr << '\n' << KNOB_BASE::StringKnobSummary() << '\n';
    return -1;
}

int main(int argc, char * argv[])
{
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

    // In case stdout or stderr are closed.
    out_stream = new std::ofstream(KnobOutputFile.Value().c_str());

    std::string routines(KnobCountRoutines.Value());
    split(routines, ',', rtn_names);
    num_rtns = rtn_names.size();
    call_counts = new int[num_rtns];

    IMG_AddInstrumentFunction(Image, 0);

    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();  // noreturn
    return 0;
}
