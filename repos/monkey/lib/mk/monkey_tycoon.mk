# Monkey Tycoon : Distribute Memory Support for Apps.
#
# Created on 2025.1.15
# gongty [at] tongji [dot] edu [dot] cn
#

INC_DIR += $(REP_DIR)/include 
vpath %.c $(REP_DIR)/src/lib/tycoon
vpath %.cc $(REP_DIR)/src/lib/tycoon
vpath %.cpp $(REP_DIR)/src/lib/tycoon

SRC_CC += Tycoon.cc Protocol1ConnectionDock.cc

LIBS = base adl monkey_crypto monkey_net monkey_genode_utils libc vfs
