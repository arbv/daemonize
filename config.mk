# Project dependency libraries
LDLIBS = 

# Install prefix
PREFIX = /usr/local

# Executable path
BINPREFIX ?= $(PREFIX)/bin
# Manual prefix
MANPREFIX ?= $(PREFIX)/share/man

# executable file extension (e.g. for Windows)
# (PATHEXT is defined on Windows)
ifdef PATHEXT
EXE = exe
endif
# Object file extension
O = o

# C compiler
CC = gcc
# C++ compiler
CXX = g++
# C and C++ compiler common flags
CFLAGS_COMMON= -Wall -pthread -ffunction-sections -MD -pipe
# C++ common compiler flags
CXXFLAGS_COMMON = $(CFLAGS) -std=c++11
# Set the following variable if C and C++ compilers generate
# dependency files as a side effect of compilation.
GENDEP=1

# linker
ifeq ($(strip $(CXX_SRC)),)
LD = $(CXX)
else
LD = $(CC)
endif
LDFLAGS_COMMON = -Wl,--gc-sections
# assembler
AS = as
# lib archiever
AR = ar
ARFLAGS = rcs
#strip
STRIP = strip
# object type
OBJTYPE = $(lastword $(subst -, ,$(CC)))-$(shell ${CC} -dumpmachine)
# parser generator and its flags
YACC = bison
YFLAGS = -d
# lexer generator and its flags
LEX = flex
LFLAGS =

# common defines
DEFINES =

# Default release options
ifndef DEBUG

# Release only defines
DEFINES += -D NDEBUG

# C compiler flags
CFLAGS = $(CFLAGS_COMMON) -O2 -g -flto

# C++ compiler flags
CXXFLAGS = $(CFLAGS) $(CXXFLAGS_COMMON)

# Linker flags
LDFLAGS = $(LDFLAGS_COMMON) -flto

# Assembler flags
ASFLAGS = 

# Debug target
else

# Additional debuf define flags
DEFINES += -D DEBUG

# Override default flags
CFLAGS = $(CFLAGS_COMMON) -O0 -g
CXXFLAGS = $(CFLAGS) $(CXXFLAGS_COMMON)
LDFLAGS = $(LDFLAGS_COMMON)
ASFLAGS =

# Append additional debug libraries
LDLIBS += 
DEFINES +=

endif

