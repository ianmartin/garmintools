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
#include <time.h>
#include "garmin.h"


#define BBOX_NW  0
#define BBOX_NE  1
#define BBOX_SE  2
#define BBOX_SW  3


typedef struct
{
  float lat, lon, elev;
  time_t t;
} route_point;

int
get_gpx_data ( garmin_data *    data,
               route_point   ** points,
               position_type *  sw,
               position_type *  ne )
{
  garmin_list *       dlist;
  garmin_list_node *  node;
  garmin_data *       point;
  D304 *              d304;
  route_point *       rp;
  float               minlat =   90.0;
  float               maxlat =  -90.0;
  float               minlon =  180.0;
  float               maxlon = -180.0;
  int                 ok     = 0;

  if ( data != NULL ) {
    data = garmin_list_data(data,2);

    if ( data == NULL ) {

      printf("get_gpx_data: no track points found\n");

    } else if ( data->type == data_Dlist ) {

      dlist = data->data;

      *points = malloc(sizeof(route_point) * (dlist->elements+1));
      rp = *points;

      for ( node = dlist->head; node != NULL; node = node->next ) {
       point = node->data;
       if ( point->type == data_D304 ) {

         d304 = point->data;

         if ( d304->posn.lat == 0x7fffffff && d304->posn.lon == 0x7fffffff )
           continue;

         rp->lat = SEMI2DEG(d304->posn.lat);
         rp->lon = SEMI2DEG(d304->posn.lon);
          rp->elev = d304->alt;
          rp->t = d304->time;

         if ( rp->lat < minlat ) minlat = rp->lat;
         if ( rp->lat > maxlat ) maxlat = rp->lat;
         if ( rp->lon < minlon ) minlon = rp->lon;
         if ( rp->lon > maxlon ) maxlon = rp->lon;

          ++rp;
        }
      }
      rp->t = 0;

      /* Now we can fill in the center coordinate and bounding box. */

      ne->lat = DEG2SEMI(maxlat);
      ne->lon = DEG2SEMI(maxlon);

      sw->lat = DEG2SEMI(minlat);
      sw->lon = DEG2SEMI(minlon);

      ok = 1;
    } else {
      printf("get_gpx_data: invalid track point list\n");
    }
  } else {
    printf("get_gpx_data: NULL data pointer\n");
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
print_gpx_header ( FILE *               fp,
                   int                  spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(fp,"<gpx version=\"1.0\"\n"
    "creator=\"Garmin Forerunner Tools - http://garmintools.googlecode.com\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
    "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
}

static void
print_time_tag ( const time_t           t,
                 FILE *                 fp,
                 int                    spaces )
{
  char buf[512];
  struct tm *tmp;

  tmp = gmtime(&t);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tmp);
  print_string_tag("time",buf,fp,spaces);
}

static void
print_bounds_tag ( const position_type * sw,
                   const position_type * ne,
                   FILE *                fp,
                   int                   spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<bounds minlat=\"%f\" minlon=\"%f\" maxlat=\"%f\" maxlon=\"%f\" />\n",
      SEMI2DEG(sw->lat), SEMI2DEG(sw->lon),
      SEMI2DEG(ne->lat), SEMI2DEG(ne->lon));
}

static void
print_route_points ( route_point * points,
                     FILE *        fp,
                     int           spaces )
{
  route_point * rp = points;
  while (rp->t > 0)
  {
    print_spaces(fp, spaces);
    fprintf(fp, "<rtept lat=\"%f\" lon=\"%f\">\n", rp->lat, rp->lon);
    print_spaces(fp, spaces+2);
    fprintf(fp, "<ele>%f</ele>\n", rp->elev);
    print_time_tag(rp->t + TIME_OFFSET, fp, spaces+2);
    print_close_tag("rtept", fp, spaces);
    ++rp;
  }
}

void
print_gpx_data ( garmin_data * data, FILE * fp, int spaces )
{
  route_point *         points = NULL;
  position_type  sw;
  position_type  ne;

  if ( get_gpx_data(data,&points,&sw,&ne) != 0 ) {
    print_gpx_header(fp,spaces);
    print_time_tag(time(NULL),fp,spaces);
    print_bounds_tag(&sw,&ne,fp,spaces);
    print_open_tag("rte",fp,spaces);
    print_route_points(points,fp,spaces+2);
    print_close_tag("rte",fp,spaces);
    print_close_tag("gpx",fp,spaces);

    if ( points != NULL ) free(points);
  }
}


int
main ( int argc, char ** argv )
{
  garmin_data * data;
  int           i;

  for ( i = 1; i < argc; i++ ) {
    if ( (data = garmin_load(argv[i])) != NULL ) {
      print_gpx_data(data,stdout,0);
      garmin_free_data(data);
    }
  }

  return 0;
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
#include <time.h>
#include "garmin.h"


#define BBOX_NW  0
#define BBOX_NE  1
#define BBOX_SE  2
#define BBOX_SW  3


typedef struct
{
  float lat, lon, elev;
  time_t t;
} route_point;

int
get_gpx_data ( garmin_data *    data,
               route_point   ** points,
               position_type *  sw,
               position_type *  ne )
{
  garmin_list *       dlist;
  garmin_list_node *  node;
  garmin_data *       point;
  D304 *              d304;
  route_point *       rp;
  float               minlat =   90.0;
  float               maxlat =  -90.0;
  float               minlon =  180.0;
  float               maxlon = -180.0;
  int                 ok     = 0;

  if ( data != NULL ) {
    data = garmin_list_data(data,2);

    if ( data == NULL ) {

      printf("get_gpx_data: no track points found\n");

    } else if ( data->type == data_Dlist ) {

      dlist = data->data;

      *points = malloc(sizeof(route_point) * (dlist->elements+1));
      rp = *points;

      for ( node = dlist->head; node != NULL; node = node->next ) {
       point = node->data;
       if ( point->type == data_D304 ) {

         d304 = point->data;

         if ( d304->posn.lat == 0x7fffffff && d304->posn.lon == 0x7fffffff )
           continue;

         rp->lat = SEMI2DEG(d304->posn.lat);
         rp->lon = SEMI2DEG(d304->posn.lon);
          rp->elev = d304->alt;
          rp->t = d304->time;

         if ( rp->lat < minlat ) minlat = rp->lat;
         if ( rp->lat > maxlat ) maxlat = rp->lat;
         if ( rp->lon < minlon ) minlon = rp->lon;
         if ( rp->lon > maxlon ) maxlon = rp->lon;

          ++rp;
        }
      }
      rp->t = 0;

      /* Now we can fill in the center coordinate and bounding box. */

      ne->lat = DEG2SEMI(maxlat);
      ne->lon = DEG2SEMI(maxlon);

      sw->lat = DEG2SEMI(minlat);
      sw->lon = DEG2SEMI(minlon);

      ok = 1;
    } else {
      printf("get_gpx_data: invalid track point list\n");
    }
  } else {
    printf("get_gpx_data: NULL data pointer\n");
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
print_gpx_header ( FILE *               fp,
                   int                  spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(fp,"<gpx version=\"1.0\"\n"
    "creator=\"Garmin Forerunner Tools - http://garmintools.googlecode.com\"\n"
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
    "xmlns=\"http://www.topografix.com/GPX/1/0\"\n"
    "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");
}

static void
print_time_tag ( const time_t           t,
                 FILE *                 fp,
                 int                    spaces )
{
  char buf[512];
  struct tm *tmp;

  tmp = gmtime(&t);
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tmp);
  print_string_tag("time",buf,fp,spaces);
}

static void
print_bounds_tag ( const position_type * sw,
                   const position_type * ne,
                   FILE *                fp,
                   int                   spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<bounds minlat=\"%f\" minlon=\"%f\" maxlat=\"%f\" maxlon=\"%f\" />\n",
      SEMI2DEG(sw->lat), SEMI2DEG(sw->lon),
      SEMI2DEG(ne->lat), SEMI2DEG(ne->lon));
}

static void
print_route_points ( route_point * points,
                     FILE *        fp,
                     int           spaces )
{
  route_point * rp = points;
  while (rp->t > 0)
  {
    print_spaces(fp, spaces);
    fprintf(fp, "<rtept lat=\"%f\" lon=\"%f\">\n", rp->lat, rp->lon);
    print_spaces(fp, spaces+2);
    fprintf(fp, "<ele>%f</ele>\n", rp->elev);
    print_time_tag(rp->t + TIME_OFFSET, fp, spaces+2);
    print_close_tag("rtept", fp, spaces);
    ++rp;
  }
}

void
print_gpx_data ( garmin_data * data, FILE * fp, int spaces )
{
  route_point *         points = NULL;
  position_type  sw;
  position_type  ne;

  if ( get_gpx_data(data,&points,&sw,&ne) != 0 ) {
    print_gpx_header(fp,spaces);
    print_time_tag(time(NULL),fp,spaces);
    print_bounds_tag(&sw,&ne,fp,spaces);
    print_open_tag("rte",fp,spaces);
    print_route_points(points,fp,spaces+2);
    print_close_tag("rte",fp,spaces);
    print_close_tag("gpx",fp,spaces);

    if ( points != NULL ) free(points);
  }
}


int
main ( int argc, char ** argv )
{
  garmin_data * data;
  int           i;

  for ( i = 1; i < argc; i++ ) {
    if ( (data = garmin_load(argv[i])) != NULL ) {
      print_gpx_data(data,stdout,0);
      garmin_free_data(data);
    }
  }

  return 0;
