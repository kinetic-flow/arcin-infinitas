import os

env = Environment(
	ENV = os.environ,
)

if os.environ.get('ARCIN_INFINITAS_250HZ_MODE') == '1':
	print("* * * Modified 250hz mode * * *")
	env.Append(
		CPPDEFINES = {
			'ARCIN_INFINITAS_250HZ_MODE': 1
		}
	)
else:
	print("* * * Default 1000hz mode * * *")

SConscript('laks/build_rules')

env.SelectMCU('stm32f303rc')

env.Firmware('arcin.elf', Glob('arcin/*.cpp'), LINK_SCRIPT = 'arcin/arcin.ld')

env.Firmware('bootloader.elf', Glob('bootloader/*.cpp'), LINK_SCRIPT = 'bootloader/bootloader.ld')

env.Firmware('test.elf', Glob('test/*.cpp'))
