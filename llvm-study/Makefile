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
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags)

LOOP_SRCS := $(wildcard $(srcdir)/Loop/*.cpp)
LOOP_OBJS := $(patsubst $(srcdir)/%.cpp,$(objdir)/%.o,$(LOOP_SRCS))
LOOP := $(objdir)/libLoopStudy.so

# opt passes
LOOP		:= loop-
FLATTEN 	:= $(LOOP)flatten
FLATTEN_SRC	:= $(srcdir)/Loop/LoopFlatten.cpp
FLATTEN_OBJ	:= $(srcdir)/Loop/LoopFlatten.op



default: $(FLATTEN).so
$(FLATTEN_OBJ): $(FLATTEN_SRC)
	$(CXX) -o $@ $< -fPIC -c -Wall $(LLVM_CXXFLAGS)

$(FLATTEN).so: $(FLATTEN_OBJ)
	$(CXX) -o $@ $< -shared -Wl,-O1 $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS)

$(FLATTEN).bc: $(srcdir)/Loop/loop-flatten-test1.ll
	llvm-as $< -o $(FLATTEN).bc

run: $(FLATTEN).so $(FLATTEN).bc
	opt -enable-new-pm=0 -load $(objdir)/$(FLATTEN).so --$(FLATTEN)-study $(FLATTEN).bc -o $(FLATTEN)-passed.bin
	llvm-dis $(FLATTEN)-passed.bin
	@echo generated .ll file which passed flatten pass.
	@echo see $(FLATTEN)-passed.bin.ll

clean::
	$(QUIET)rm -f *.ll *.bin *.bc *.so
	$(QUIET)rm -f $(FLATTEN_OBJ)
