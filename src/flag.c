/*  Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sys/inotify.h>

#include <Python.h>

#include "module.h"
#include "flag.h"


PyObject *EventFlag;


static int
PyDict_SetInteger(PyObject *dict, const char *name, uint32_t value)
{
	PyObject* o = PyLong_FromLong(value);

	if ( !o )
		return -1;

	int err = PyDict_SetItemString(dict, name, o);
	Py_DECREF(o);
	return err;
}


extern int
add_EventFlag(PyObject *module)
{
	int err = 0;
	PyObject* values = PyDict_New();
	PyObject* IntFlag = import_from("enum", "IntFlag");
	PyObject* FlagBoundary = import_from("enum", "FlagBoundary");
	PyObject* STRICT = PyObject_GetAttrString(FlagBoundary, "STRICT");

	if ( !values || !IntFlag )
		return -1;

	err |= PyDict_SetInteger(values, "ACCESS",        IN_ACCESS);
	err |= PyDict_SetInteger(values, "ATTRIB",        IN_ATTRIB);
	err |= PyDict_SetInteger(values, "CLOSE_WRITE",   IN_CLOSE_WRITE);
	err |= PyDict_SetInteger(values, "CLOSE_NOWRITE", IN_CLOSE_NOWRITE);
	err |= PyDict_SetInteger(values, "CREATE",        IN_CREATE);
	err |= PyDict_SetInteger(values, "DELETE",        IN_DELETE);
	err |= PyDict_SetInteger(values, "DELETE_SELF",   IN_DELETE_SELF);
	err |= PyDict_SetInteger(values, "MODIFY",        IN_MODIFY);
	err |= PyDict_SetInteger(values, "MOVE_SELF",     IN_MOVE_SELF);
	err |= PyDict_SetInteger(values, "MOVED_FROM",    IN_MOVED_FROM);
	err |= PyDict_SetInteger(values, "MOVED_TO",      IN_MOVED_TO);
	err |= PyDict_SetInteger(values, "OPEN",          IN_OPEN);
	err |= PyDict_SetInteger(values, "DONT_FOLLOW",   IN_DONT_FOLLOW);
	err |= PyDict_SetInteger(values, "EXCL_UNLINK",   IN_EXCL_UNLINK);
	err |= PyDict_SetInteger(values, "MASK_ADD",      IN_MASK_ADD);
	err |= PyDict_SetInteger(values, "ONESHOT",       IN_ONESHOT);
	err |= PyDict_SetInteger(values, "ONLYDIR",       IN_ONLYDIR);
	err |= PyDict_SetInteger(values, "MASK_CREATE",   IN_MASK_CREATE);
	err |= PyDict_SetInteger(values, "IGNORED",       IN_IGNORED);
	err |= PyDict_SetInteger(values, "ISDIR",         IN_ISDIR);
	err |= PyDict_SetInteger(values, "Q_OVERFLOW",    IN_Q_OVERFLOW);
	err |= PyDict_SetInteger(values, "UNMOUNT",       IN_UNMOUNT);

	if ( err ) {
		Py_DECREF(values);
		return -1;
	}

	PyObject* name = PyUnicode_FromString("EventFlag");
	PyObject* kwnames = Py_BuildValue("(s)", "boundary");
	PyObject* const args[] = {NULL, name, values, STRICT};

	EventFlag = PyObject_Vectorcall(IntFlag, &args[1], 2|PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
	/* EventFlag = PyObject_CallFunction(IntFlag, "sN", "EventFlag", values); */

	Py_DECREF(values);
	Py_DECREF(IntFlag);
	Py_DECREF(FlagBoundary);
	Py_DECREF(STRICT);
	Py_DECREF(name);
	Py_DECREF(kwnames);

	return PyModule_AddObject(module, "EventFlag", EventFlag);
}

extern int
EventFlag_Converter(PyObject *obj, void *out)
{
	if ( !PyObject_IsInstance(obj, EventFlag) ) {
		PyErr_Format(PyExc_TypeError, "Not a EventFlag: %R", obj);
		return 0;
	}

	uint32_t* mask = (uint32_t*) out;

	*mask = PyLong_AsLong(obj);

	return ( -1 == *mask && PyErr_Occurred() )? 0:1;
}
