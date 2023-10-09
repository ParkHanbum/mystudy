LLVM_CONFIG?=llvm-config
CXX := clang++

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
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags) -Wno-unused-value -Wno-macro-redefined
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_BLDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs)

# Loop Optimization Tutorial
TUTORIAL		:= LoopOptTutorial
TUTORIAL_SRC	:= $(srcdir)/$(TUTORIAL)/$(TUTORIAL).cpp
TUTORIAL_OBJ	:= $(srcdir)/$(TUTORIAL)/$(TUTORIAL).op


loop-opt-tutorial: $(TUTORIAL).so $(TUTORIAL).bc
$(TUTORIAL).bc: $(srcdir)/LoopOptTutorial/simple.ll
	llvm-as $< -o $@

$(TUTORIAL_OBJ): $(TUTORIAL_SRC)
	$(CXX) -o $@ $< -fPIC -c -Wall $(LLVM_CXXFLAGS)

$(TUTORIAL).so: $(TUTORIAL_OBJ)
	$(CXX) -o $@ $< -shared -Wl,-O1 $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS)

all: loop-opt-tutorial

run-loop-opt-tutorial: loop-opt-tutorial
	opt -load-pass-plugin=./$(TUTORIAL).so -passes="$<" $(TUTORIAL).bc -o $(TUTORIAL)-passed.bin
	llvm-dis $(TUTORIAL)-passed.bin
	@echo generated .ll file which passed $(TUTORIAL) pass.
	@echo see $(TUTORIAL)-passed.bin.ll

run-all: run-loop-opt-tutorial


clean::
	$(QUIET)rm -f *.ll *.bin *.bc *.so
	$(QUIET)rm -f $(TUTORIAL).bc $(TUTORIAL).so $(TUTORIAL_OBJ)
