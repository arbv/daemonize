# default build configuration
CFG ?= config.mk

include $(CFG)
include proj.mk

# Executable path
BINPATH = $(PREFIX)/bin

# Manual prefix
MANPREFIX = $(PREFIX)/share/man

# Generate object file names from source file names
# C and C++ objects
COBJ = $(SRC:.c=.${O}) $(CXX_SRC:.cpp=.${O})
# Assembler objects
ASOBJ = $(AS_SRC:.S=.${O})
# All objects
OBJ = $(COBJ) $(ASOBJ)

.PHONY : all debug clean install uninstall

all: $(OUT)

.S.$(O) :
	@echo "AS" $<
	@$(AS) $(ASFLAGS) -o $@ $<

.c.$(O) :
	@echo "CC" $<
	@$(CC) $(CFLAGS) $(DEFINES) -c $<

.cpp.$(O) :
	@echo "CXX" $<
	@$(CXX) $(CXXFLAGS) $(DEFINES) -c $<

$(COBJ) : $(HEADERS) $(CXX_HEADERS)

$(ASOBJ) : $(AS_INC)

$(OUT) : $(OBJ)
	@echo "LD" $@
	@$(LD) $(LDFLAGS) -o $(OUT) $(OBJ) $(LIBS)

clean :
	@rm -f $(OBJ)
	@rm -f $(OUT) $(OUT).exe

# Debug target
debug :
	@echo "DEBUG build"
	@$(MAKE) -s -f $(MAKEFILE) DEBUG="1"

install : all
	@echo "INSTALL to: $(PREFIX)"
	@mkdir -p $(BINPATH) 
	@echo "CP $(OUT) $(BINPATH)"
	@cp -f $(OUT) $(BINPATH)
	@chmod 755 $(BINPATH)/$(OUT)
ifdef STRIP
	@$(STRIP) $(BINPATH)/$(OUT)
endif
# If defined manual files
ifdef MANFILES
	@for i in ${MANFILES} ; \
	do \
		manfile="$$(basename $${i})"; \
		mancat="$${manfile##*.}"; \
		manpath="${MANPREFIX}/man$${mancat}"; \
		\
		echo "CP $${manfile} $${manpath}"; \
		mkdir -p $${manpath}; \
		cp -f $${i} $${manpath}; \
		chmod 644 $${manpath}/$${manfile}; \
	done
endif

uninstall :
	@echo "UNINSTALL from: $(PREFIX)"
	@echo "RM $(BINPATH)/$(OUT)"
	@rm -f $(BINPATH)/$(OUT)
# If defined manual files
ifdef MANFILES
	@for i in ${MANFILES} ; \
	do \
		manfile="$$(basename $${i})"; \
		mancat="$${manfile##*.}"; \
		manpath="${MANPREFIX}/man$${mancat}"; \
		\
		echo "RM $${manpath}/$${manfile}"; \
		rm -f $${manpath}/$${manfile}; \
	done
endif

