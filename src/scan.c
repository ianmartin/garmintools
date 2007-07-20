#include "config.h"
#include <string.h>
#include <ctype.h>
#include "garmin.h"

/* 
   This file contains functions that scan the output of the functions in
   print.c, reconstructing Garmin datatypes from the XML output.
*/


static void
garmin_scan_attribute ( const char ** cursor, const char * name, char ** val )
{
  
}


static void
garmin_scan_type ( const char ** cursor, uint32 * type )
{
  *type = 0;

  /* Expect a string 'type="xxx"' */

  
}


static void
garmin_scan_position ( )
{

}


