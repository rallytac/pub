#
# Copyright (c) 2020 Rally Tactical Systems, Inc.
#

CC := gcc
CXX := g++
CFLAGS := -Wall -std=c++11
#CFLAGS += -Wno-psabi
# Engage engine version: required for depend/depends (see getengage.sh). Example:
#   make depend VER=1.255.9095
#   make depend VER=1.255.9095 ENGAGE_LINE=interim
ENGAGE_LINE ?= interim

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -O0 -ggdb
else
    CFLAGS += -DNDEBUG -O3
endif

UNAME_S := $(shell uname -s | tr A-Z a-z)
ARCH := $(shell uname -p | tr A-Z a-z)

INCLUDES = -I. -I./engage -I./engage/include
SRCS = Mingage.cpp WorkQueue.cpp
OBJS = $(SRCS:.cpp=.o)

MAIN = mingage

ENGAGE_LIB := ./engage/libengage-static-native.a

ifeq ($(UNAME_S),darwin)
    ENGAGE_LIBS := $(ENGAGE_LIB) -lpthread \
        -framework CoreFoundation \
        -framework SystemConfiguration \
        -framework CoreAudio \
        -framework AudioToolbox \
        -framework Security
else
    ENGAGE_LIBS := $(ENGAGE_LIB) -lpthread -ldl
endif

.PHONY: depend depends clean

all: $(MAIN)

$(MAIN): $(OBJS) $(ENGAGE_LIB)
	$(CXX) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(ENGAGE_LIBS)

.cpp.o:
	$(CXX) -c $(CFLAGS) $(INCLUDES) $< -o $@
	
clean:
	${RM} *.o *~ $(MAIN)
	${RM} -r engage
	@mkdir -p engage

depend: depends
depends:
	@if [ -z "$(strip $(VER))" ]; then \
		echo >&2 "VER is required (Engage engine version). Example: make depend VER=1.255.9095 [ENGAGE_LINE=$(ENGAGE_LINE)]"; \
		exit 1; \
	fi
	@./getengage.sh $(VER) $(ENGAGE_LINE)

# DO NOT DELETE THIS LINE -- make depend needs it
