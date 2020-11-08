#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

CC := gcc
CXX := g++
CFLAGS := -Wall -std=c++11 -fPIC 
#CFLGAS += -Wno-psabi
VER := 1.189.9026

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -O0 -ggdb
else
    CFLAGS += -DNDEBUG -O3
endif

UNAME_S := $(shell uname -s | tr A-Z a-z)
ARCH := $(shell uname -p | tr A-Z a-z)

ifeq ($(UNAME_S), darwin)
	LIBNAME=libengage-shared.dylib
else ifeq ($(ARCH), x86_64)
	LIBNAME=libengage-shared.so
else ifeq ($(ARCH), aarch64)
	LIBNAME=libengage-shared.so
endif

INCLUDES =  -I. -I./engage
LFLAGS = -L./engage
LIBS = -lengage-shared -lpthread -lstdc++
SRCS = Mingage.cpp WorkQueue.cpp
OBJS = $(SRCS:.cpp=.o)

MAIN = mingage

.PHONY: depend clean

all:    $(MAIN)	

$(MAIN): $(OBJS) 
	$(CXX) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)
	@echo ""
	@echo "****************************** IMPORTANT ******************************"
	@echo "$(LIBNAME) has been placed in './engage'"
	@echo "Be sure to 'export LD_LIBRARY_PATH=./engage' if your OS requires it"
	@echo "***********************************************************************"

.cpp.o:
	$(CXX) -c $(CFLAGS) $(INCLUDES) $<  -o $@

clean:
	${RM} *.o *~ $(MAIN)
	${RM} -r engage

depend: depends
depends:
	@getengage.sh $(VER)
	
# DO NOT DELETE THIS LINE -- make depend needs it
