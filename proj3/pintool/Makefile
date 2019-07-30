##
## Makefile to build tools against Pin
##

# This should point to whereever you've unpacked a pin kit.  This location
# exists on CSAIL AFS and should work if you're in 6.172.
PIN_KIT ?= /afs/csail.mit.edu/proj/courses/6.172/pin-2.8-37300-gcc.3.4.6-ia32_intel64-linux

PIN_HOME ?= $(PIN_KIT)/source/tools

PINTOOL_DIR := $(PIN_HOME)/source/tools

TARGET_COMPILER?=gnu
ifdef OS
    ifeq (${OS},Windows_NT)
        TARGET_COMPILER=ms
    endif
endif

ifeq ($(TARGET_COMPILER),gnu)
    include $(PIN_HOME)/makefile.gnu.config
    CXXFLAGS ?= -Wall -Werror -Wno-unknown-pragmas $(DBG) $(OPT)
endif

ifeq ($(TARGET_COMPILER),ms)
    include $(PIN_HOME)/makefile.ms.config
    DBG?=
endif

## build rules

all: tools

TOOL_ROOTS = countcalls malloctrace

TOOLS = $(TOOL_ROOTS:%=%$(PINTOOL_SUFFIX))
TOOL_OBJS = $(TOOL_ROOTS:%=%.o)

tools: $(TOOLS)

$(TOOL_OBJS): %.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) ${OUTOPT}$@ $<
$(TOOLS): $(PIN_LIBNAMES)
$(TOOLS): %$(PINTOOL_SUFFIX) : %.o
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) ${LINK_OUT}$@ $< ${PIN_LPATHS} $(PIN_LIBS) $(DBG)

## cleaning
clean:
	-rm -rf *.o *$(PINTOOL_SUFFIX)

## tests

test: all
	./test_countcalls.py
