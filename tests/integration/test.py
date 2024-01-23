# Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from pathlib import Path
from tempfile import TemporaryDirectory

import trio.lowlevel
from kodo.inotify import EventFlag
from kodo.inotify import INotify


async def mkfile_task(path: Path, delay: float = 0.5) -> None:
	"""
	Create a file at 'path' after the given delay (seconds)
	"""
	await trio.sleep(delay)
	path.touch()


async def test() -> None:
	"""
	Check that creating a file results in an event being delivered
	"""
	inotify = INotify()
	inotify.setblocking(False)

	with TemporaryDirectory() as temp:
		wd = inotify.add_watch((dirpath := Path(temp)), EventFlag.CREATE)

		async with trio.open_nursery() as nursery:
			nursery.start_soon(mkfile_task, dirpath / "test.txt")

			await trio.lowlevel.wait_readable(inotify)
			evt = inotify.read_event()

		inotify.rm_watch(evt.wd)

	assert evt.wd == wd
	assert evt.mask == EventFlag.CREATE
	assert evt.cookie == 0
	assert evt.name == "test.txt"


if __name__ == "__main__":
	trio.run(test)
