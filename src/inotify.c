/*  Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <fcntl.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <Python.h>

#include "event.h"
#include "flag.h"
#include "module.h"

#if __has_include("docstrings.h")
#include "docstrings.h"
#endif


typedef struct {
	PyObject_HEAD
	int socket;
} INotify;


static int
INotify_init(INotify *self, PyObject *args, PyObject *kwargs)
{
	/* static char *kwnames[] = {}; */

	/* int success = PyArg_ParseTupleAndKeywords( */
	/* 	args, kwargs, ":INotify.__init__", kwnames */
	/* ); */
	/* if ( !success ) */
	/* 	return -1; */

	self->socket = inotify_init1(IN_CLOEXEC);
	if ( self->socket == -1 ) {
		raise_from_errno("inotify_init1");
		return -1;
	}

	return 0;
}


static void
INotify_dealloc(INotify *self)
{
	// TODO: should be in finalize?
	close(self->socket);
}


#ifndef INotify_fileno_DOC
#define INotify_fileno_DOC ""
#define INotify_fileno_DOC_SRC src/inotify.pyi:INotify.fileno
#endif

static PyObject*
INotify_fileno(INotify *self)
{
	return PyLong_FromLong(self->socket);
}


#ifndef INotify_setblocking_DOC
#define INotify_setblocking_DOC ""
#define INotify_setblocking_DOC_SRC src/inotify.pyi:INotify.setblocking
#endif

static PyObject*
INotify_setblocking(INotify *self, PyObject *state)
{
	if ( !PyBool_Check(state) )
		return PyErr_Format(PyExc_TypeError, "Not a bool: %R", state);

	int flags = fcntl(self->socket, F_GETFL);

	if ( Py_True == state )
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	fcntl(self->socket, F_SETFL, flags);

	Py_RETURN_NONE;
}


#ifndef INotify_add_watch_DOC
#define INotify_add_watch_DOC ""
#define INotify_add_watch_DOC_SRC src/inotify.pyi:INotify.add_watch
#endif

static PyObject*
INotify_add_watch(INotify *self, PyObject *args, PyObject *kwargs)
{
	PyObject *path;
	uint32_t mask;
	int wd;

	static char *kwnames[] = {"path", "events"};
	int success = PyArg_ParseTupleAndKeywords(
		args, kwargs, "O&O&:INotify.add_watch", kwnames,
		PyUnicode_FSConverter, (void*) &path,
		EventFlag_Converter, (void*) &mask
	);
	if ( !success )
		return NULL;

	wd = inotify_add_watch(self->socket, PyBytes_AsString(path), mask);
	if ( -1 == wd )
		return raise_from_errno("inotify_add_watch");

	return PyLong_FromLong(wd);
}


#ifndef INotify_rm_watch_DOC
#define INotify_rm_watch_DOC ""
#define INotify_rm_watch_DOC_SRC src/inotify.pyi:INotify.rm_watch
#endif

static PyObject*
INotify_rm_watch(INotify *self, PyObject *watch_descriptor)
{
	long wd = PyLong_AsLong(watch_descriptor);

	if ( -1 == wd && PyErr_Occurred() )
		return NULL;
	if ( wd < 0 || wd > INT_MAX )
		return PyErr_Format(PyExc_ValueError, "Invalid watch descriptor: %R", watch_descriptor);

	if ( -1 == inotify_rm_watch(self->socket, wd) )
		return raise_from_errno("inotify_rm_watch");

	Py_RETURN_NONE;
}


#ifndef INotify_read_event_DOC
#define INotify_read_event_DOC ""
#define INotify_read_event_DOC_SRC src/inotify.pyi:INotify.read_event
#endif

static PyObject*
INotify_read_event(INotify *self)
{
	char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
	struct inotify_event *evt = (struct inotify_event*) buf;
	ssize_t size;

	size = read(self->socket, (void*) buf, sizeof(buf));
	if ( size < 0 )
		return raise_from_errno("read");
	if ( size < sizeof(struct inotify_event) || size < sizeof(struct inotify_event) + evt->len )
		return PyErr_Format(PyExc_RuntimeError, "Incomplete read from inotify socket");

	return Event_from_struct(evt);
}


#ifndef INotify_DOC
#define INotify_DOC ""
#define INotify_DOC_SRC src/inotify.pyi:INotify
#endif

static PyMethodDef INotify_methods[] = {
	{"fileno", (PyCFunction) INotify_fileno, METH_NOARGS,
		PyDoc_STR(INotify_fileno_DOC)},
	{"setblocking", (PyCFunction) INotify_setblocking, METH_O,
		PyDoc_STR(INotify_setblocking_DOC)},
	{"add_watch", (PyCFunction) INotify_add_watch, METH_VARARGS,
		PyDoc_STR(INotify_add_watch_DOC)},
	{"rm_watch", (PyCFunction) INotify_rm_watch, METH_O,
		PyDoc_STR(INotify_rm_watch_DOC)},
	{"read_event", (PyCFunction) INotify_read_event, METH_NOARGS,
		PyDoc_STR(INotify_read_event_DOC)},
	{NULL},
};

#if 0
static PyMemberDef INotify_members[] = {
	{NULL},
};
#endif

static PyTypeObject INotify_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name            = "kodo.inotify.INotify",
	.tp_basicsize       = sizeof(INotify),
	.tp_flags           = Py_TPFLAGS_DEFAULT,
	.tp_doc             = PyDoc_STR(INotify_DOC),
	.tp_new             = PyType_GenericNew,
	.tp_init            = (initproc) INotify_init,
	.tp_dealloc         = (destructor) INotify_dealloc,
	.tp_methods         = INotify_methods,
	//.tp_members         = INotify_members,
};

int
add_INotify(PyObject *module)
{
	if ( PyType_Ready(&INotify_Type) < 0 )
		return -1;

	return PyModule_AddType(module, &INotify_Type);
}
