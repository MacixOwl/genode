# Application-layer network support for monkey.
#
# Created on 2025.1.5
# gongty [at] tongji [dot] edu [dot] cn
#

INC_DIR += $(REP_DIR)/include 
vpath %.c $(REP_DIR)/src/lib/net
vpath %.cc $(REP_DIR)/src/lib/net
vpath %.cpp $(REP_DIR)/src/lib/net

SRC_CC += protocol/protocol.cc 

LIBS = base adl monkey_crypto
