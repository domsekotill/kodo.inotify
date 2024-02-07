/*  Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define PY_SSIZE_T_CLEAN

#include <stddef.h>
#include <sys/inotify.h>

#include <Python.h>

#include "flag.h"
#include "event.h"

// Only needed for Python 3.11, can be removed when dropped
// See https://docs.python.org/3/c-api/structures.html#member-flags
#include "structmember.h"

#if __has_include("docstrings.h")
#include "docstrings.h"
#endif


static char* const EMPTY_NAME = "";


typedef struct {
	PyObject_HEAD
	int       wd;
	PyObject *mask;
	uint32_t  cookie;
	char     *name;
} Event;

static PyTypeObject Event_Type;


static int
Event_init(Event *self, PyObject *args, PyObject *kwargs)
{
	static char *kwnames[] = {"wd", "mask", "cookie", "name", NULL};

	self->name = EMPTY_NAME;

	int success = PyArg_ParseTupleAndKeywords(
		args, kwargs, "iOIes:Event.__init__", kwnames,
		&self->wd, &self->mask, &self->cookie, NULL, &self->name
	);

	Py_XINCREF(self->mask);

	return success? 0:-1;
}


static void
Event_dealloc(Event *self)
{
	Py_CLEAR(self->mask);

	if ( self->name && self->name != EMPTY_NAME )
		PyMem_Free(self->name);

	Py_TYPE(self)->tp_free(self);
}


#ifndef Event_unpack_DOC
#define Event_unpack_DOC ""
#define Event_unpack_DOC_SRC src/inotify.pyi:Event.unpack
#endif

static PyObject*
Event_unpack(PyTypeObject *cls, PyObject *source)
{
	size_t remaining;
	struct inotify_event *event;
	PyObject *inst;
	Py_buffer buffer = {};

	if ( PyObject_GetBuffer(source, &buffer, PyBUF_SIMPLE) == -1 )
		return NULL;

	event = (struct inotify_event*) buffer.buf;
	remaining = buffer.len - sizeof(struct inotify_event);

	// Ensure 'remaining' is non-negative BEFORE accessing 'event' fields.
	if ( remaining < 0 || event->len > remaining ) {
		PyBuffer_Release(&buffer);
		return PyErr_Format(PyExc_ValueError, "Not enough bytes to unpack");
	}

	inst = Event_from_struct(event);
	PyBuffer_Release(&buffer);
	return inst;
}

/* Create an Event instance from the values of 'event'
 */
PyObject*
Event_from_struct(struct inotify_event *event)
{
	// TODO: maybe use PyObject_New & manual init
	PyObject *evtype = PyObject_CallFunction(EventFlag, "i", event->mask);

	if ( event->len == 0 )
		return PyObject_CallFunction(
			(PyObject*) &Event_Type, "iNIN",
			event->wd, evtype, event->cookie, PyUnicode_FromString("")
		);

	return PyObject_CallFunction(
		(PyObject*) &Event_Type, "iNIs",
		event->wd, evtype, event->cookie, event->name
	);
}


#ifndef Event_size_DOC
#define Event_size_DOC ""
#define Event_size_DOC_SRC src/inotify.pyi:Event.size
#endif

static PyObject*
Event_size(Event *self)
{
	return PyLong_FromLong(
		sizeof(struct inotify_event) +
		(self->name? strlen(self->name) : 0)
	);
}


#ifndef Event_DOC
#define Event_DOC ""
#define Event_DOC_SRC src/inotify.pyi:Event
#endif

static PyMethodDef Event_methods[] = {
	{"unpack", (PyCFunction) Event_unpack, METH_CLASS|METH_O,
		PyDoc_STR(Event_unpack_DOC)},
	{"size", (PyCFunction) Event_size, METH_NOARGS,
		PyDoc_STR(Event_size_DOC)},
	{NULL},
};

static PyMemberDef Event_members[] = {
	{"wd",     T_INT,       offsetof(Event, wd),     READONLY},
	{"mask",   T_OBJECT_EX, offsetof(Event, mask),   READONLY},
	{"cookie", T_ULONG,     offsetof(Event, cookie), READONLY},
	{"name",   T_STRING,    offsetof(Event, name),   READONLY},
	{NULL},
};

static PyTypeObject Event_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name            = "kodo.inotify.Event",
	.tp_basicsize       = sizeof(Event),
	.tp_flags           = Py_TPFLAGS_DEFAULT,
	.tp_doc             = PyDoc_STR(Event_DOC),
	.tp_new             = PyType_GenericNew,
	.tp_init            = (initproc) Event_init,
	.tp_dealloc         = (destructor) Event_dealloc,
	.tp_methods         = Event_methods,
	.tp_members         = Event_members,
};

int
add_Event(PyObject *module)
{
	if ( PyType_Ready(&Event_Type) < 0 )
		return -1;

	return PyModule_AddType(module, &Event_Type);
}
