/*
 *
 * Bacula interface to Python for the Storage Daemon
 *
 * Kern Sibbald, January MMV
 *
 *   Version $Id: pythonsd.c,v 1.3 2005/08/10 16:35:37 nboichat Exp $
 *
 */

/*
   Copyright (C) 2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "stored.h"

#ifdef HAVE_PYTHON
#undef _POSIX_C_SOURCE
#include <Python.h>

extern JCR *get_jcr_from_PyObject(PyObject *self);
extern PyObject *find_method(PyObject *eventsObject, PyObject *method, 
         const char *name);

static PyObject *set_job_events(PyObject *self, PyObject *arg);
static PyObject *job_write(PyObject *self, PyObject *arg);

PyMethodDef JobMethods[] = {
    {"set_events", set_job_events, METH_VARARGS, "Set Job events"},
    {"write", job_write, METH_VARARGS, "Write to output"},
    {NULL, NULL, 0, NULL}             /* last item */
};


struct s_vars {
   const char *name;
   char *fmt;
};

static struct s_vars getvars[] = {
   { N_("Job"),        "s"},          /* 0 */
   { N_("SDName"),     "s"},          /* 1 */
   { N_("Level"),      "s"},          /* 2 */
   { N_("Type"),       "s"},          /* 3 */
   { N_("JobId"),      "i"},          /* 4 */
   { N_("Client"),     "s"},          /* 5 */
   { N_("Pool"),       "s"},          /* 6 */
   { N_("MediaType"),  "s"},          /* 7 */
   { N_("JobName"),    "s"},          /* 8 */
   { N_("JobStatus"),  "s"},          /* 9 */
   { N_("VolumeName"), "s"},          /* 10 */
   { N_("Device"),     "s"},          /* 11 */

   { NULL,             NULL}
};


/* Writable variables */
static struct s_vars setvars[] = {
   { N_("JobReport"),   "s"},

   { NULL,             NULL}
};


/* Return Job variables */
PyObject *job_getattr(PyObject *self, char *attrname)
{
   JCR *jcr;
   bool found = false;
   int i;
   char buf[10];
   char errmsg[200];
   
   Dmsg1(100, "In job_getattr=%s\n", attrname);
   jcr = get_jcr_from_PyObject(self);
   if (!jcr) {
      bstrncpy(errmsg, _("Job pointer not found."), sizeof(errmsg));
      goto bail_out;
   }

   for (i=0; getvars[i].name; i++) {
      if (strcmp(getvars[i].name, attrname) == 0) {
         found = true;
         break;
      }
   }
   if (!found) {
      /* Try our methods */
      return Py_FindMethod(JobMethods, self, attrname);
   }  
   switch (i) {
   case 0:                            /* Job */
      return Py_BuildValue(getvars[i].fmt, jcr->job_name);    /* Non-unique name */
   case 1:                            /* SD's name */
      return Py_BuildValue(getvars[i].fmt, my_name);
   case 2:                            /* level */
      return Py_BuildValue(getvars[i].fmt, job_level_to_str(jcr->JobLevel));
   case 3:                            /* type */
      return Py_BuildValue(getvars[i].fmt, job_type_to_str(jcr->JobType));
   case 4:                            /* JobId */
      return Py_BuildValue(getvars[i].fmt, jcr->JobId);
   case 5:                            /* Client */
      return Py_BuildValue(getvars[i].fmt, jcr->client_name);
   case 6:                            /* Pool */
      return Py_BuildValue(getvars[i].fmt, jcr->dcr->pool_name);
   case 7:                            /* MediaType */
      return Py_BuildValue(getvars[i].fmt, jcr->dcr->media_type);
   case 8:                            /* JobName */
      return Py_BuildValue(getvars[i].fmt, jcr->Job);
   case 9:                            /* JobStatus */
      buf[1] = 0;
      buf[0] = jcr->JobStatus;
      return Py_BuildValue(getvars[i].fmt, buf);
   case 10:
      return Py_BuildValue(getvars[i].fmt, jcr->dcr->VolumeName);
   case 11:
      return Py_BuildValue(getvars[i].fmt, jcr->dcr->dev_name);
   }
   bsnprintf(errmsg, sizeof(errmsg), _("Attribute %s not found."), attrname);
bail_out:
   PyErr_SetString(PyExc_AttributeError, errmsg);
   return NULL;
}

int job_setattr(PyObject *self, char *attrname, PyObject *value)
{
  JCR *jcr;
   bool found = false;
   char *strval = NULL;
   char buf[200];
   char *errmsg;
   int i;

   Dmsg2(100, "In job_setattr=%s val=%p.\n", attrname, value);
   if (value == NULL) {                /* Cannot delete variables */
      bsnprintf(buf, sizeof(buf), _("Cannot delete attribute %s"), attrname);
      errmsg = buf;
      goto bail_out;
   }
   jcr = get_jcr_from_PyObject(self);
   if (!jcr) {
      errmsg = _("Job pointer not found.");
      goto bail_out;
   }

   /* Find attribute name in list */
   for (i=0; setvars[i].name; i++) {
      if (strcmp(setvars[i].name, attrname) == 0) {
         found = true;
         break;
      }
   }
   if (!found) {
      goto not_found;
   }
   /* Get argument value ***FIXME*** handle other formats */
   if (setvars[i].fmt != NULL) {
      if (!PyArg_Parse(value, setvars[i].fmt, &strval)) {
         PyErr_SetString(PyExc_TypeError, _("Read-only attribute"));
         return -1;
      }
   }   
   switch (i) {
   case 0:                            /* JobReport */
      Jmsg(jcr, M_INFO, 0, "%s", strval);
      return 0;
   }
not_found:
   bsnprintf(buf, sizeof(buf), _("Cannot find attribute %s"), attrname);
   errmsg = buf;
bail_out:
   PyErr_SetString(PyExc_AttributeError, errmsg);
   return -1;
}


static PyObject *set_job_events(PyObject *self, PyObject *arg)
{
   PyObject *eObject;
   JCR *jcr;

   Dmsg0(100, "In set_job_events.\n");
   if (!PyArg_ParseTuple(arg, "O:set_events", &eObject)) {
      Dmsg0(000, _("Error in ParseTuple\n"));
      return NULL;
   }
   jcr = get_jcr_from_PyObject(self);
   Py_XDECREF((PyObject *)jcr->Python_events);
   Py_INCREF(eObject);
   jcr->Python_events = (void *)eObject;
   Py_INCREF(Py_None);
   return Py_None;
}

static PyObject *job_write(PyObject *self, PyObject *args)
{
   char *text = NULL;

   if (!PyArg_ParseTuple(args, "s:write", &text)) {
      Dmsg0(000, _("Parse tuple error in job_write\n"));
      return NULL;
   }
   if (text) {
      Jmsg(NULL, M_INFO, 0, "%s", text);
   }
   Py_INCREF(Py_None);
   return Py_None;
}


int generate_job_event(JCR *jcr, const char *event)
{
   PyObject *method = NULL;
   PyObject *Job = (PyObject *)jcr->Python_job;
   PyObject *events = (PyObject *)jcr->Python_events;
   PyObject *result = NULL;
   int stat = 0;

   if (!Job || !events) {
      return 0;
   }

   PyEval_AcquireLock();

   method = find_method(events, method, event);
   if (!method) {
      goto bail_out;
   }

   bstrncpy(jcr->event, event, sizeof(jcr->event));
   result = PyObject_CallFunction(method, "O", Job);
   jcr->event[0] = 0;             /* no event in progress */
   if (result == NULL) {
      if (PyErr_Occurred()) {
         PyErr_Print();
         Dmsg1(000, _("Error in Python method %s\n"), event);
      }
   } else {
      stat = 1;
   }
   Py_XDECREF(result);

bail_out:
   PyEval_ReleaseLock();
   return stat;
}


#else

/* Dummy if Python not configured */
int generate_job_event(JCR *jcr, const char *event)
{ return 1; }


#endif /* HAVE_PYTHON */
