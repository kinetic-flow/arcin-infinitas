# arcin-infinitas
arcin firmware for beatmania IIDX Infinitas (2020 ver)

## Where do I download???

Check out the releases tab.

## How do I flash??

Run the executable in Windows. You might need to do it a few times until it works.

If it does not detect your controller, unplug, hold down 1+2, plug back in - you are now in flashing mode. Try running the executable again.

## How do I set up in the game?

* Start the launcher
* Settings
* Press the button that resets all keybinds to DEFAULT.
* Whatever you do, do not change the turntable settings.
* You *might* be able to rebind some keys. As of launch (August 4th, 2020), this doesn't seem to work properly - the game tends to ignore any key binds if you are using an infinitas controller (or this firmware which pretends to be one)
    * This means you might need to re-wire some buttons to make it friendly for the game

## Details

This is a custom arcin firmware, mainly for playing the new version of Infinitas.

On existing infinitas firmware that people use, some people reported that their turntable is not sensitive enough in the new verison of the game, as if the "deadzone" is too large. This is probably caused by the fact that the engine for Infinitas is now based on Heroic Verse to run at 120hz. This firmware aims to fix that.

On top of the "arcin_flash_config_alpha5" firmware, the following changes are made:

* Changes the hardware ID (USB VID / PID) to be the same as Konami infinitas controller. This allows the game to automatically detect it & use the analog turntable.
* Removes input elements that the game does not expect:
   * Button 7/8 typically used for digital turntable in BMS
   * Analog Y-axis
* Increases the turntable sensitivity
   * This is a hardcoded 25% increase.
   
This firmware is technically compatible with the configuration tool (arcin_conf.exe), but you need to flash the firmware to a compatible firmware, change the settings using the tool, and switch back to this infinitas firmware. Your settings should remain on the board even after a flash.

This firmware is also technically compatible with the flashing tools, but the flashing tools are unable to automatically discover the controller becuase the USB IDs will be different from what they expect. To work around this, unplug the USB cable, hold down 1+2, plug in the USB cable, this will enter flashing mode and you will be able to proceed as normal.

## Building
This is the hard part... components required to are not included in this repository, so you need to shop around.

* Get the ARM toolchain for Linux and put it in your $PATH
    * https://launchpad.net/gcc-arm-embedded/+download
    * Try to get these files from the website instead of the ones from your OS's package repository.
* Get SCons : http://scons.org/pages/download.html
    * You can get scons from yor OS's package repository (e.g., apt-get)
* git submodule init: this will pull the external laks submodule, which contains headers for the hardware used by the arcin.

To build, run

    scons

To flash, run

    ./hidflash.py arcin.elf

To create an executable for easily flashing the ELF file, grab https://github.com/theKeithD/arcin/tree/svre9/arcin-utils and then run:

    ./hidloader_append.py arcin.elf hidloader_v2.exe arcin_flash_custom.exe

I had success with Ubuntu 20.04 LTS on WSL2 (Windows Subsystem for Linux). Mind that ARM toolchian will not work in WSL1!

## Disclosure

The repository owner or contributors are not liable for any damage caused to your hardware. That being said, arcin is a fairly robust board with recovey capabilities, so you should be fine!

This repository is a fork of zyp's respository, specifically at this commit: https://cgit.jvnv.net/arcin/commit/?h=conf&id=1c211c70f4a23a770724ce435e223d442e5fb78e

Thank you for the awesome project, zyp!
