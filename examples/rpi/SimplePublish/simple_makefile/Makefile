#Target binary program
TARGET = main

#Relative path to directories for SmartMesh C and QuickStart Libraries
DIR_CLIB	= sm_clib
DIR_QSL		= sm_qsl
#DIR_CLIB	= ../../../sm_clib/sm_clib
#DIR_QSL		= ../../../sm_qsl

#Object directory
ODIR = obj

#Compiler and linker
CC = gcc

#Flags, Libraries and Includes
LIBS	= -lpthread
CFLAGS	= -Wall -I$(DIR_CLIB) -I$(DIR_QSL)
EXT		= .c

#Object files for source, C Library and QuickStart Library
_OBJ		= main.o dn_time.o dn_watchdog.o dn_uart.o dn_endianness.o dn_lock.o
_OBJ_QSL	= dn_fsm.o
_OBJ_CLIB	= dn_ipmt.o dn_serial_mt.o dn_hdlc.o

#Header files in source, C Library and QuickStart Library
_DEPS		=
_DEPS_QSL	= dn_qsl_api.h dn_fsm.h dn_time.h dn_watchdog.h dn_defaults.h dn_debug.h
_DEPS_CLIB	= dn_ipmt.h dn_serial_mt.h dn_hdlc.h dn_uart.h dn_endianness.h dn_lock.h dn_common.h


#Append object files with relative paths inside object directory
ODIR_QSL	= $(ODIR)/$(DIR_QSL)
ODIR_CLIB	= $(ODIR)/$(DIR_CLIB)
OBJ			= $(patsubst %, $(ODIR)/%, $(_OBJ))
OBJ_QSL		= $(patsubst %, $(ODIR_QSL)/%, $(_OBJ_QSL))
OBJ_CLIB	= $(patsubst %, $(ODIR_CLIB)/%, $(_OBJ_CLIB))

#Append header files with their relative path
DEPS = $(_DEPS)
DEPS_QSL = $(patsubst %,$(DIR_QSL)/%,$(_DEPS_QSL))
DEPS_CLIB = $(patsubst %, $(DIR_CLIB)/%, $(_DEPS_CLIB))

#Collect all objects and header files for target
OBJ_ALL = $(OBJ) $(OBJ_QSL) $(OBJ_CLIB)
DEPS_ALL = $(DEPS) $(DEPS_QSL) $(DEPS_CLIB)

#Default make
all: prebuild $(TARGET)

#Build object directories
prebuild:
	@mkdir -p $(ODIR)
	@mkdir -p $(ODIR_QSL)
	@mkdir -p $(ODIR_CLIB)

#Clean before building
remake: clean all

#Delete object directory and target
clean:
	@rm -rf $(ODIR) $(TARGET)

#Link
$(TARGET): $(OBJ_ALL)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

#Compile source
$(ODIR)/%.o: %$(EXT) $(DEPS_ALL)
	$(CC) -c -o $@ $< $(CFLAGS)
#Comlile QuickStart Library
$(ODIR_QSL)/%.o: $(DIR_QSL)/%$(EXT) $(DEPS_ALL)
	$(CC) -c -o $@ $< $(CFLAGS)
#Compile C Library
$(ODIR_CLIB)/%.o: $(DIR_CLIB)/%$(EXT) $(DEPS_ALL)
	$(CC) -c -o $@ $< $(CFLAGS)

#None-file targets
.PHONY: all prebuild remake clean
