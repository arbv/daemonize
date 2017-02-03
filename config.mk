# Project dependency libraries
LIBS = 

# Install prefix
PREFIX ?= /usr/local

# Object file suffix
O = o
# dll suffix
SO = so
# static lib suffix
A = a

# C compiler
CC ?= cc
# C++ compiler
CXX ?= g++
# linker
LD = $(CC)
# assembler
AS ?= as
# lib archiever
AR ?= ar
ARFLAGS ?= rcs

#strip
STRIP ?= strip

# common defines
DEFINES ?=

# Default release options
ifndef DEBUG

# Release only defines
DEFINES +=

# C compiler flags
CFLAGS ?= -O2 -Wall -g

# C++ compiler flags
CXXFLAGS ?= $(CFLAGS)

# Linker flags
LDFLAGS ?= 

# Assembler flags
ASFLAGS ?= 

else # Debug target

# Additional debuf define flags
DEFINES += -D DEBUG

# Override default flags
CFLAGS ?= -O0 -g -Wall
CXXFLAGS ?= $(CFLAGS)
LDFLAGS ?=
ASFLAGS ?=
#YFLAGS ?=
#LFLAGS ?=

# Append additional debug libraries
LIBS += 
DEFINES +=

endif

