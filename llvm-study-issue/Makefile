LLVM_CONFIG?=llvm-config
CXX := clang++
LLVM_SRC?=.

srcdir = $(CURDIR)
ifeq ($(objdir),)
    ifneq ($(O),)
	objdir = $(O)
    else
	objdir = $(CURDIR)
    endif
endif

ifndef VERBOSE
QUIET:=@
endif

SRC_DIR ?= $(PWD)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags)
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags) -I$(LLVM_SRC)
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags) 
LLVM_BLDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs)  -lz -ltinfo


# study
ISSUE		:= issue-
33874		:= $(ISSUE)33874
33874_SRC	:= $(srcdir)/$(33874).cpp
33874_OBJ	:= $(srcdir)/$(33874).op


default: $(33874)
$(33874_OBJ): $(33874_SRC)
	$(CXX) -o $@ $< -c -g -Wall $(LLVM_CXXFLAGS)

$(33874): $(33874_OBJ)
	$(CXX) -o $@ $< -g -O0 -lgtest -lpthread $(LLVM_CXXFLAGS) $(LLVM_BLDFLAGS)

clean:
	$(QUIET)rm -f *.ll *.bin *.bc *.so *.op
