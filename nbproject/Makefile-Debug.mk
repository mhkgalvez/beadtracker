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
CCC=g++-4.8
CXX=g++-4.8
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Frame.o \
	${OBJECTDIR}/GeneralException.o \
	${OBJECTDIR}/OpenVideoException.o \
	${OBJECTDIR}/ReadException.o \
	${OBJECTDIR}/VideoStream.o \
	${OBJECTDIR}/common.o \
	${OBJECTDIR}/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-m64 -O3
CXXFLAGS=-m64 -O3

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lavcodec -lavformat -lavutil -lswscale `pkg-config --libs opencv` -ljpeg -lpthread   

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/beadtracker

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/beadtracker: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	g++-4.8 -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/beadtracker ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/Frame.o: Frame.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/Frame.o Frame.cpp

${OBJECTDIR}/GeneralException.o: GeneralException.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeneralException.o GeneralException.cpp

${OBJECTDIR}/OpenVideoException.o: OpenVideoException.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/OpenVideoException.o OpenVideoException.cpp

${OBJECTDIR}/ReadException.o: ReadException.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/ReadException.o ReadException.cpp

${OBJECTDIR}/VideoStream.o: VideoStream.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/VideoStream.o VideoStream.cpp

${OBJECTDIR}/common.o: common.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/common.o common.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -Wall `pkg-config --cflags opencv` -std=c++11 -O3 -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/beadtracker

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
