/*
 *
 * Bacula common code library interface to Python
 *
 * Kern Sibbald, November MMIV
 *
 *   Version $Id: pythonlib.c,v 1.11 2005/08/10 16:35:19 nboichat Exp $
 *
 */
/*
   Copyright (C) 2004-2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "jcr.h"

#ifdef HAVE_PYTHON

#undef _POSIX_C_SOURCE
#include <Python.h>

extern char *configfile;

/* Imported subroutines */
//extern PyMethodDef JobMethods[];
extern PyObject *job_getattr(PyObject *self, char *attrname);
extern int job_setattr(PyObject *self, char *attrname, PyObject *value);

static PyObject *bacula_module = NULL;    /* We create this */
static PyObject *StartUp_module = NULL;   /* We import this */

/* These are the daemon events or methods that are defined */
static PyObject *JobStart_method = NULL;
static PyObject *JobEnd_method = NULL;
static PyObject *Exit_method = NULL;

static PyObject *set_bacula_events(PyObject *self, PyObject *args);
static PyObject *bacula_write(PyObject *self, PyObject *args);

PyObject *find_method(PyObject *eventsObject, PyObject *method, const char *name);

/* Define Bacula daemon method entry points */
static PyMethodDef BaculaMethods[] = {
    {"set_events", set_bacula_events, METH_VARARGS, "Define Bacula events."},
    {"write", bacula_write, METH_VARARGS, "Write output."},
    {NULL, NULL, 0, NULL}             /* last item */
};

static char my_version[] = VERSION " " BDATE;

/*
 * This is a Bacula Job type as defined in Python. We store a pointer
 *   to the jcr. That is all we need, but the user's script may keep
 *   local data attached to this. 
 */
typedef struct s_JobObject {
    PyObject_HEAD
    JCR *jcr;
} JobObject;

static PyTypeObject JobType = {
    PyObject_HEAD_INIT(NULL)
    /* Other items filled in in code below */
};

/* Return the JCR pointer from the JobObject */
JCR *get_jcr_from_PyObject(PyObject *self)
{
   if (!self) {
      return NULL;
   }
   return ((JobObject *)self)->jcr;
}


/* Start the interpreter */
void init_python_interpreter(const char *progname, const char *scripts,
    const char *module)
{
   char buf[MAXSTRING];

   if (!scripts || scripts[0] == 0) {
      Dmsg1(100, "No script dir. prog=%s\n", module);
      return;
   }
   Dmsg2(100, "Script dir=%s prog=%s\n", scripts, module);

   Py_SetProgramName((char *)progname);
   Py_Initialize();
   PyEval_InitThreads();
   bacula_module = Py_InitModule("bacula", BaculaMethods);
   PyModule_AddStringConstant(bacula_module, "Name", my_name);
   PyModule_AddStringConstant(bacula_module, "Version", my_version);
   PyModule_AddStringConstant(bacula_module, "ConfigFile", configfile);
   PyModule_AddStringConstant(bacula_module, "WorkingDir", (char *)working_directory);
   if (!bacula_module) {
      Jmsg0(NULL, M_ERROR_TERM, 0, _("Could not initialize Python\n"));
   }
   bsnprintf(buf, sizeof(buf), "import sys\n"
            "sys.path.append('%s')\n", scripts);
   if (PyRun_SimpleString(buf) != 0) {
      Jmsg1(NULL, M_ERROR_TERM, 0, _("Could not Run Python string %s\n"), buf);
   }   

   /* Explicitly set values we want */
   JobType.tp_name = "Bacula.Job";
   JobType.tp_basicsize = sizeof(JobObject);
   JobType.tp_flags = Py_TPFLAGS_DEFAULT;
   JobType.tp_doc = "Bacula Job object";
   JobType.tp_getattr = job_getattr;
   JobType.tp_setattr = job_setattr;

   if (PyType_Ready(&JobType) != 0) {
      Jmsg0(NULL, M_ERROR_TERM, 0, _("Could not initialize Python Job type.\n"));
      PyErr_Print();
   }   
   StartUp_module = PyImport_ImportModule((char *)module);
   if (!StartUp_module) {
      Emsg2(M_ERROR, 0, _("Could not import Python script %s/%s. Python disabled.\n"),
           scripts, module);
      if (PyErr_Occurred()) {
         PyErr_Print();
         Dmsg0(000, "Python Import error.\n");
      }
   }
   PyEval_ReleaseLock();
}


void term_python_interpreter()
{
   if (StartUp_module) {
      Py_XDECREF(StartUp_module);
      Py_Finalize();
   }
}

static PyObject *set_bacula_events(PyObject *self, PyObject *args)
{
   PyObject *eObject;

   Dmsg0(100, "In set_bacula_events.\n");
   if (!PyArg_ParseTuple(args, "O:set_bacula_events", &eObject)) {
      return NULL;
   }
   JobStart_method = find_method(eObject, JobStart_method, "JobStart");
   JobEnd_method = find_method(eObject, JobEnd_method, "JobEnd");
   Exit_method = find_method(eObject, Exit_method, "Exit");

   Py_XINCREF(eObject);
   Py_INCREF(Py_None);
   return Py_None;
}

/* Write text to daemon output */
static PyObject *bacula_write(PyObject *self, PyObject *args)
{
   char *text;
   if (!PyArg_ParseTuple(args, "s:write", &text)) {
      return NULL;
   }
   if (text) {
      Jmsg(NULL, M_INFO, 0, "%s", text);
   }
   Py_INCREF(Py_None);
   return Py_None;
}


/*
 * Check that a method exists and is callable.
 */
PyObject *find_method(PyObject *eventsObject, PyObject *method, const char *name)
{
   Py_XDECREF(method);             /* release old method if any */
   method = PyObject_GetAttrString(eventsObject, (char *)name);
   if (method == NULL) {
       Dmsg1(000, "Python method %s not found\n", name);
   } else if (PyCallable_Check(method) == 0) {
       Dmsg1(000, "Python object %s found but not a method.\n", name);
       Py_XDECREF(method);
       method = NULL;
   } else {
       Dmsg1(100, "Got method %s\n", name);
   }
   return method; 
}


/*
 * Generate and process a Bacula event by importing a Python
 *  module and running it.
 *
 *  Returns: 0 if Python not configured or module not found
 *          -1 on Python error
 *           1 OK
 */
int generate_daemon_event(JCR *jcr, const char *event)
{
   PyObject *pJob;
   int stat = -1;
   PyObject *result = NULL;

   if (!StartUp_module) {
      Dmsg0(100, "No startup module.\n");
      return 0;
   }

   Dmsg1(100, "event=%s\n", event);
   PyEval_AcquireLock();
   if (strcmp(event, "JobStart") == 0) {
      if (!JobStart_method) {
         stat = 0;
         goto bail_out;
      }
      /* Create JCR argument to send to function */
      pJob = (PyObject *)PyObject_New(JobObject, &JobType);
      if (!pJob) {
         Jmsg(jcr, M_ERROR, 0, _("Could not create Python Job Object.\n"));
         goto bail_out;
      }
      ((JobObject *)pJob)->jcr = jcr;
      bstrncpy(jcr->event, event, sizeof(jcr->event));
      result = PyObject_CallFunction(JobStart_method, "O", pJob);
      jcr->event[0] = 0;             /* no event in progress */
      if (result == NULL) {
         JobStart_method = NULL;
         if (PyErr_Occurred()) {
            PyErr_Print();
            Dmsg0(000, "Python JobStart error.\n");
         }
         Jmsg(jcr, M_ERROR, 0, _("Python function \"%s\" not found.\n"), event);
         Py_XDECREF(pJob);
         goto bail_out;
      }
      jcr->Python_job = (void *)pJob;
      stat = 1;                       /* OK */
      goto jobstart_ok;

   } else if (strcmp(event, "JobEnd") == 0) {
      if (!JobEnd_method || !jcr->Python_job) {
         stat = 0;                    /* probably already here */
         goto bail_out;
      }
      bstrncpy(jcr->event, event, sizeof(jcr->event));
      Dmsg1(100, "Call daemon event=%s\n", event);
      result = PyObject_CallFunction(JobEnd_method, "O", jcr->Python_job);
      jcr->event[0] = 0;             /* no event in progress */
      if (result == NULL) {
         if (PyErr_Occurred()) {
            PyErr_Print();
            Dmsg2(000, "Python JobEnd error. job=%p JobId=%d\n", jcr->Python_job,
               jcr->JobId);
            JobEnd_method = NULL;
         }
         Jmsg(jcr, M_ERROR, 0, _("Python function \"%s\" not found.\n"), event);
         goto bail_out;
      }
      stat = 1;                    /* OK */
   } else if (strcmp(event, "Exit") == 0) {
      if (!Exit_method) {
         stat = 0;
         goto bail_out;
      }
      result = PyObject_CallFunction(Exit_method, NULL);
      if (result == NULL) {
         goto bail_out;
      }
      stat = 1;                    /* OK */
   } else {
      Jmsg1(jcr, M_ABORT, 0, _("Unknown Python daemon event %s\n"), event);
   }

bail_out:
   if (jcr) {
      Py_XDECREF((PyObject *)jcr->Python_job);
      jcr->Python_job = NULL;
      Py_XDECREF((PyObject *)jcr->Python_events);
      jcr->Python_events = NULL;
   }
   /* Fall through */
jobstart_ok:
   Py_XDECREF(result);
   PyEval_ReleaseLock();
   return stat; 
}

#else

/*
 *  No Python configured -- create external entry points and
 *    dummy routines so that library code can call this without
 *    problems even if it is not configured.
 */
int generate_daemon_event(JCR *jcr, const char *event) { return 0; }
void init_python_interpreter(const char *progname, const char *scripts,
         const char *module) { }
void term_python_interpreter() { }

#endif /* HAVE_PYTHON */
