TARGETS = qsort
TARGETS32 := $(TARGETS:%=%.32)
TARGETS64 := $(TARGETS:%=%.64)
ALLTARGETS := $(TARGETS64)

# results dumped by cilkview objects
VIEWS = qsort

CC      = cilk++
CFLAGS  = -g -Werror
LDFLAGS = -lcilkutil

OLDMODE := $(shell cat .buildmode 2> /dev/null)
ifeq ($(DEBUG),1)
  CFLAGS := -DDEBUG -O0 $(CFLAGS)
  ifneq ($(OLDMODE),debug)
  $(shell echo debug > .buildmode)
endif
        else
  CFLAGS := -O3 $(CFLAGS)
  ifneq ($(OLDMODE),nodebug)
  $(shell echo nodebug > .buildmode)
endif
        endif

CFLAGS32 := $(CFLAGS) -m32
CFLAGS64 := $(CFLAGS) -m64

OUTPUTS = $(VIEWS:%=%.csv) $(VIEWS:%=%.plt) $(ALLTARGETS:%=%.cv.out) cilkview.out

all: $(ALLTARGETS)

.buildmode:
	touch .buildmode

%.32 : %.cilk .buildmode
	$(CC) -o $@ $(CFLAGS32) $< $(LDFLAGS)

%.64 : %.cilk .buildmode
	$(CC) -o $@ $(CFLAGS64) $< $(LDFLAGS)

clean:
	rm -f $(ALLTARGETS) $(OUTPUTS) *.stdout *.stderr
