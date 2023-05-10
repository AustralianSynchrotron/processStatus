/* $File: //ASP/tec/epics/processStatus/trunk/processStatusSup/src/processStatus.h $
 * $Revision: #2 $
 * $DateTime: 2020/11/07 15:44:29 $
 * Last checked in by: $Author: starritt $
 *
 * Description:
 * This is an ASYN port driver that determines if the nominated process in running.
 *
 * This can be by either:
 * a/ process name only; or
 * b/ qualified by checking a nominated paramemeter contain specified sub-string.
 *    This may be extended to allow use of a regular expression in a later version.
 *
 * Examples:
 * ProcessStatus_Initialise (2)
 * ProcessStatus_Configure ("DIA",    "ArchiveEngine", 7, "/dia/engineconfig.xml")
 * ProcessStatus_Configure ("DAEMON", "/usr/bin/perl", 2, "/ArchiveDaemon.pl")
 * ProcessStatus_Configure ("HTTP",   "httpd", 0, "")
 *
 *
 * Copyright (c) 2016-2020 Australian Synchrotron
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * Licence as published by the Free Software Foundation; either
 * version 2.1 of the Licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * Licence along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact details:
 * firstname.andrew@synchrotron.org.au
 * 800 Blackburn Road, Clayton, Victoria 3168, Australia.
*/

#ifndef PROCESS_STATUS_H_
#define PROCESS_STATUS_H_

#include <regex.h>
#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsThread.h>
#include <shareLib.h>
#include <asynPortDriver.h>

class epicsShareClass ProcessStatusDriver : public asynPortDriver {
public:
   explicit ProcessStatusDriver (const char* port_name,
                                 const char* process_name,
                                 const int argument_index,
                                 const char* argument_regex);
   ~ProcessStatusDriver ();

   enum Qualifiers {
      DriverVersion = 0,         // EPICS driver version
      ProcessStatus,             // Process Status
      ProcessCount,              // Process Count
      ProcessIdentity,           // Process (Unique, when count = 1) Id
      //
      NUMBER_QUALIFIERS          // Number of qualifiers - MUST be last
   };

   // Overide asynPortDriver functions needed for this driver.
   //
   void report (FILE* fp, int details);
   asynStatus readOctet (asynUser* pasynUser, char* value, size_t maxChars,
                         size_t* nActual, int* eomReason);
   asynStatus readUInt32Digital (asynUser* pasynUser, epicsUInt32* value, epicsUInt32 mask);
   asynStatus readInt32 (asynUser *pasynUser, epicsInt32 *value);

private:
   static const char* qualifierImage (Qualifiers qualifer);
   Qualifiers reasonToQualifier (const int reason) const;
   int extractProcessInfo (int& pid) const;

   int indexList [NUMBER_QUALIFIERS];  // used by asynPortDriver

   char process_name [200];
   int argument_index;
   char argument_regex [200];
   regex_t regex;

   char full_name [80];
   int is_initialised;    // Init failed.
};

#endif   // PROCESS_STATUS_H_
