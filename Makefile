LLVM_CONFIG?=llvm-config

ifndef VERBOSE
QUIET:=@
endif

SRC_DIR ?= $(PWD)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags)
COMMON_FLAGS :=
CXXFLAGS += -g $(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags) -fno-rtti
CPPFLAGS += -g $(shell $(LLVM_CONFIG) --cppflags) -I$(SRC_DIR) -fPIC

ifeq ($(shell uname),Darwin)
LOADABLE_MODULE_OPTIONS=-bundle -undefined dynamic_lookup
else
LOADABLE_MODULE_OPTIONS=-shared -Wl,-O1
endif

PACKAGE		:= Obfuscator
STRING-PASS	:= $(PACKAGE)String
STRING-PASS-SO	:= $(STRING-PASS).so
STRING-PASS-OBJS:= $(STRING-PASS).op
BITCODE_SRC 	:= String.c
BITCODE 	:= String.bc

CALL-PASS	:= $(PACKAGE)Call
CALL-PASS-SO	:= $(CALL-PASS).so
CALL-PASS-OBJS	:= $(CALL-PASS).op
CALL-PASS-TEST 	:= $(CALL-PASS)Test.c
CALL-PASS-BC 	:= $(CALL-PASS)Test.bc

CXX := clang++

default: $(CALL-PASS-SO) $(STRING-PASS-SO)

%.op : $(SRC_DIR)/%.cpp
	@echo 2. Compile $@ by Compiling $*.cpp
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

%.bc : $(SRC_DIR)/%.c
	@echo 2. Compile $@ by Compiling $*.cpp
	clang -emit-llvm -o $@ -c $<

$(STRING-PASS-SO) : $(STRING-PASS-OBJS)
	@echo 3. Linking $@ with LLVM libraries
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^

$(CALL-PASS-SO) : $(CALL-PASS-OBJS)
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^

run: $(BITCODE) $(STRING-PASS-SO) $(CALL-PASS-BC) $(CALL-PASS-SO)
	opt -load-pass-plugin=$(SRC_DIR)/$(STRING-PASS-SO) -passes="$(STRING-PASS)" < $(BITCODE) -o $(BITCODE).bin 
	llvm-dis $(BITCODE).bin
	opt -load-pass-plugin $(SRC_DIR)/$(CALL-PASS-SO) -passes="$(CALL-PASS)" $(CALL-PASS-BC) -o $(CALL-PASS-BC).bin 
	llvm-dis $(CALL-PASS-BC).bin

clean::
	$(QUIET)rm -f $(STRING-PASS-OBJS) $(STRING-PASS-SO)
	$(QUIET)rm -f $(CALL-PASS-OBJS) $(CALL-PASS-SO) $(CALL-PASS-BC)
	$(QUIET)rm -f *.ll *.bc *.bin
