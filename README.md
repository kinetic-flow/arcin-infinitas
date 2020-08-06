# arcin-infinitas
arcin firmware for beatmania IIDX Infinitas (2020 ver)

## What is this?
A custom arcin firmware, mainly written for best experience playing new infinitas 120hz version. No key config or joy2key needed, everything will work with the game out of the gate. It is also general enough to work with HID-light and beatoraja. Does not work with LR2.

## Where do I download?

[Check out the releases tab.](https://github.com/minsang-github/arcin-infinitas/releases)

## How do I flash?

Run the executable in Windows. You might need to do it a few times until it works.

If it does not detect your controller, unplug, hold down 1+2, plug back in - you are now in flashing mode. Try running the executable again.

## How do I set up in the game?

* Start the launcher, enter Settings
* **Press "デフォルトに戻す" to reset all keybinds. This is important and you MUST do this!!**
    * This firmware pretends to be the official infinitas controller, so the game detects this automatically and uses the correct key binds. Even if the key binds "seem" wrong in the UI, ignore it.
* Do not change any key binds - it won't work.
    * This means you might need to re-wire some buttons to make it friendly for the game

# Details

This is a custom arcin firmware, mainly for playing the new version of Infinitas.

On existing infinitas firmware that people use, some people reported that their turntable is not sensitive enough in the new verison of the game, as if the "deadzone" is too large. This is probably caused by the fact that the engine for Infinitas is now based on Heroic Verse to run at 120hz. This firmware aims to fix that.

On top of the "arcin_flash_config_alpha5" firmware, the following changes are made:

* Changes the hardware ID (USB VID / PID) to be the same as Konami infinitas controller. This allows the game to automatically detect it & use the analog turntable.
* Removes input elements that the game does not expect:
   * Digital turntable in BMS
   * Analog Y-axis
* Remaps E1-E4 buttons correctly (start = E1, select = E2, 8 = E3, 9 = E4)
* Optionally increases the turntable sensitivity to compensate for 120hz infinitas.
   * You can use the sensitive_tt version if you want this. If you don't need this, use the normal_tt version.

In addition to Infinitas, this firmware should perfectly fine with HID-light and beatoraja (analog turntable) configurations. It will not work with LR2 because it lacks a digital turntable - this is by design.

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

## Disclaimer

The repository owner or contributors are not liable for any damage caused to your hardware. That being said, arcin is a fairly robust board with recovery capabilities, so you should be fine!

This repository is a fork of zyp's respository, specifically at this commit: https://cgit.jvnv.net/arcin/commit/?h=conf&id=1c211c70f4a23a770724ce435e223d442e5fb78e

Thank you for the awesome project, zyp!
