import os

env = Environment(
	ENV = os.environ,
)

SConscript('laks/build_rules')

env.SelectMCU('stm32f303rc')

env.Firmware('arcin.elf', Glob('*.cpp'))
