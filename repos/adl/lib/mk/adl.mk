

INC_DIR += $(REP_DIR)/src/include
vpath %.c $(REP_DIR)/src
vpath %.cc $(REP_DIR)/src
vpath %.cpp $(REP_DIR)/src

SRC_CC += config.cpp string.cpp TString.cpp collections/LinkedList.cpp
