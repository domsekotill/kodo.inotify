project('kodo.inotify', 'c')

python = import('python').find_installation('python3', pure: false)


link_args = [
	'-flinker-output=rel',
]

# If enabled add flags to check all extension functions are provided by the
# platform implementation
if not get_option('check-all-defined').disabled()
	link_args += [
		'-Wl,--no-undefined',
		'-lpython@0@'.format(python.language_version()),
	]
endif


docstrings = custom_target(
	'docstrings',
	input: [
		'scripts/generate-docstrings.py',
		'src/event.c',
		'src/module.c',
	],
	output: ['docstrings.h'],
	command: [python, '@INPUT@', '@OUTPUT@'],
)

python.install_sources(
	'src/inotify.pyi',
	subdir: 'kodo',
)

python.extension_module(
	'inotify',
	[
		'src/inotify.c',
		'src/event.c',
		'src/flag.c',
		'src/module.c',
		docstrings,
	],
	subdir: 'kodo',
	install: true,
	link_args: link_args,
)
