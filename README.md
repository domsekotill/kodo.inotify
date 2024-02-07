INotify
=======

This was a quick project I made over December '23 & January '24 to both stretch my legs with 
the Python C-API and attempt to make a modern [inotify][inotify-man] package for Python that 
left all I/O to the user, allowing them to use any I/O framework they wanted.

My need for an inotify package disappeared so it has been left in this functional but 
bare-bones state for the future.

[inotify-man]:
	https://www.man7.org/linux/man-pages/man7/inotify.7.html
	"inotify manual page hosted at man7.org"


Build
-----

The package can be built with any PEP 517 builder; for instance `pip` (>=20.0):

```bash
# Build and install
pip install /path/to/project/directory

# Build a wheel package
pip wheel
```

Or with [build](https://pypi.org/project/build/):

```bash
# Build a wheel package
python -m build --wheel

# Build a source package
python -m build --sdist
```


Usage
-----

The package can be used synchronously in blocking mode (the default):

```python
from kodo import inotify

watcher = inotify.INotify()
watcher.add_watch("/tmp", inotify.EventFlag.CREATE)

while (evt := watcher.read_event()):
	print(f"{evt.name} created in /tmp")
```

Or asynchronously in non-blocking mode with an I/O framework, for instance using 
[`trio`](https://trio.readthedocs.io/en/stable/) (a nice alternative to `asyncio`):

```python
import trio.lowlevel
from pathlib import Path
from kodo import inotify

async def watch() -> None:
	watcher = inotify.INotify()
	watcher.setblocking(False)

	watcher.add_watch("/tmp", inotify.EventFlag.CREATE)

	while 1:
		await trio.lowlevel.wait_readable(watcher)
		evt = watcher.read_event()
		print(f"{evt.name} created in /tmp")

trio.run(watch)
```
