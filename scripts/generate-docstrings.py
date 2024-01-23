# Copyright 2023-2024 Dominik Sekotill <dom.sekotill@kodo.org.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

"""
Script for generating docstring #define lines from header files and Python stubs

This script looks for lines in each input file matching:

```c
#define <NAME>_DOC_SRC python/stub.pyi:object.path
```

These are then used to extract docstrings from the given objects in stub files and add them
to the output file as:

```c
#define <NAME>_DOC "The extracted docstring"
```
"""

import re
import sys
from collections.abc import Iterator
from contextlib import suppress
from itertools import accumulate
from pathlib import Path
from types import ModuleType

SCRIPT = Path(__file__)
ROOT = SCRIPT.parent.parent

PACKAGE = "kodo.inotify"

define_re = re.compile(
	r"""
	[#]define\s+(?P<target>[A-Za-z_]+_DOC)_SRC\s+(?P<source>\S+)
	""",
	re.X,
)


class CLiteralTranslate:
	"""
	Mapping of bytes to escape sequences for C string literals

	If 'skip' is given any printable characters in it will not be escaped; this only affects
	characters that would actually be escaped otherwise
	"""

	def __init__(self, skip: bytes = b""):
		self.skip = skip

	def __getitem__(self, ordinal: int) -> bytes:
		match bytes([ordinal]):
			case b"'" | b'"' | b"\\" | b"?" as character if ordinal not in self.skip:
				return b"\\" + character
			case b"\t":
				return b"\\t"
			case b"\n":
				return b"\\n"
			case b"\r":
				return b"\\r"
			case character:
				if 32 <= ordinal < 127:
					return character
				return f"\\x{ordinal:02X}".encode()

	def with_regex(self, match: re.Match[bytes]) -> bytes:
		"""
		Provide replacements as callable required by `re.sub` or `re.Pattern.sub`
		"""
		if len(char := match.group(0)) != 1:
			raise ValueError(f"{match} has a length != 1")
		return self[ord(char)]


def mock_package(name: str, path: Path) -> None:
	"""
	Add missing mock packages to sys.modules
	"""
	names = name.split('.')
	for pkgname in accumulate(names, lambda *p: ".".join(p), initial=names.pop(0)):
		try:
			pkg = sys.modules[pkgname]
		except KeyError:
			pkg = sys.modules[pkgname] = ModuleType(pkgname)
			pkg.__path__ = []
	pkg.__path__.append(path.absolute().as_posix())


def find_module_by_path(path: Path, package: str) -> ModuleType:
	"""
	Return the module loaded from path, or raise KeyError
	"""
	for module in sys.modules.values():
		if getattr(module, "__package__", None) != package:
			continue
		if path.samefile(getattr(module, "__file__", "/dev/null")):
			return module
	raise KeyError(path)


def load_source(source: Path, package: str = "", name: str = "") -> object:
	"""
	Load and execute the given file as a Python module from the given package
	"""
	source = Path(source)
	try:
		return find_module_by_path(source, package)
	except KeyError:
		pass
	name = name or source.with_suffix("").name
	modname = f"{package}.{name}" if package else name
	if package and package not in sys.modules:
		mock_package(package, source.parent)
	module = sys.modules[modname] = ModuleType(modname)
	module.__package__ = package
	module.__file__ = source.as_posix()
	with source.open("rb") as fhandle:
		code = compile(fhandle.read(), source, "exec")
	exec(code, module.__dict__)
	return module


def docstring_defs(*sources: Path) -> Iterator[tuple[str, str]]:
	"""
	Return an iterator yielding (macro-name, source-spec) strings from header files
	"""
	for source in sources:
		for path in source.glob("**/*") if source.is_dir() else [source]:
			with suppress(IsADirectoryError, UnicodeDecodeError):
				for match in define_re.finditer(path.read_text()):
					yield match.group("target", "source")  # type: ignore


def get_docstring(spec: str) -> str|None:
	"""
	Return a docstring from a spec formatted as "source-file" or "source-file:object-path"
	"""
	try:
		source, object_name = spec.split(":", 1)
	except ValueError:
		return load_source(ROOT / spec, PACKAGE).__doc__
	module = load_source(ROOT / source, PACKAGE)
	obj = follow_names(module, *object_name.split("."))
	return obj.__doc__


def follow_names(obj: object, name: str, *descendants: str) -> object:
	"""
	Recursively follow the named object path given as arguments
	"""
	if descendants:
		return follow_names(getattr(obj, name), *descendants)
	return getattr(obj, name)


def main() -> None:
	"""
	Generate docstring #defines from header files and Python stubs
	"""
	if len(sys.argv) <= 2:
		usage = f"Usage: {sys.argv[0]} SOURCE [SOURCE ...] OUTFILE\n"
		sys.stderr.write(usage)
		sys.exit(2)

	with open(sys.argv[-1], "w") as output:
		paths = (Path(p) for p in sys.argv[1:-1])
		for macro, source in docstring_defs(*paths):
			docstring = get_docstring(source) or ""
			docstring_enc = (
				re
				.compile(rb"[\x00-\xff]", re.S)
				.sub(CLiteralTranslate().with_regex, docstring.encode("utf-8"))
				.decode()
			)
			output.write(f'#define {macro} "{docstring_enc}"\n')


if __name__ == "__main__":
	main()
