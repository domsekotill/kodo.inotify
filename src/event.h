/*  Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sys/inotify.h>

#include <Python.h>


int add_Event(PyObject*);

PyObject* Event_from_struct(struct inotify_event *);
