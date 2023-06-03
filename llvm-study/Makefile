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
LLVM_BLDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs)

LOOP_SRCS := $(wildcard $(srcdir)/Loop/*.cpp)
LOOP_OBJS := $(patsubst $(srcdir)/%.cpp,$(objdir)/%.o,$(LOOP_SRCS))
LOOP := $(objdir)/libLoopStudy.so

# opt passes
LOOP		:= loop-
FLATTEN 	:= $(LOOP)flatten
FLATTEN_SRC	:= $(srcdir)/Loop/LoopFlatten.cpp
FLATTEN_OBJ	:= $(srcdir)/Loop/LoopFlatten.op
SIMPLIFY	:= $(LOOP)simplify
SIMPLIFY_SRC	:= $(srcdir)/Loop/LoopInstSimplify.cpp
SIMPLIFY_OBJ	:= $(srcdir)/Loop/LoopInstSimplify.op

# Utils
LOCAL		:= local-

# study
LOOP_STUDY	:= $(LOOP)study
LOOP_STUDY_SRC	:= $(srcdir)/$(LOOP_STUDY).cpp
LOOP_STUDY_OBJ	:= $(srcdir)/$(LOOP_STUDY).op
LOCAL_STUDY	:= $(LOCAL)study
LOCAL_STUDY_SRC := $(srcdir)/$(LOCAL_STUDY).cpp
LOCAL_STUDY_OBJ := $(objdir)/$(LOCAL_STUDY).op
SIMPLIFY_STUDY	:= simplify-study
SIMPLIFY_SRC := $(srcdir)/$(SIMPLIFY_STUDY).cpp
SIMPLIFY_OBJ := $(objdir)/$(SIMPLIFY_STUDY).op
INST_STUDY	:= simplify-study
INST_SRC := $(srcdir)/$(INST_STUDY).cpp
INST_OBJ := $(objdir)/$(INST_STUDY).op


default: $(FLATTEN).so $(SIMPLIFY).so $(LOOP_STUDY) $(LOCAL_STUDY) $(SIMPLIFY_STUDY)
$(SIMPLIFY_STUDY): $(SIMPLIFY_SRC)
	$(CXX) -o $@ $< -g -pg -O0 -lgtest -lcurses -lz -lpthread $(LLVM_CXXFLAGS) $(LLVM_BLDFLAGS)

$(LOCAL_STUDY_OBJ): $(LOCAL_STUDY_SRC)
	$(CXX) -o $@ $< -c -g -Wall $(LLVM_CXXFLAGS)

$(LOCAL_STUDY): $(LOCAL_STUDY_OBJ)
	$(CXX) -o $@ $< -g -O0 -lgtest -lpthread -lcurses -lz $(LLVM_CXXFLAGS) $(LLVM_BLDFLAGS)

$(LOOP_STUDY_OBJ): $(LOOP_STUDY_SRC)
	$(CXX) -o $@ $< -c -g -Wall $(LLVM_CXXFLAGS)

$(LOOP_STUDY): $(LOOP_STUDY_OBJ)
	$(CXX) -o $@ $< -g -O0 -lgtest -lpthread -lcurses -lz $(LLVM_CXXFLAGS) $(LLVM_BLDFLAGS)

$(FLATTEN_OBJ): $(FLATTEN_SRC)
	$(CXX) -o $@ $< -fPIC -c -Wall $(LLVM_CXXFLAGS)

$(FLATTEN).so: $(FLATTEN_OBJ)
	$(CXX) -o $@ $< -shared -Wl,-O1 $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS)

$(SIMPLIFY_OBJ): $(SIMPLIFY_SRC)
	$(CXX) -o $@ $< -fPIC -c -Wall $(LLVM_CXXFLAGS)

$(SIMPLIFY).so: $(SIMPLIFY_OBJ)
	$(CXX) -o $@ $< -shared -Wl,-O1 $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS)

$(SIMPLIFY).bc: $(srcdir)/Loop/loop-simplify1.ll
	llvm-as $< -o $@

$(FLATTEN).bc: $(srcdir)/Loop/loop-flatten-test1.ll
	llvm-as $< -o $@

run: $(FLATTEN).so $(FLATTEN).bc $(SIMPLIFY).so $(SIMPLIFY).bc
	opt -enable-new-pm=0 -load $(objdir)/$(FLATTEN).so --$(FLATTEN)-study $(FLATTEN).bc -o $(FLATTEN)-passed.bin
	llvm-dis $(FLATTEN)-passed.bin
	@echo generated .ll file which passed flatten pass.
	@echo see $(FLATTEN)-passed.bin.ll

	opt -enable-new-pm=0 -load $(objdir)/$(SIMPLIFY).so --loop-instsimplify-study $(SIMPLIFY).bc -o $(SIMPLIFY)-passed.bin
	llvm-dis $(SIMPLIFY)-passed.bin
	@echo generated .ll file which passed SIMPLIFY pass.
	@echo see $(SIMPLIFY)-passed.bin.ll


clean::
	$(QUIET)rm -f *.ll *.bin *.bc *.so
	$(QUIET)rm -f $(LOCAL_STUDY) $(LOCAL_STUDY_OBJ)
	$(QUIET)rm -f $(LOOP_STUDY) $(LOOP_STUDY_OBJ)
	$(QUIET)rm -f $(FLATTEN_OBJ)
	$(QUIET)rm -f $(SIMPLIFY_OBJ)
