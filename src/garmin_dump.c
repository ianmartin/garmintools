#include <stdio.h>
#include "garmin.h"


int
main ( int argc, char ** argv )
{
  garmin_data * data;
  int           i;

  for ( i = 1; i < argc; i++ ) {    
    if ( (data = garmin_load(argv[i])) != NULL ) {
      garmin_print_data(data,stdout,0);
      garmin_free_data(data);
    }
  }

  return 0;
}
