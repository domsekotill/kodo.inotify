# Copyright 2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import annotations

import ctypes
import unittest

from kodo.inotify import Event
from kodo.inotify import EventFlag


def min_max(c_int_type: ctypes._SimpleCData[int], /) -> tuple(int, int):
	"""
	Return (min, max) representable values for a given C integer type
	"""
	# https://stackoverflow.com/questions/52475749/maximum-and-minimum-value-of-c-types-integers-from-python
	signed = c_int_type(-1).value < c_int_type(0).value
	bit_size = ctypes.sizeof(c_int_type) * 8
	signed_limit = 2 ** (bit_size - 1)
	return (-signed_limit, signed_limit - 1) if signed else (0, 2 * signed_limit - 1)


class Struct(ctypes.Structure):
	"""
	Implementation of `struct inotify_event` with Python ctypes
	"""

	_fields_ = [
		("wd", ctypes.c_int),
		("mask", ctypes.c_uint32),
		("cookie", ctypes.c_uint32),
		("len", ctypes.c_uint32),
	]


class UnpackTests(unittest.TestCase):
	"""
	Tests for `Event.unpack()`
	"""

	def test_unpack(self) -> None:
		"""
		Check that unpacking acceptable byte strings produces the correct results
		"""
		strings = [
			bytes(Struct(1, 2, 3, 16)) + b"foo\0\0\0\0\0\0\0\0\0\0\0\0\0",
			bytes(Struct(1, 2, 3, 4)) + b"foo\0",
		]

		for string in strings:
			with self.subTest(string=string):
				evt = Event.unpack(string)

				assert evt.wd == 1
				assert evt.mask is EventFlag.MODIFY
				assert evt.cookie == 3
				assert evt.name == "foo"

	def test_no_name(self) -> None:
		"""
		Check that unpacking a structure without a name is handled correctly
		"""
		strings = [
			bytes(Struct(1, 2, 3, 0)),
			bytes(Struct(1, 2, 3, 0)) + b"foo\0",
		]

		for string in strings:
			with self.subTest(string=string):
				evt = Event.unpack(string)

				assert evt.wd == 1
				assert evt.mask is EventFlag.MODIFY
				assert evt.cookie == 3
				assert evt.name == ""

	def test_max(self) -> None:
		"""
		Check that the maximum int values are unpacked correctly
		"""
		_, int_max = min_max(ctypes.c_int)
		_, uint32_max = min_max(ctypes.c_uint32)
		string = bytes(Struct(wd=int_max, mask=2, cookie=uint32_max, len=0))

		evt = Event.unpack(string)

		assert evt.wd == int_max, evt.wd
		assert evt.mask is EventFlag.MODIFY
		assert evt.cookie == uint32_max
		assert evt.name == ""

	def test_bad_mask(self) -> None:
		"""
		Check that a bad mask value raises ValueError
		"""
		string = bytes(Struct(wd=1, mask=0b1000000000000, cookie=0, len=0))

		with self.assertRaises(ValueError):
			Event.unpack(string)
