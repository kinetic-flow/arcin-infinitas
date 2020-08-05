import os

env = Environment(
	ENV = os.environ,
)

if os.environ.get('ARCIN_INF_SENSITIVE_TT') == '1':
	print("* * * Sensitive turntable mode for 120hz infinitas * * *")
	env.Append(
		CPPDEFINES = {
			'ARCIN_INFINITAS_SENSITIVE_TT': 1
		}
	)
else:
	print("* * * Normal turntable mode * * *")

if os.environ.get('ARCIN_INF_FLIP_START_SELECT') == '1':
	print("* * * Flip start and select buttons * * *")
	env.Append(
		CPPDEFINES = {
			'ARCIN_INFINITAS_FLIP_START_SELECT': 1
		}
	)
else:
	print("* * * Start and select buttons NOT flipped * * *")

SConscript('laks/build_rules')

env.SelectMCU('stm32f303rc')

env.Firmware('arcin.elf', Glob('arcin/*.cpp'), LINK_SCRIPT = 'arcin/arcin.ld')

env.Firmware('bootloader.elf', Glob('bootloader/*.cpp'), LINK_SCRIPT = 'bootloader/bootloader.ld')

env.Firmware('test.elf', Glob('test/*.cpp'))
