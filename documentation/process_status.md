# Process Status Module

## Introduction

This module is an ASYN port driver that can provide process information.
The EPICS IOC and the processorp processes of interest _must_ run on the same
Linux host.

The documentation still under development.
Look at the test app for example on usage.

## Identifying process to monitor

There are two commands that must be run in IOC's st.cmd, or equivilent, file.

#### ProcessStatus_Initialise

This command both initialises the driver, and sets the weeor warnong level.
The command takes a single integer parameter.
The expected values are:
- 0 report errors only
- 1 report errors and warnings
- 2 report errors, warnings and info

Example:

    ProcessStatus_Initialise (2)


#### ProcessStatus_Configure

This command identifies the process to monitor.
The command takes four parameters.
These are:

- 1:  The asyn port name.
- 2:  The program name (out of /proc/&lt;pid&gt;/cmdline).
Examples: /bin/bash, kwrite.
__Note:__ how the process was launched impacts wheather full path name or
just the basename appears in /proc/&lt;pid&gt;/cmdline.
This parameter _must_ match exactly.
- 3:  When you want to distiguish between different instances of the same executable,
this nominates the parameter number (or 0 if not applicable) to be used to
identify each instance,
- 4:  When #3 is specified, this provides a regular expression used to match the parameter.

Examples:

    ProcessStatus_Configure ("PSBASH",  "/bin/bash",                 0, "")
    ProcessStatus_Configure ("PSINIT",  "/usr/lib/systemd/systemd",  0, "")
    ProcessStatus_Configure ("PSSLEEP", "sleep",                     1, "^500$")
    ProcessStatus_Configure ("PSKRITE", "kwrite",                    1, "st.cmd")

#### Loading the template (example)

    dbLoadRecords ("db/process_status.template", "PORT=PSBASH,  PROCESS=BASH,   DESC=any bash")
    dbLoadRecords ("db/process_status.template", "PORT=PSINIT,  PROCESS=INIT,   DESC=systemd")
    dbLoadRecords ("db/process_status.template", "PORT=PSSLEEP, PROCESS=SLEEP,  DESC=sleep 500")
    dbLoadRecords ("db/process_status.template", "PORT=PSKRITE, PROCESS=KWRITE, DESC=kwrite st.cmd")

A substituion file may be more appropriate if many processes are being monitored.

## EPICS Database template

The module includes process_status.template which can be used as is, or as a
guide or inspiration to creating your template file.

The template provides three records. These are described below.

#### $(PROCESS):STATUS

This is an mbbi record.
The status are:

| State    | Severity | Comment                                    |
|:---------|:---------|:-------------------------------------------|
| Unknown  | INVALID  | Undefined                                  |
| Stopped  | MAJOR    | No processes match the specified criteria  |
| Running  | NO_ALARM | One process matches the criteria           |
| Multiple | MINOR    | Tow or more processes matches the criteria |


#### $(PROCESS):COUNT

This is a longin record. In provides a count of the number of processes that
match the specified criteria.

#### $(PROCESS):PID

This is a longin record. If a single process matches the specified criteria,
i.e. the status is running, this record provides the process id.
If no processes or multiple processes match, the record is set to 0.

<font size="-1">Last updated: Sat May 13 18:10:14 2023</font>
<br>
