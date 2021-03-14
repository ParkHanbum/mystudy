include Makefile.include

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

FLATTER-PASS		:= Flatter
FLATTER-PASS-SO		:= $(FLATTER-PASS).so
FLATTER-PASS-OBJS	:= $(FLATTER-PASS).op

SKELETON-PASS		:= Skeleton
SKELETON-PASS-SO	:= $(SKELETON-PASS).so
SKELETON-PASS-OBJS	:= $(SKELETON-PASS).op

CFI-PASS		:= CFI
CFI-PASS-SO		:= $(CFI-PASS).so
CFI-PASS-OBJS		:= $(CFI-PASS).op

# examples
EX		:= Ex-
ifstate		:= $(EX)ifstate
switch		:= $(EX)switch
while		:= $(EX)while
call		:= $(EX)call
cfi		:= $(EX)CFI

EXAMPLE_SRCS 	:= $(ifstate).c $(switch).c $(while).c $(call).c

CALL-EX-BC += $(call).bc
FLATTER-PASS-BC := $(ifstate).bc
CFI-EX-BC := $(cfi).bc

CC := clang-10
CXX := clang++-10
OPT := opt-10
DIS := llvm-dis-10


default: $(CALL-PASS-SO) $(STRING-PASS-SO) $(SKELETON-PASS-SO) $(FLATTER-PASS-SO)

%.op : $(SRC_DIR)/%.cpp
	@echo 2. GEN $@ by Compiling $*.cpp
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

%.bc : $(SRC_DIR)/%.c
	@echo 2. GEN $@ by Compiling $*.c
	$(CC) -emit-llvm -o $@ -c $<

$(STRING-PASS-SO) : $(STRING-PASS-OBJS)
	@echo 3. Linking $@ with LLVM libraries
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ 
$(CALL-PASS-SO) : $(CALL-PASS-OBJS)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ 

$(FLATTER-PASS-SO) : $(FLATTER-PASS-OBJS)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ 

$(SKELETON-PASS-SO) : $(SKELETON-PASS-OBJS)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ 

$(CFI-PASS-SO) : $(CFI-PASS-OBJS)
	$(CXX) $(LOADABLE_MODULE_OPTIONS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ 

run: $(STRING-PASS-BC) $(STRING-PASS-SO) $(CALL-PASS-BC) $(CALL-PASS-SO) $(FLATTER-PASS-SO) $(SKELETON-PASS-SO) $(CFI-EX-BC) $(ifstate).bc $(switch).bc $(while).bc
	$(OPT) -load-pass-plugin=$(SRC_DIR)/$(STRING-PASS-SO) -passes="$(STRING-PASS)" < $(STRING-PASS-BC) -o $(STRING-PASS-BC).bin 
	$(DIS) $(STRING-PASS-BC).bin
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(ifstate).bc -o $(ifstate).bin 
	$(DIS) $(ifstate).bin
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(switch).bc -o $(switch).bin 
	$(DIS) $(switch).bin
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(while).bc -o $(while).bin 
	$(DIS) $(while).bin

debug: $(SKELETON-PASS-SO) $(CFI-EX-BC) $(ifstate).bc $(switch).bc $(while).bc
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(ifstate).bc -o $(ifstate).bin 
	$(DIS) $(ifstate).bin
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(switch).bc -o $(switch).bin 
	$(DIS) $(switch).bin
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(SKELETON-PASS-SO) -passes="$(SKELETON-PASS)" $(while).bc -o $(while).bin 
	$(DIS) $(while).bin


obfstring: $(STRING-PASS-BC) $(STRING-PASS-SO)
	$(OPT) -load-pass-plugin=$(SRC_DIR)/$(STRING-PASS-SO) -passes="$(STRING-PASS)" $(STRING-PASS-BC) -o $(STRING-PASS-BC).bin 
	$(DIS) $(STRING-PASS-BC).bin

obfcall: $(CALL-EX-BC) $(CALL-PASS-BC) $(CALL-PASS-SO)
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(CALL-PASS-SO) -passes="$(CALL-PASS)" $(CALL-PASS-BC) -o $(CALL-PASS-BC).bin 
	$(DIS) $(CALL-PASS-BC).bin
	$(DIS) $(CALL-EX-BC)

flatting: $(FLATTER-PASS-SO) $(ifstate).bc
	$(OPT) -load-pass-plugin=$(SRC_DIR)/$^ -passes="$(FLATTER-PASS)" < $(FLATTER-PASS-BC) -o $(FLATTER-PASS-BC).bin 
	$(DIS) $(FLATTER-PASS-BC).bin

cfi: $(CFI-EX-BC) $(CFI-PASS-BC) $(CFI-PASS-SO)
	$(CC) -o nocfi Ex-CFI.c
	#clang -o cfi Ex-CFI.c -flto -fsanitize=cfi -fvisibility=default
	$(OPT) -load-pass-plugin $(SRC_DIR)/$(CFI-PASS-SO) -passes="$(CFI-PASS)" $(CFI-EX-BC) -o $(CFI-EX-BC).bin 
	$(DIS) $(CFI-EX-BC).bin

clean::
	$(QUIET)rm -f $(STRING-PASS-OBJS) $(STRING-PASS-SO)
	$(QUIET)rm -f $(CALL-PASS-OBJS) $(CALL-PASS-SO)
	$(QUIET)rm -f $(SKELETON-PASS-OBJS) $(SKELETON-PASS-SO)
	$(QUIET)rm -f *.ll *.bc *.bin *.op
