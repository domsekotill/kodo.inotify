# Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""
Flag enums and data class for use with inotify
"""
from dataclasses import dataclass
from enum import IntFlag
from enum import auto
from os import PathLike
from typing import Self

class EventFlag(IntFlag):
	"""
	Flag values that can be passed to `inotify_add_watch(2)` and received in event structures

	These values correspond to the `IN_*` values from the `sys/inotify.h` header.
	"""

	ACCESS = auto()
	ATTRIB = auto()
	CLOSE_WRITE = auto()
	CLOSE_NOWRITE = auto()
	CREATE = auto()
	DELETE = auto()
	DELETE_SELF = auto()
	MODIFY = auto()
	MOVE_SELF = auto()
	MOVED_FROM = auto()
	MOVED_TO = auto()
	OPEN = auto()
	DONT_FOLLOW = auto()
	EXCL_UNLINK = auto()
	MASK_ADD = auto()
	ONESHOT = auto()
	ONLYDIR = auto()
	MASK_CREATE = auto()
	IGNORED = auto()
	ISDIR = auto()
	Q_OVERFLOW = auto()
	UNMOUNT = auto()


@dataclass
class Event:
	"""
	An event object corresponding to `struct inotify_event` structures
	"""

	wd: int
	"""
	Watch descriptor: corresponds to descriptors returned from `inotify_add_watch(2)`
	"""

	mask: EventFlag
	"""
	A bitfield (mask) of `EventFlag` flags
	"""

	cookie: int
	"""
	If non-zero this value links an event with other events immediately before or after
	"""

	name: str
	"""
	If not empty, the filesystem path the event refers to
	"""

	@classmethod
	def unpack(cls, buffer: bytes, /) -> Self:
		"""
		Return a tuple of (instance, bytes_read) for an instance unpacked from a buffer

		The buffer data is expected to contain `struct inotify_event` structured data as
		described in `inotify(7)`; note this data must be in host byte order.
		"""

	def size(self) -> int:
		"""
		Return the size of the data structure an event fills

		This is the sum of the size of the `struct inotify_event` structure and length of
		any name string appended to it.
		"""

class INotify:

	def fileno(self) -> int: ...
	def setblocking(self, state: bool, /) -> None: ...
	def add_watch(self, path: PathLike[str], events: EventFlag) -> int: ...
	def rm_watch(self, watch_descriptor: int, /) -> None: ...
	def read_event(self) -> Event: ...
