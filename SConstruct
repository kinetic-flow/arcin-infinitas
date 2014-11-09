import os

env = Environment(
	ENV = os.environ,
)

SConscript('laks/build_rules')

env.SelectMCU('stm32f303rc')

env.Firmware('arcin.elf', Glob('arcin/*.cpp'), LINK_SCRIPT = 'arcin/arcin.ld')

env.Firmware('bootloader.elf', Glob('bootloader/*.cpp'), LINK_SCRIPT = 'bootloader/bootloader.ld')

env.Firmware('test.elf', Glob('test/*.cpp'))
