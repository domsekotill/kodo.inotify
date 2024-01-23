/*  Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Python.h>

#include "inotify.h"
#include "event.h"
#include "flag.h"

#if __has_include("docstrings.h")
#include "docstrings.h"
#endif

#ifndef INOTIFY_DOC
#define INOTIFY_DOC ""
#define INOTIFY_DOC_SRC src/inotify.pyi
#endif


static PyModuleDef_Slot module_slots[] = {
	{Py_mod_exec, add_EventFlag},
	{Py_mod_exec, add_Event},
	{Py_mod_exec, add_INotify},
	{0, NULL},
};

static PyModuleDef module = {
	.m_base             = PyModuleDef_HEAD_INIT,
	.m_name             = "kodo.inotify",
	.m_doc              = PyDoc_STR(INOTIFY_DOC),
	.m_slots            = module_slots,
};


PyMODINIT_FUNC
PyInit_inotify(void)
{
	if ( PyType_Ready(&Event_Type) < 0 )
		return NULL;

	return PyModuleDef_Init(&module);
}


PyObject*
import_from(const char *module, const char *name)
{
	PyObject *mod;

	if ( !(mod = PyImport_ImportModule(module)) )
		return NULL;
	Py_DECREF(mod);  // Not keeping the new reference to mod

	return PyObject_GetAttrString(mod, name);
}


PyObject*
raise_from_errno(const char *name)
{
	switch (errno) {
		case ENOMEM:
			return PyErr_NoMemory();
		case EACCES:
			return PyErr_SetFromErrno(PyExc_PermissionError);
		case ENOENT:
			return PyErr_SetFromErrno(PyExc_FileNotFoundError);
		case ENOTDIR:
			return PyErr_SetFromErrno(PyExc_NotADirectoryError);
		case EISDIR:
			return PyErr_SetFromErrno(PyExc_IsADirectoryError);
		case EINTR:
			return PyErr_SetFromErrno(PyExc_InterruptedError);
		case EAGAIN:
		#if EWOULDBLOCK != EAGAIN
		case EWOULDBLOCK:
		#endif
			return PyErr_SetFromErrno(PyExc_BlockingIOError);
		case EBADF:
		case EFAULT:
		case EINVAL:
			// TODO: Might need strerror_r for thread safety
			return PyErr_Format(
				PyExc_RuntimeError,
				"An error occurred when calling %s: %s (%s)",
				name, strerror(errno), strerrorname_np(errno)
			);
		default:
			return PyErr_SetFromErrno(PyExc_OSError);
	}
}
