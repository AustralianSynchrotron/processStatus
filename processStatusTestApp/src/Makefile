# $File: //ASP/tec/epics/processStatus/trunk/processStatusTestApp/src/Makefile $
# $Revision: #1 $
# $DateTime: 2016/06/04 15:30:06 $
# Last checked in by: $Author: starritt $
#

TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = processStatusTest
# processStatusTest.dbd will be created and installed
DBD += processStatusTest.dbd

# processStatusTest.dbd will be made up from these files:
processStatusTest_DBD += base.dbd

# Include dbd files from all support applications:
#
processStatusTest_DBD += processStatus.dbd

# Add all the support libraries needed by this IOC
#
processStatusTest_LIBS += asyn
processStatusTest_LIBS += processStatus

# processStatusTest_registerRecordDeviceDriver.cpp derives from processStatusTest.dbd
processStatusTest_SRCS += processStatusTest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
processStatusTest_SRCS_DEFAULT += processStatusTestMain.cpp
processStatusTest_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#processStatusTest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
processStatusTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# end