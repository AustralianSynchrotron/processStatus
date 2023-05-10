# $File: //ASP/tec/epics/processStatus/trunk/processStatusSup/src/Makefile $
# $Revision: #1 $
# $DateTime: 2016/06/04 15:30:06 $
# Last checked in by: $Author: starritt $
#
TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#========================================

#========================================
# build ProcessStatus support library
#
LIBRARY_IOC += processStatus

# install processStatus.dbd into <top>/dbd
#
DBD += processStatus.dbd

# specify all source files to be compiled and added to the library
#
processStatus_SRCS += processStatus.cpp

processStatus_LIBS += asyn
processStatus_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end
