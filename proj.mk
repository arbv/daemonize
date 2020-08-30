# Target name
TARGETS = example_linux example_portable

# Project C sources
SRC = $(filter-out  $(LEX_YACC_OUT_SRC),$(wildcard *.c */*.c)) $(LEX_YACC_OUT_SRC)

# Project C headers
# HEADERS = $(filter-out  $(LEX_YACC_OUT_HEADERS),$(wildcard *.h */*.h)) $(LEX_YACC_OUT_HEADERS)

# Project C++ sources
CXX_SRC =  $(wildcard *.cpp */*.cpp)

# Project C++ headers
#CXX_HEADERS = $(wildcard *.hpp */*.hpp) $(HEADERS)

# parsers (bison)
YFILES = $(wildcard *.y */*.y)

# lexers (flex)
LFILES = $(wildcard *.l */*.l)

# Assembler sources
AS_SRC =

# Assembler headers
AS_HEADERS =

# Manuals
MANFILES =

# Makefiles
MKFILES = $(MAKEFILE_PATH) $(wildcard $(MAKEFILE_DIR)/*.mk)

# Lex and Yacc generated sources
LEX_YACC_OUT_SRC = $(YFILES:.y=.tab.c) $(LFILES:.l=.lex.c)
LEX_YACC_OUT_HEADERS = $(YFILES:.y=.tab.h)
