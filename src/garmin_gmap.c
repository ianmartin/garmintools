/*
  Garmintools software package
  Copyright (C) 2006-2008 Dave Bailey
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <math.h>
#include <stdio.h>
#include "garmin.h"


#define BBOX_NW  0
#define BBOX_NE  1
#define BBOX_SE  2
#define BBOX_SW  3


static unsigned long M[32] = {
  0x00000001, 0x00000003, 0x00000007, 0x0000000f,
  0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
  0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
  0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
  0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
  0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
  0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
  0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};


#define FIVEBITCHUNKS(v) \
  (v<0x20)?1:(v<0x400)?2:(v<0x8000)?3:(v<0x100000)?4:(v<0x2000000)?5:6



int
get_gmap_data ( garmin_data *    data,
		char **          points,
		char **          levels,
		position_type *  center,
		position_type *  start,
		position_type *  sw,
		position_type *  ne )
{
  garmin_list *       dlist;
  garmin_list_node *  node;
  garmin_data *       point;
  char *              pp;
  char *              lp;
  D304 *              d304;
  int                 ilat5;
  int                 ilon5;
  int                 llat5;
  int                 llon5;
  int                 dlat5;
  int                 dlon5;
  float               lat;
  float               lon;
  float               minlat =   90.0;
  float               maxlat =  -90.0;
  float               minlon =  180.0;
  float               maxlon = -180.0;
  int                 ok     = 0;
  int                 i;
  int                 j;
  int                 x;

  if ( data != NULL ) {
    data = garmin_list_data(data,2);

    if ( data == NULL ) {

      printf("get_gmap_data: no track points found\n");

    } else if ( data->type == data_Dlist ) {

      dlist = data->data;
      
      *points = malloc(12 * dlist->elements);
      *levels = malloc(2 * dlist->elements);

      pp = *points;
      lp = *levels;

      j = 0;
      llat5 = 0;
      llon5 = 0;
      
      for ( node = dlist->head; node != NULL; node = node->next ) {
	point = node->data;
	if ( point->type == data_D304 ) {

	  d304 = point->data;

	  if ( d304->posn.lat == 0x7fffffff && d304->posn.lon == 0x7fffffff )
	    continue;

	  lat = SEMI2DEG(d304->posn.lat);
	  lon = SEMI2DEG(d304->posn.lon);

	  if ( j == 0 ) {	    
	    start->lat = d304->posn.lat;
	    start->lon = d304->posn.lon;
	  }

	  if ( lat < minlat ) minlat = lat;
	  if ( lat > maxlat ) maxlat = lat;
	  if ( lon < minlon ) minlon = lon;
	  if ( lon > maxlon ) maxlon = lon;

	  ilat5 = floor(lat * 1.0e5);
	  ilon5 = floor(lon * 1.0e5);
	  dlat5 = (abs(ilat5-llat5)<<1)-(ilat5<llat5);
	  dlon5 = (abs(ilon5-llon5)<<1)-(ilon5<llon5);
	  
	  if ( dlat5 || dlon5 ) {

	    /* Encode the point. */

	    for (x = dlat5, i = FIVEBITCHUNKS(x); i > 0; x >>= 5, i--, pp++)
	      if ((*pp = ((x&0x1f)|((i>1)?0x20:0))+0x3f) == '\\') *++pp = '\\';
	    for (x = dlon5, i = FIVEBITCHUNKS(x); i > 0; x >>= 5, i--, pp++)
	      if ((*pp = ((x&0x1f)|((i>1)?0x20:0))+0x3f) == '\\') *++pp = '\\';
	    
	    /* Compute the zoom level at which to show this point. */
	    
	    for (i = 0, *lp = 0x40; i<32 && (j&M[i])==M[i]; i++, (*lp)++);
	    lp++;
	    
	    *pp = 0;
	    *lp = 0;

	    j++;
	  }
	  llat5 = ilat5;
	  llon5 = ilon5;
	}
      }

      **levels = *(lp-1) = 'P';

      /* Now we can fill in the center coordinate and bounding box. */

      center->lat = DEG2SEMI((minlat+maxlat)/2.0);
      center->lon = DEG2SEMI((minlon+maxlon)/2.0);

      ne->lat = DEG2SEMI(maxlat);
      ne->lon = DEG2SEMI(maxlon);

      sw->lat = DEG2SEMI(minlat);
      sw->lon = DEG2SEMI(minlon);

      ok = 1;
    } else {
      printf("get_gmap_data: invalid track point list\n");
    }
  } else {
    printf("get_gmap_data: NULL data pointer\n");
  }

  return ok;
}


static void
print_spaces ( FILE * fp, int spaces )
{
  int i;

  for ( i = 0; i < spaces; i++ ) {
    fprintf(fp," ");
  }
}


static void
print_open_tag ( const char * tag, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<%s>\n",tag);
}


static void
print_close_tag ( const char * tag, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"</%s>\n",tag);
}


static void
print_string_tag ( const char * tag, const char * val, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<%s>%s</%s>\n",tag,val,tag);
}


static void
print_position ( const char *           tag, 
		 const position_type *  p, 
		 FILE *                 fp, 
		 int                    spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<%s lat=\"%f\" lon=\"%f\"/>\n",
	  tag,SEMI2DEG(p->lat),SEMI2DEG(p->lon));
}


void
print_gmap_data ( garmin_data * data, FILE * fp, int spaces )
{
  char *         points = NULL;
  char *         levels = NULL;
  position_type  center;
  position_type  start;
  position_type  sw;
  position_type  ne;

  if ( get_gmap_data(data,&points,&levels,&center,&start,&sw,&ne) != 0 ) {

    print_open_tag("gmap_data",fp,spaces);
    print_open_tag("coordinates",fp,spaces+1);
    print_position("start",&start,fp,spaces+2);
    print_position("center",&center,fp,spaces+2);
    print_position("southwest",&sw,fp,spaces+2);
    print_position("northeast",&ne,fp,spaces+2);
    print_close_tag("coordinates",fp,spaces+1);
    print_open_tag("polyline",fp,spaces+1);
    print_string_tag("points",points,fp,spaces+2);
    print_string_tag("levels",levels,fp,spaces+2);
    print_close_tag("polyline",fp,spaces+1);
    print_close_tag("gmap_data",fp,spaces);

    if ( points != NULL ) free(points);
    if ( levels != NULL ) free(levels);
  }
}


int
main ( int argc, char ** argv )
{
  garmin_data * data;
  int           i;

  for ( i = 1; i < argc; i++ ) {    
    if ( (data = garmin_load(argv[i])) != NULL ) {
      print_gmap_data(data,stdout,0);
      garmin_free_data(data);
    }
  }

  return 0;
}
