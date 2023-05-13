/* $File: //ASP/tec/epics/processStatus/trunk/processStatusSup/src/processStatus.cpp $
 * $Revision: #2 $
 * $DateTime: 2023/05/13 17:23:14 $
 * Last checked in by: $Author: starritt $
 *
 * Description:
 * This is an ASYN port driver to support process status.
 *
 * Copyright (c) 2016-2023 Australian Synchrotron
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * Licence as published by the Free Software Foundation; either
 * version 3 of the Licence, or (at your option) any later version.
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
 * Original author: Andrew Starritt
 * Maintained by:   Andrew Starritt
 *
 * Contact details:
 * as-open-source@ansto.gov.au
 * 800 Blackburn Road, Clayton, Victoria 3168, Australia.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <epicsExport.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <errlog.h>
#include <iocsh.h>

#include <asynParamType.h>
#include <asynCommonSyncIO.h>

#include "processStatus.h"

// Useful type neutral numerical macro fuctions.
//
#define ABS(a)              ((a) >= 0  ? (a) : -(a))
#define MAX(a, b)           ((a) >= (b) ? (a) : (b))
#define MIN(a, b)           ((a) <= (b) ? (a) : (b))
#define LIMIT(x,low,high)   (MAX(low, MIN(x, high)))

// Calculates number of items in an array
//
#define ARRAY_LENGTH(xx)   ((int) (sizeof (xx) /sizeof (xx [0])))

#define PROCESS_STATUS_DRIVER_VERSION   "1.1.5"

struct QualifierDefinitions {
   asynParamType type;
   const char* name;
};

// MUST be consistent with enum Qualifiers type out of ProcessStatusDriver (in processStatus.h)
//
static const QualifierDefinitions qualifierList [] = {
   {asynParamOctet,          "DRVVER"  },  // DriverVersion
   {asynParamUInt32Digital,  "STATUS"  },  // Process Status, zero, one or many
   {asynParamInt32,          "COUNT"   },  // Number of matches
   {asynParamInt32,          "PID"     }   // Process Id (when unique)
};

// Supported interrupts.
//
const static int interruptMask = 0;

// Any interrupt must also have an interface.
//
const static int interfaceMask = interruptMask | asynDrvUserMask |
                                 asynOctetMask | asynUInt32DigitalMask | asynInt32Mask;

const static int asynFlags = ASYN_CANBLOCK;

// Static data.
//
static int verbosity = 4;               // High until set low
static int driver_initialised = false;


//------------------------------------------------------------------------------
// Local functions
//------------------------------------------------------------------------------
//
static void devprintf (const int required_min_verbosity,
                       const char* function,
                       const int line_no,
                       const char* format, ...)
{
   if (verbosity >= required_min_verbosity) {
      char message [100];
      va_list arguments;
      va_start (arguments, format);
      vsnprintf (message, sizeof (message), format, arguments);
      va_end (arguments);
      errlogPrintf ("ProcessStatusDriver: %s:%d  %s", function, line_no, message);
   }
}

// Wrapper macros to devprintf.
//
#define ERROR(...)    devprintf (0, __FUNCTION__, __LINE__, __VA_ARGS__);
#define WARNING(...)  devprintf (1, __FUNCTION__, __LINE__, __VA_ARGS__);
#define INFO(...)     devprintf (2, __FUNCTION__, __LINE__, __VA_ARGS__);
#define DETAIL(...)   devprintf (3, __FUNCTION__, __LINE__, __VA_ARGS__);


//==============================================================================
//
//==============================================================================
//
ProcessStatusDriver::Qualifiers
ProcessStatusDriver::reasonToQualifier (const int reason) const
{
   return Qualifiers (reason - this->indexList [0]);
}

//------------------------------------------------------------------------------
// static
//
const char* ProcessStatusDriver::qualifierImage (const Qualifiers q)
{
   static char result [24];

   if ((q >= 0) && (q < NUMBER_QUALIFIERS)) {
      return qualifierList[q].name;
   } else {
      sprintf (result, "unknown (%d)", q);
      return result;
   }
}

//------------------------------------------------------------------------------
// static
//
static void ProcessStatus_Initialise (const int verbosityIn)
{
   verbosity = verbosityIn;
   driver_initialised = true;
}

//------------------------------------------------------------------------------
//
ProcessStatusDriver::ProcessStatusDriver (const char* port_name,
                                          const char* process_name,
                                          const int argument_index,
                                          const char* argument_regex) :

   asynPortDriver (port_name,             //
                   0,                     //
                   interfaceMask,         //
                   interruptMask,         //
                   asynFlags,             //
                   1,                     // Autoconnect
                   0,                     // Default priority
                   0)                     // Default stack size

{
   asynStatus status;

   this->is_initialised = false;   // flag object as not initialised.

   if (!driver_initialised) {
      ERROR ("driver not initialised (call %s first)\n", "ProcessStatus_Initialise");
      return;
   }

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Verify port name/process_name/regex are sensible
   //
   if ((port_name == NULL) || (strcmp (port_name, "") == 0)) {
      ERROR ("null/empty port name\n",0);
      return;
   }

   if ((process_name == NULL) || (strcmp (process_name, "") == 0)) {
      ERROR ("null/empty process name\n", 0);
      return;
   }

   if (argument_index > 0) {
      if ((argument_regex == NULL) || (strcmp (argument_regex, "") == 0)) {
         ERROR ("null/empty argument regular expresion\n", 0);
         return;
      }
   }

   this->argument_index = argument_index;
   snprintf (this->process_name, sizeof (this->process_name), "%s", process_name);

   // Note: null argument_sub_str translated to empty string.
   //
   snprintf (this->argument_regex, sizeof (this->argument_regex), "%s", argument_regex ? argument_regex : "");

   snprintf (this->full_name, sizeof (this->full_name), "%s-%s", port_name, process_name);

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   // Set up asyn parameters.
   //
   for (int j = 0; j < ARRAY_LENGTH (qualifierList); j++) {
      status = this->createParam (qualifierList[j].name,
                                  qualifierList[j].type,
                                  &this->indexList[j]);
      if (status != asynSuccess) {
          ERROR ("%s createParam %d '%s' failed\n", this->full_name, j, qualifierList[j].name)
          return;
      }
   }

   // Attempt to compile the regular expression.
   //
   int reg_comp_status;
   reg_comp_status = regcomp (&this->regex, this->argument_regex, 0);
   if (reg_comp_status != 0) {
      ERROR ("regcomp (...,'%s', 0) failed, status: %d\n", this->argument_regex, reg_comp_status);
      return;
   }

   this->is_initialised = true;
   INFO ("%s initialisation complete\n", this->full_name);
}

//------------------------------------------------------------------------------
//
ProcessStatusDriver::~ProcessStatusDriver ()
{
   if (this->is_initialised) {
      regfree (&this->regex);
   }
}


//------------------------------------------------------------------------------
// Based on suggestions from:
// http://stackoverflow.com/questions/6898337/determine-programmatically-if-a-program-is-running
//
int ProcessStatusDriver::extractProcessInfo (int &pid) const
{
   int result = 0;
   DIR *dir;
   struct dirent *ent;

   pid = -1;

   dir = opendir ("/proc");
   if (!dir) {
      ERROR ("can't open /proc");
      return false;
   }

   while ((ent = readdir (dir)) != NULL) {
      char* endptr;
      char proc_file[40];
      char cmdline_buffer[1024];

      // If endptr is not a null character, the directory is not
      // entirely numeric, so ignore it
      //
      const long lpid = strtol (ent->d_name, &endptr, 10);
      if (*endptr != '\0') {
         continue;
      }

      // Try to open the cmdline file
      //
      snprintf (proc_file, sizeof (proc_file), "/proc/%ld/cmdline", lpid);

      const int proc_fd = open (proc_file, 0);
      if (proc_fd < 0) {
         WARNING ("Can't open: %s\n", proc_file);
         continue;
      }

      const size_t num_read = read (proc_fd, cmdline_buffer, sizeof (cmdline_buffer));
      close (proc_fd);

      if (num_read <= 0) {
         DETAIL ("No bytes read from: %s\n", proc_file);
         continue;
      }

      // cmdline is collection of tokens seprated by zero ('\0') characters.
      //
      int argc;
      char *argv[20];
      for (argc = 0; argc < ARRAY_LENGTH (argv); argc++) argv[argc] = NULL;

      argv[0] = cmdline_buffer;  // first token is the process name.
      argc = 1;

      // Does process name match?
      //
      if (strcmp (argv[0], this->process_name) != 0) {
         // No - skip to next entry
         continue;
      }

      // Has a match argument been specified?
      //
      if (this->argument_index <= 0) {
         // No - match on process name only
         //
         pid = (int) lpid;
         result += 1;
         continue;
      }

      // Parse the command line parameters.
      //
      const int last = num_read - 1;
      for (int j = 1; (j < last) && (argc < ARRAY_LENGTH (argv)); j++) {
         if (cmdline_buffer[j] == '\0') {
            argv[argc] = &cmdline_buffer[j + 1];
            argc += 1;
         }
      }

      if ((argument_index > 0) && (argument_index < argc)) {
         const char* line = argv [argument_index];

         int regex_status = regexec (&this->regex, line, 0, NULL, 0);
         if (regex_status == 0) {
            // We have a match.
            //
            pid = (int) lpid;
            result += 1;
            continue;
         }
      }
   }

   closedir (dir);
   return result;
}

//------------------------------------------------------------------------------
// Asyn callback functions
//------------------------------------------------------------------------------
//
void ProcessStatusDriver::report (FILE * fp, int details)
{
   if (details > 0) {
      fprintf (fp, "    driver info:\n");
      fprintf (fp, "        process name:   %s\n", this->process_name);
      fprintf (fp, "        argument index: %d\n", this->argument_index);
      fprintf (fp, "        regex:          '%s'\n", this->argument_regex);
      fprintf (fp, "        initialised:    %s\n", this->is_initialised ? "yes" : "no");
      fprintf (fp, "\n");
   }
}

//------------------------------------------------------------------------------
//
asynStatus ProcessStatusDriver::readOctet (asynUser* pasynUser,
                                           char* data, size_t maxchars,
                                           size_t* nbytesTransfered,
                                           int* eomReason)
{
   const Qualifiers qualifier = this->reasonToQualifier (pasynUser->reason);
   asynStatus status;
   size_t length;

   *nbytesTransfered = 0;
   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case DriverVersion:
         strncpy (data, PROCESS_STATUS_DRIVER_VERSION, maxchars);
         length = strlen (PROCESS_STATUS_DRIVER_VERSION);
         *nbytesTransfered = MIN (length, maxchars);
         break;

      default:
         ERROR ("%s Unexpected qualifier (%s)\n", this->full_name,
                qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus ProcessStatusDriver::readUInt32Digital (asynUser* pasynUser,
                                                   epicsUInt32* value,
                                                   epicsUInt32 mask)
{
   const Qualifiers qualifier = this->reasonToQualifier (pasynUser->reason);
   asynStatus status;
   int count;
   int pid;

   if (this->is_initialised != true) {
      return asynError;
   }

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case ProcessStatus:
         count = this->extractProcessInfo (pid);
         if (count > 2) count = 2;
         *value = (count + 1) & mask;
         break;

      default:
         ERROR ("%s Unexpected qualifier (%s)\n", this->full_name,
                qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
//
asynStatus ProcessStatusDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
   const Qualifiers qualifier = this->reasonToQualifier (pasynUser->reason);
   asynStatus status;
   int count;
   int pid;

   if (this->is_initialised != true) {
      return asynError;
   }

   status = asynSuccess;        // hypothesize okay

   switch (qualifier) {

      case ProcessCount:
         *value  = this->extractProcessInfo (pid);
         break;

      case ProcessIdentity:
         count = this->extractProcessInfo (pid);
         if (count == 1) {
            *value = pid;
         } else {
            *value = 0;
         }
         break;

      default:
         ERROR ("%s Unexpected qualifier (%s)\n", this->full_name,
                qualifierImage (qualifier));
         status = asynError;
         break;
   }

   return status;
}

//------------------------------------------------------------------------------
// IOC shell command registration
//------------------------------------------------------------------------------
//
// Define argument kinds
//
static const iocshArg verbosity_arg =        { "Verbosity (0 .. 4)", iocshArgInt    };
static const iocshArg port_name_arg =        { "ASYN port name",     iocshArgString };
static const iocshArg process_name_arg =     { "Process name",       iocshArgString };
static const iocshArg argument_number_arg =  { "Argument (0 .. N)",  iocshArgInt    };
static const iocshArg regex_arg =            { "Regular expression", iocshArgString };


//------------------------------------------------------------------------------
//
static const iocshArg *const ProcessStatus_Initialise_Args[1] = {
   &verbosity_arg,
};

static const iocshFuncDef ProcessStatus_Initialise_Func_Def = {
   "ProcessStatus_Initialise", 1, ProcessStatus_Initialise_Args
};

static void Call_ProcessStatus_Initialise (const iocshArgBuf* args)
{
   ProcessStatus_Initialise (args[0].ival);
}


//------------------------------------------------------------------------------
//
static const iocshArg *const ProcessStatus_Configure_Args[4] = {
   &port_name_arg,
   &process_name_arg,
   &argument_number_arg,
   &regex_arg
};

static const iocshFuncDef ProcessStatus_Configure_Func_Def = {
   "ProcessStatus_Configure", 4, ProcessStatus_Configure_Args
};

static void Call_ProcessStatus_Configure (const iocshArgBuf* args)
{
   new ProcessStatusDriver (args[0].sval, args[1].sval, args[2].ival, args[3].sval);
}

//------------------------------------------------------------------------------
//
static void ProcessStatusStartup (void) {
   printf ("ProcessStatusStartup (driver version %s)\n", PROCESS_STATUS_DRIVER_VERSION);

   iocshRegister (&ProcessStatus_Initialise_Func_Def,
                  Call_ProcessStatus_Initialise);

   iocshRegister (&ProcessStatus_Configure_Func_Def,
                  Call_ProcessStatus_Configure);

}

epicsExportRegistrar (ProcessStatusStartup);

// end
