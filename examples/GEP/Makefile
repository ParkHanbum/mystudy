include ../../Makefile.include
SRC_DIR := $(PWD)

# example
EX-SRC 	:= $(wildcard $(SRC_DIR)/Ex*.c)
EX-BC	:= $(patsubst $(SRC_DIR)/Ex%.c,$(SRC_DIR)/Ex%.bc,$(EX-SRC))
EX-BIN	:= $(patsubst $(SRC_DIR)/Ex%.c,$(SRC_DIR)/Ex%.bin,$(EX-SRC))


# example module
MODULE-PASS		:= Hello
MODULE-PASS-SO		:= $(MODULE-PASS).so
MODULE-PASS-OBJ		:= $(MODULE-PASS).op


%.op : $(SRC_DIR)/%.cpp
	@echo GEN $@ by Compiling $*.cpp
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(SRC_DIR)/%.bin : $(SRC_DIR)/%.bc $(MODULE-PASS-SO)
	@echo RUN module for $<. result saved to $@ 
	$(OPT) -load ./$(MODULE-PASS-SO) -$(MODULE-PASS) < $< > $@

$(SRC_DIR)/%.bc : $(SRC_DIR)/%.c
	@echo GEN $@ by Compiling $*.c
	$(CC) -emit-llvm -o $@ -c $<

$(MODULE-PASS-SO) : $(MODULE-PASS-OBJ)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@


all: $(EX-BIN)

clean:
	$(QUIET)rm -f *.bc *.ll *.so *.op *.bin
