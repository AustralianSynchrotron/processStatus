#!../../bin/linux-x86_64/processStatusTest
#
# $File: //ASP/tec/epics/processStatus/trunk/iocBoot/iocprocessStatusTest/st.cmd $
# $Revision: #2 $
# $DateTime: 2023/05/13 17:23:14 $
# Last checked in by: $Author: starritt $
#
## You may have to change processStatusTest to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/processStatusTest.dbd"
processStatusTest_registerRecordDeviceDriver pdbbase

# Do warnings
# 0 errors
# 1 errors and warnings
# 2 errors, warnings and info
#
ProcessStatus_Initialise (2)

# Parameters
# 1 asyn port name
# 2 program name (out of /proc/<pid>/cmdline) - full path required.
# 3 parameter number (or 0 if not applicable)
# 4 regular expression to match parameter.
#
ProcessStatus_Configure ("PSBASH",  "/bin/bash",                 0, "")
ProcessStatus_Configure ("PSINIT",  "/usr/lib/systemd/systemd",  0, "")
ProcessStatus_Configure ("PSSLEEP", "sleep",                     1, "^500$")
ProcessStatus_Configure ("PSKRITE", "kwrite",                    1, "st.cmd")

## Load record instances
#
dbLoadRecords ("db/process_status.template", "PORT=PSBASH,  PROCESS=BASH,   DESC=any bash")
dbLoadRecords ("db/process_status.template", "PORT=PSINIT,  PROCESS=INIT,   DESC=systemd")
dbLoadRecords ("db/process_status.template", "PORT=PSSLEEP, PROCESS=SLEEP,  DESC=sleep 500")
dbLoadRecords ("db/process_status.template", "PORT=PSKRITE, PROCESS=KWRITE, DESC=kwrite st.cmd")

cd "${TOP}/iocBoot/${IOC}"
iocInit
  
dbl
  
# end
