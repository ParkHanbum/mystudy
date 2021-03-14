include ../../Makefile.include
SRC_DIR := $(PWD)

# example
EX	:= Ex-BasicBlock
EX-SRC	:= $(EX).c
EX-BC	:= $(EX).bc

# example module
LISTING-PASS		:= Listing
LISTING-PASS-SO		:= $(LISTING-PASS).so
LISTING-PASS-OBJ	:= $(LISTING-PASS).op


%.op : $(SRC_DIR)/%.cpp
	@echo GEN $@ by Compiling $*.cpp
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<


%.bc : $(SRC_DIR)/%.c
	@echo GEN $@ by Compiling $*.c
	$(CC) -emit-llvm -o $@ -c $<

$(LISTING-PASS-SO) : $(LISTING-PASS-OBJ)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@


all: $(EX-BC) $(LISTING-PASS-SO)
	$(OPT) -load-pass-plugin ./$(LISTING-PASS-SO) -passes="$(LISTING-PASS)" $(EX-BC) -o $(EX-BC).bin
	$(DIS) $(EX-BC).bin

clean:
	$(QUIET)rm -f *.bc *.ll *.so *.op
