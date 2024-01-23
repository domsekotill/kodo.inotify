/*  Copyright 2023-2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Python.h>


/* Return an object extracted by name from the named module
 */
PyObject* import_from(const char *module, const char *name);

PyObject* raise_from_errno(const char *name);
