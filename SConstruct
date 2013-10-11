import os

env = Environment(
	ENV = os.environ,
)

SConscript('laks/build_rules')

env.SelectMCU('stm32f303rc')

env.Firmware('arcin.elf', ['main.cpp'], LINK_SCRIPT = 'arcin.ld')

env.Firmware('bootloader.elf', ['bootloader.cpp'], LINK_SCRIPT = 'bootloader.ld')
