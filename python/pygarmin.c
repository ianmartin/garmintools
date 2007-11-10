#include <Python.h>
#include <stdio.h>
#include "garmin.h"


int verbose = 0;


/* Toggle the state of the verbose flag and return the new value */

static PyObject* toggle_verbose(PyObject* obj, PyObject* args)
{
  verbose = (int) ! verbose;
  return PyBool_FromLong(verbose);
}


/* Return the current state of the verbose flag */

static PyObject* get_verbose(PyObject* obj, PyObject* args)
{
  return PyBool_FromLong(verbose);
}


/* Initialize the garmin unit and hanlde errors */

bool initialize_garmin(garmin_unit* garmin)
{
  int init_code = garmin_init(garmin, verbose);

  switch (init_code)
  {
    case 0 :
      PyErr_SetString(PyExc_RuntimeError, "Garmin unit could not be opened.");
      return 0;
    
    default:
      return 1;
  }
}


/* Return information about the attached garmin unit */

static PyObject* get_info(PyObject* obj, PyObject* args)
{
  garmin_unit garmin;

  if (!initialize_garmin(&garmin))
      return NULL;
      
  PyObject* dict = PyDict_New();
    
  uint32 unit_id = garmin.id;
  PyDict_SetItem(dict, PyString_FromString("unit_id"), PyLong_FromUnsignedLong(unit_id));

  uint16 product_id = garmin.product.product_id;
  PyDict_SetItem(dict, PyString_FromString("product_id"), PyInt_FromLong(product_id));

  double software_version = garmin.product.software_version / 100.0;
  PyDict_SetItem(dict, PyString_FromString("software_version"), PyFloat_FromDouble(software_version));
    
  char* product_description = garmin.product.product_description; 
  PyDict_SetItem(dict, PyString_FromString("description"), PyString_FromString(product_description));

  return Py_BuildValue("N", dict);
}


/* Return all run data from the attached garmin unit as python dictionary */

static PyObject* get_runs(PyObject* obj, PyObject* args)
{
  garmin_unit garmin;

  if (!initialize_garmin(&garmin))
    return NULL;
  
  garmin_data * data;

  if ( (data = garmin_get(&garmin, GET_RUNS)) == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "Unable to extract any data.");
    return NULL;
  }

  /*
    We should have a list with three elements:
    1) The runs (which identify the track and lap indices)
    2) The laps (which are related to the runs)
    3) The tracks (which are related to the runs)
  */

  garmin_data * tmpdata;
  garmin_list * runs    = NULL;
  garmin_list * laps    = NULL;
  garmin_list * tracks  = NULL;

  tmpdata = garmin_list_data(data, 0);
  if ( tmpdata == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "Toplevel data missing element 0 (runs)");
    return NULL;
  }

  runs = tmpdata->data;
  if ( runs == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "No runs extracted.");
    return NULL;
  }
    
  tmpdata = garmin_list_data(data, 1);
  if ( tmpdata == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "Toplevel data missing element 1 (laps)");
    return NULL;
  }

  laps = tmpdata->data;
  if ( laps == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "No laps extracted.");
    return NULL;
  }
  
  tmpdata = garmin_list_data(data, 2);
  if ( tmpdata == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "Toplevel data missing element 2 (tracks)");
    return NULL;
  }
  
  tracks = tmpdata->data;
  if ( tracks == NULL )
  {
    PyErr_SetString(PyExc_RuntimeError, "No tracks extracted.");
    return NULL;
  }

  garmin_list_node * n;
  garmin_list_node * m;
  garmin_list_node * o;
  uint32             trk;
  uint32             f_lap;
  uint32             l_lap;
  uint32             l_idx;
  time_type          start;

  /* Print some debug output if requested. */
  if ( verbose != 0 )
  {
    for ( m = laps->head; m != NULL; m = m->next )
    {
      if ( get_lap_index(m->data,&l_idx) != 0 )
        printf("[garmin] lap: index [%d]\n", l_idx);
      else
        printf("[garmin] lap: index [??]\n");
    }
  }
    
  /* For each run, get its laps and track points. */
  
  PyObject* dict = PyDict_New();

  for ( n = runs->head; n != NULL; n = n->next )
  {      
    if ( get_run_track_lap_info(n->data, &trk, &f_lap, &l_lap) != 0 )
    {
      time_type f_lap_start = 0;
      
      PyObject* run = PyDict_New();
      PyObject* rlaps = PyDict_New();

      PyDict_SetItem(run, PyString_FromString("track"), Py_BuildValue("i", trk));
      PyDict_SetItem(run, PyString_FromString("first_lap"), Py_BuildValue("i", f_lap));
      PyDict_SetItem(run, PyString_FromString("last_lap"), Py_BuildValue("i", l_lap));
      PyDict_SetItem(run, PyString_FromString("type"), Py_BuildValue("i", (int)n->data->type));

      /* TODO:
         Implement something similar for the other run types, D1000 and D1010
         See src/run.c get_run_track_lap_info() for more information
       */
      
      if (n->data->type == data_D1009)
      {
        D1009 * d1009;
        d1009 = n->data->data;
        PyDict_SetItem(run, PyString_FromString("multisport"), PyBool_FromLong(d1009->multisport));
        switch (d1009->sport_type)
        {
          case D1000_running:
            PyDict_SetItem(run, PyString_FromString("sport"), PyString_FromString("running"));
            break;
          case D1000_biking:
            PyDict_SetItem(run, PyString_FromString("sport"), PyString_FromString("biking"));
            break;
          case D1000_other:
            PyDict_SetItem(run, PyString_FromString("sport"), PyString_FromString("other"));
            break;            
        }
      }

      if (verbose != 0)
        printf("[garmin] run: track [%d], laps [%d:%d]\n",trk,f_lap,l_lap);
      
      for ( m = laps->head; m != NULL; m = m->next )
      {
        if ( get_lap_index(m->data, &l_idx) != 0 )
        {
          if ( l_idx >= f_lap && l_idx <= l_lap )
          {
            PyObject* lap = PyDict_New();
            
            if (verbose != 0)
              printf("[garmin] lap [%d] falls within laps [%d:%d]\n", l_idx,f_lap,l_lap);
            
            start = 0;
            get_lap_start_time(m->data, &start);

            if (start != 0)
            {
              if (l_idx == f_lap)
                  f_lap_start = start;

              PyDict_SetItem(lap, PyString_FromString("start_time"), Py_BuildValue("i", (int)start));
              PyDict_SetItem(lap, PyString_FromString("type"), Py_BuildValue("i", (int)m->data->type));

              if (m->data->type == data_D1015)
              {
                D1015 * d1015;
                d1015 = m->data->data;
                PyDict_SetItem(lap, PyString_FromString("duration"), Py_BuildValue("i", d1015->total_time));
                PyDict_SetItem(lap, PyString_FromString("distance"), Py_BuildValue("f", d1015->total_dist));
                PyDict_SetItem(lap, PyString_FromString("max_speed"), Py_BuildValue("f", d1015->max_speed));
              }

              PyObject * points = PyList_New(0);
              
              bool have_track = 0;
              bool done = 0;
                
              for ( o = tracks->head; o != NULL; o = o->next )
              {
                if ( o->data != NULL )
                {
                  if (o->data->type == data_D311)
                  {
                    if ( ! have_track )
                    {
                      D311 * d311;
                      d311 = o->data->data;
                      if (  d311->index == trk )
                          have_track = 1;
                    }
                    else /* We've reached the end of the track */
                        done = 1;
                  }
                  
                  else if (o->data->type == data_D304 && have_track)
                  {
                    D304 * d304;
                    d304 = o->data->data;
                    PyObject* point = PyDict_New();
                    
                    if (d304->posn.lat != 2147483647 && d304->posn.lon != 2147483647)
                    {
                      PyDict_SetItem(point, PyString_FromString("position"), Py_BuildValue("(ff)", SEMI2DEG(d304->posn.lat), SEMI2DEG(d304->posn.lon)));
                      PyDict_SetItem(point, PyString_FromString("type"), Py_BuildValue("i", (int)o->data->type));
                      PyDict_SetItem(point, PyString_FromString("time"), Py_BuildValue("f", (float)(d304->time + TIME_OFFSET)));
                      PyDict_SetItem(point, PyString_FromString("distance"), Py_BuildValue("f", d304->distance));
                      PyDict_SetItem(point, PyString_FromString("altitude"), Py_BuildValue("f", d304->alt));
                      PyDict_SetItem(point, PyString_FromString("heart_rate"), Py_BuildValue("i", d304->heart_rate));
                      
                      if (d304->cadence != 255)
                          PyDict_SetItem(point, PyString_FromString("cadence"), Py_BuildValue("i", d304->cadence));
                      
                      PyList_Append(points, Py_BuildValue("N", point));
                    }
                  }
                  
                  else if (have_track)
                      printf("get_track: point type %d invalid!\n",o->data->type);
                  }

                if ( done ) break;
              }
              PyDict_SetItem(lap, PyString_FromString("points"), Py_BuildValue("N", points));
            }
            else
                PyErr_Warn(PyExc_Warning, "Start time of first lap not found.");
            
            PyDict_SetItem(rlaps, PyString_FromFormat("%d", (int)l_idx), Py_BuildValue("N", lap));
          }
        }  
      }
      PyDict_SetItem(run, PyString_FromString("laps"), Py_BuildValue("N", rlaps));
      PyDict_SetItem(dict, PyString_FromFormat("%d", (int)f_lap_start), Py_BuildValue("N", run));
    }
  }

  garmin_free_data(data);

  return Py_BuildValue("N", dict);  
}


/* Assign python names to the exported functions */

static PyMethodDef MethodTable[] = {
  {"toggle_verbose", toggle_verbose, METH_VARARGS, "Toggle verbose flag and return its current state, True if turned on, False else."},
  {"get_verbose", get_verbose, METH_VARARGS, "Return the current state of the verbose flag, True if turned on, False else."},
  {"get_info", get_info, METH_VARARGS, "Return a dictionary with device information."},
  {"get_runs", get_runs, METH_VARARGS, "Return a dictionary with all runs on the device."},
  {NULL, NULL, 0, NULL}
};


/* Initialize pygarmin as python extension */

PyMODINIT_FUNC initpygarmin(void)
{
  PyObject* module = 0;
  module = Py_InitModule3("pygarmin", MethodTable, "Python bindings for garmintools");

  if (module == 0)
    PyErr_SetString(PyExc_RuntimeError, "Failure on Py_InitModule3.");
}
