#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/74e416bf/dn_hdlc.o \
	${OBJECTDIR}/_ext/74e416bf/dn_ipmg.o \
	${OBJECTDIR}/_ext/74e416bf/dn_ipmt.o \
	${OBJECTDIR}/_ext/74e416bf/dn_serial_mg.o \
	${OBJECTDIR}/_ext/74e416bf/dn_serial_mt.o \
	${OBJECTDIR}/dn_endianness.o \
	${OBJECTDIR}/dn_lock.o \
	${OBJECTDIR}/dn_uart.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/simplepublish

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/simplepublish: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/simplepublish ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/74e416bf/dn_hdlc.o: ../../../../sm_clib/sm_clib/dn_hdlc.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/74e416bf
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/74e416bf/dn_hdlc.o ../../../../sm_clib/sm_clib/dn_hdlc.c

${OBJECTDIR}/_ext/74e416bf/dn_ipmg.o: ../../../../sm_clib/sm_clib/dn_ipmg.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/74e416bf
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/74e416bf/dn_ipmg.o ../../../../sm_clib/sm_clib/dn_ipmg.c

${OBJECTDIR}/_ext/74e416bf/dn_ipmt.o: ../../../../sm_clib/sm_clib/dn_ipmt.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/74e416bf
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/74e416bf/dn_ipmt.o ../../../../sm_clib/sm_clib/dn_ipmt.c

${OBJECTDIR}/_ext/74e416bf/dn_serial_mg.o: ../../../../sm_clib/sm_clib/dn_serial_mg.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/74e416bf
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/74e416bf/dn_serial_mg.o ../../../../sm_clib/sm_clib/dn_serial_mg.c

${OBJECTDIR}/_ext/74e416bf/dn_serial_mt.o: ../../../../sm_clib/sm_clib/dn_serial_mt.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/74e416bf
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/74e416bf/dn_serial_mt.o ../../../../sm_clib/sm_clib/dn_serial_mt.c

${OBJECTDIR}/dn_endianness.o: dn_endianness.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dn_endianness.o dn_endianness.c

${OBJECTDIR}/dn_lock.o: dn_lock.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dn_lock.o dn_lock.c

${OBJECTDIR}/dn_uart.o: dn_uart.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dn_uart.o dn_uart.c

${OBJECTDIR}/main.o: main.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/simplepublish

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
