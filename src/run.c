#include <time.h>
#include <string.h>
#include <errno.h>
#include "garmin.h"


int
get_run_track_lap_info ( garmin_data * run,
			 uint32 *      track_index,
			 uint32 *      first_lap_index,
			 uint32 *      last_lap_index )
{
  D1000 * d1000;
  D1009 * d1009;
  D1010 * d1010;

  int ok = 1;

  switch ( run->type ) {
  case data_D1000:
    d1000            = run->data;
    *track_index     = d1000->track_index;
    *first_lap_index = d1000->first_lap_index;
    *last_lap_index  = d1000->last_lap_index;
    break;
  case data_D1009:
    d1009            = run->data;
    *track_index     = d1009->track_index;
    *first_lap_index = d1009->first_lap_index;
    *last_lap_index  = d1009->last_lap_index;
    break;
  case data_D1010:
    d1010            = run->data;
    *track_index     = d1010->track_index;
    *first_lap_index = d1010->first_lap_index;
    *last_lap_index  = d1010->last_lap_index;
    break;
  default:
    ok = 0;
    break;
  }

  return ok;
}


int
get_lap_index ( garmin_data * lap, uint32 * lap_index )
{
  D1001 * d1001;
  D1011 * d1011;

  int ok = 1;

  switch ( lap->type ) {
  case data_D1001:
    d1001      = lap->data;
    *lap_index = d1001->index;
    break;
  case data_D1011:
    d1011      = lap->data;
    *lap_index = d1011->index;
    break;
  default:
    ok = 0;
    break;
  }

  return ok;
}


int
get_lap_start_time ( garmin_data * lap, time_type * start_time )
{
  D1001 * d1001;
  D1011 * d1011;

  int ok = 1;

  switch ( lap->type ) {
  case data_D1001:
    d1001       = lap->data;
    *start_time = d1001->start_time + TIME_OFFSET;
    break;
  case data_D1011:
    d1011       = lap->data;
    *start_time = d1011->start_time + TIME_OFFSET;
    break;
  default:
    ok = 0;
    break;
  }

  return ok;
}


garmin_data *
get_track ( garmin_list * points, uint32 trk_index )
{
  garmin_list_node * n;
  garmin_data *      track = NULL;
  D311 *             d311;
  int                done = 0;

  /* Look for a data_D311 with an index that matches. */

  for ( n = points->head; n != NULL; n = n->next ) {    
    if ( n->data != NULL ) {
      switch ( n->data->type ) {
      case data_D311:
	if ( track == NULL ) {
	  d311 = n->data->data;
	  if ( d311->index == trk_index ) {
	    track = garmin_alloc_data(data_Dlist);
	    garmin_list_append(track->data,n->data);
	  }
	} else {
	  /* We've reached the end of the track */
	  done = 1;
	}
	break;
      case data_D300:
      case data_D301:
      case data_D302:
      case data_D303:
      case data_D304:
	if ( track != NULL ) {
	  garmin_list_append(track->data,n->data);
	}
	break;
      default:
	break;
      }
    }

    if ( done != 0 ) break;
  }

  return track;
}


void
garmin_save_runs ( garmin_unit * garmin )
{
  garmin_data *       data;
  garmin_data *       data0;
  garmin_data *       data1;
  garmin_data *       data2;
  garmin_data *       rlaps;
  garmin_data *       rtracks;
  garmin_list *       runs;
  garmin_list *       laps;
  garmin_list *       tracks;
  garmin_data *       rlist;
  garmin_list_node *  n;
  garmin_list_node *  m;
  uint32              trk;
  uint32              f_lap;
  uint32              l_lap;
  uint32              l_idx;
  time_type           start;
  time_t              start_time;
  char                filename[BUFSIZ];
  char *              filedir = NULL;
  char                path[PATH_MAX];
  char                filepath[BUFSIZ];
  struct tm *         tbuf;

  if ( (filedir = getenv("GARMIN_SAVE_RUNS")) != NULL ) {
    filedir = realpath(filedir,path);
    if ( filedir == NULL ) {
      printf("GARMIN_SAVE_RUNS: %s\n",strerror(errno));
    }
  }
  if ( filedir == NULL ) {
    filedir = ".";
  }

  if ( (data = garmin_get(garmin,GET_RUNS)) != NULL ) {

    /* 
       We should have a list with three elements:

       1) The runs (which identify the track and lap indices)
       2) The laps (which are related to the runs)
       3) The tracks (which are related to the runs)
    */

    data0 = garmin_list_data(data,0);
    data1 = garmin_list_data(data,1);
    data2 = garmin_list_data(data,2);

    if ( data0 != NULL && (runs   = data0->data) != NULL &&
	 data1 != NULL && (laps   = data1->data) != NULL &&
	 data2 != NULL && (tracks = data2->data) != NULL ) {
      
      /* For each run, get its laps and track points. */

      for ( n = runs->head; n != NULL; n = n->next ) {
	if ( get_run_track_lap_info(n->data,&trk,&f_lap,&l_lap) != 0 ) {

	  start = 0;

	  /* Get the laps. */

	  rlaps = garmin_alloc_data(data_Dlist);
	  for ( m = laps->head; m != NULL; m = m->next ) {
	    if ( get_lap_index(m->data,&l_idx) != 0 &&
		 l_idx >= f_lap && l_idx <= l_lap ) {	      
	      garmin_list_append(rlaps->data,m->data);
	      if ( l_idx == f_lap ) get_lap_start_time(m->data,&start);
	    }
	  }

	  /* Get the track points. */
	  
	  rtracks = get_track(tracks,trk);

	  /* Now make a three-element list for this run. */

	  rlist = garmin_alloc_data(data_Dlist);
	  garmin_list_append(rlist->data,n->data);
	  garmin_list_append(rlist->data,rlaps);
	  garmin_list_append(rlist->data,rtracks);

	  /* 
	     Determine the filename based on the start time of the first lap. 
	  */

	  if ( (start_time = start) != 0 ) {
	    tbuf = localtime(&start_time);
	    snprintf(filepath,sizeof(filepath)-1,"%s/%d/%02d",
		    filedir,tbuf->tm_year+1900,tbuf->tm_mon+1);
	    strftime(filename,sizeof(filename),"%Y%m%dT%H%M%S.gmn",tbuf);

	    /* Save rlist to the file. */

	    if ( garmin_save(rlist,filename,filepath) != 0 ) {
	      printf("Wrote:   %s/%s\n",filepath,filename);
	    } else {
	      printf("Skipped: %s/%s\n",filepath,filename);
	    }
	  }

	  /* Free the temporary lists we were using. */

	  if ( rlaps != NULL ) {
	    garmin_free_list_only(rlaps->data);
	    free(rlaps);
	  }

	  if ( rtracks != NULL ) {
	    garmin_free_list_only(rtracks->data);
	    free(rtracks);
	  }

	  if ( rlist != NULL ) {
	    garmin_free_list_only(rlist->data);
	    free(rlist);
	  }
	}
      }
    }

    garmin_free_data(data);
  }
}
