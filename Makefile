ifeq ($(ERL_ROOT),)
	export ERL_ROOT=$(shell erl -noshell -eval 'io:format("~s~n", [code:root_dir()]), init:stop().')
endif

ifeq (,$(ERL_ROOT))
$(error "Unable to locate erlang root directory! Is erlang installed?")
endif

#Search ERL_ROOT for erl_interface
ERL_INTERFACE_ROOT = $(addprefix $(ERL_ROOT)/lib/, $(shell ls $(ERL_ROOT)/lib | grep erl_interface))
ERL_INTERFACE_INCLUDE = $(ERL_INTERFACE_ROOT)/include
ERL_INTERFACE_LIB = $(ERL_INTERFACE_ROOT)/lib

###################################################################################
GTEST_DIR=/home/j/lab/gtest-1.7.0/
MOCKCPP_HEADER_DIR=/home/j/lab/mockcpp/mockcpp/include
MOCKCPP_LIB_DIR=/usr/include/mockcpp/lib

WARNFLAGS = -Wall -Wno-invalid-offsetof -Wno-c++0x-compat
CPPFLAGS= $(WARNFLAGS) -g -I/usr/include -I$(GTEST_DIR)include -I$(MOCKCPP_HEADER_DIR)
LDFLAGS=
LDLIBS=-L$(GTEST_DIR)lib -L$(MOCKCPP_LIB_DIR) -lgtest -lpthread -lmockcpp -lpcrecpp

#TEST_SRCS=$(shell find test -iname *.cpp)
TEST_SRCS=test/test.cpp test/main.cpp
TEST_OBJS=$(subst .cpp,.o,$(TEST_SRCS))


###################################################################################
all: $(TEST_OBJS)
#g++ -I/usr/include -o msgflow src/msgflow.cpp -lpthread -lpcrecpp -lboost_regex
	@g++ $(CPPFLAGS) $(LDFLAGS) -g -O2 -o main $^ $(LDLIBS) -I$(ERL_INTERFACE_INCLUDE) -L$(ERL_INTERFACE_LIB) -lerl_interface -lei -lpthread -lnsl && ./main --gtest_filter=* && echo "" && echo "" && echo ""

clean:
	rm -rdf $(OBJS) main main.dSYM test/*.o msgflow
