# Monkey Memory : Local memory tools.
#
# Created on 2025.1.14
# gongty [at] tongji [dot] edu [dot] cn
#



INC_DIR += $(REP_DIR)/include 
vpath %.c $(REP_DIR)/src/lib/memory
vpath %.cc $(REP_DIR)/src/lib/memory
vpath %.cpp $(REP_DIR)/src/lib/memory

SRC_CC += map.cc

LIBS = base adl 

