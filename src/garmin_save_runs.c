#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include "garmin.h"


int
main ( int argc, char ** argv )
{
  garmin_unit garmin;

  if ( garmin_init(&garmin) != 0 ) {
    /* Set the verbosity if the -v option was provided. */
    garmin.verbose = (getopt(argc,argv,"v") != -1);
    /* Read and save the runs. */
    garmin_save_runs(&garmin);
  } else {
    printf("garmin unit could not be opened!\n");
  }

  return 0;
}
