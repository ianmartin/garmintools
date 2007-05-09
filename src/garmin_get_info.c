#include <stdio.h>
#include "garmin.h"


int
main ( int argc, char ** argv )
{
  garmin_unit   garmin;

  if ( garmin_init(&garmin) != 0 ) {
    garmin_print_info(&garmin,stdout,0);
  } else {
    printf("garmin unit could not be opened!\n");
  }

  return 0;
}
