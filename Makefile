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

PACKAGE			:= Obfuscator
STRING-PASS		:= $(PACKAGE)String
STRING-PASS-SO		:= $(STRING-PASS).so
STRING-PASS-OBJS	:= $(STRING-PASS).op
STRING-PASS-TEST	:= String.c
STRING-PASS-BC		:= String.bc

CALL-PASS		:= $(PACKAGE)Call
CALL-PASS-SO		:= $(CALL-PASS).so
CALL-PASS-OBJS		:= $(CALL-PASS).op
CALL-PASS-TEST		:= $(CALL-PASS)Test.c
CALL-PASS-BC		:= $(CALL-PASS)Test.bc

PRINTER-PASS		:= Printer
PRINTER-PASS-SO		:= $(PRINTER-PASS).so
PRINTER-PASS-OBJS	:= $(PRINTER-PASS).op

# examples
EX		:= Ex-
ifstate		:= $(EX)ifstate
switch		:= $(EX)switch

EXAMPLE_SRCS 	:= $(ifstate).c $(switch).c

CXX := clang++

default: $(CALL-PASS-SO) $(STRING-PASS-SO) $(PRINTER-PASS-SO)

%.op : $(SRC_DIR)/%.cpp
	@echo 2. Compile $@ by Compiling $*.cpp
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

%.bc : $(SRC_DIR)/%.c
	@echo 2. Compile $@ by Compiling $*.c
	clang -emit-llvm -o $@ -c $<

$(STRING-PASS-SO) : $(STRING-PASS-OBJS)
	@echo 3. Linking $@ with LLVM libraries
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^

$(CALL-PASS-SO) : $(CALL-PASS-OBJS)
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^

$(PRINTER-PASS-SO) : $(PRINTER-PASS-OBJS)
	$(CXX) -o $@ $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^


run: $(STRING-PASS-BC) $(STRING-PASS-SO) $(CALL-PASS-BC) $(CALL-PASS-SO) $(ifstate).bc $(switch).bc
	opt -load-pass-plugin=$(SRC_DIR)/$(STRING-PASS-SO) -passes="$(STRING-PASS)" < $(STRING-PASS-BC) -o $(STRING-PASS-BC).bin 
	llvm-dis $(STRING-PASS-BC).bin
	opt -load-pass-plugin $(SRC_DIR)/$(CALL-PASS-SO) -passes="$(CALL-PASS)" $(CALL-PASS-BC) -o $(CALL-PASS-BC).bin 
	llvm-dis $(CALL-PASS-BC).bin
	opt -load-pass-plugin $(SRC_DIR)/$(PRINTER-PASS-SO) -passes="$(PRINTER-PASS)" $(ifstate).bc -o $(ifstate).bin 
	llvm-dis $(ifstate).bin
	opt -load-pass-plugin $(SRC_DIR)/$(PRINTER-PASS-SO) -passes="$(PRINTER-PASS)" $(switch).bc -o $(switch).bin 
	llvm-dis $(switch).bin

clean::
	$(QUIET)rm -f $(STRING-PASS-OBJS) $(STRING-PASS-SO)
	$(QUIET)rm -f $(CALL-PASS-OBJS) $(CALL-PASS-SO) $(CALL-PASS-BC)
	$(QUIET)rm -f *.ll *.bc *.bin
