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


loop-opt-tutorial: $(TUTORIAL).so

$(TUTORIAL_OBJ): $(TUTORIAL_SRC)
	$(CXX) -o $@ $< -fPIC -c -Wall $(LLVM_CXXFLAGS)

$(TUTORIAL).so: $(TUTORIAL_OBJ)
	$(CXX) -o $@ $< -shared -Wl,-O1 $(LLVM_CXXFLAGS) $(LLVM_LDFLAGS)

all: loop-opt-tutorial


simple: loop-opt-tutorial
	opt -load-pass-plugin=./$(TUTORIAL).so -passes="require<opt-remark-emit>,loop(loop-rotate,$<)" $(srcdir)/$(TUTORIAL)/$@.ll -S > $@-passed.ll
	@echo generated $@.ll file which passed $(TUTORIAL) pass.
	@echo see $@-passed.ll

loop_nest: loop-opt-tutorial
	opt -load-pass-plugin=./$(TUTORIAL).so -passes="require<opt-remark-emit>,loop(loop-rotate,$<)" $(srcdir)/$(TUTORIAL)/$@.ll -S > $@-passed.ll
	@echo generated $@.ll file which passed $(TUTORIAL) pass.
	@echo see $@-passed.ll

diagnostics: loop-opt-tutorial
	opt -load-pass-plugin=./$(TUTORIAL).so -passes="require<opt-remark-emit>,loop(loop-rotate,$<)" -pass-remarks-analysis=loop-opt-tutorial $(srcdir)/$(TUTORIAL)/$@.ll -S > $@-passed.ll
	@echo generated $@.ll file which passed $(TUTORIAL) pass.
	@echo see $@-passed.ll

run-all: simple loop_nest diagnostics

gen-png: simple loop_nest diagnostics
	for target in $^ ; do \
		opt -passes="loop(loop-rotate),dot-cfg" $${target}-passed.ll ; \
		dot -Tpng .*.dot > $${target}-passesd.png ; \
	done

clean::
	$(QUIET)rm -f *.ll *.so *.png .*.dot
	$(QUIET)rm -f $(TUTORIAL).bc $(TUTORIAL).so $(TUTORIAL_OBJ)
