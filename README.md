<p align="center">
  <img src="assets/logo.png"/>
</p>

Efi-memory is a proof-of-concept EFI runtime driver for reading and writing to virtual memory. It uses [EfiGuards](https://github.com/Mattiwatti/EfiGuard/) method of hooking SetVariable to communicate with the user-mode process. [Here is an example how it works](https://youtu.be/XKODdIsTgzU).

## Repo content
driver/
- EFI driver itself

client/efi-mapper/
- [kdmapper](https://github.com/z175/kdmapper/) fork that uses efi-memory to manual map any Windows driver

## Compiling
Compiling any of the example client programs is pretty simple. Open the solution file in Visual Studio and compile the project with it's default settings.

Compiling the driver is also pretty simple. First you need a working Linux install (or you can use Linux subsystem for Windows) and install gnu-efi (commands for Arch Linux):
```
sudo pacman -S gnu-efi-libs
```
That's all you need to install. Package manager (in the example apt) should take care of all the depencies for you. Once the installation is complete, clone this repo (make sure you have git installed):
```   
git clone https://github.com/SamuelTulach/efi-memory
```
Than navigate to the driver folder and compile the driver with make:
```
cd efi-memory
cd driver
make
```
If the compile was successful, you should now see memory.efi in the driver folder.

**Note:** Some people were reporting that they were unable to compile the driver with some errors related to GUIDs (passing them in as a pointer). If you are having the same issues please make sure that you are using latest gcc and gnu-efi libs. Ubuntu and Debian have older versions of them and therefore require you to manually compile and install latest versions.

```
[q@propc:~]$ pacman -Q --info gnu-efi-libs
Name            : gnu-efi-libs
Version         : 3.0.11-2
Description     : Library for building UEFI Applications using GNU toolchain
Architecture    : x86_64
URL             : https://sourceforge.net/projects/gnu-efi/
Licenses        : GPL
Groups          : None
Provides        : None
Depends On      : None
Optional Deps   : None
Required By     : None
Optional For    : None
Conflicts With  : None
Replaces        : None
Installed Size  : 1943.01 KiB
Packager        : Felix Yan <felixonmars@archlinux.org>
Build Date      : Sat 16 May 2020 12:57:49 PM CEST
Install Date    : Tue 19 May 2020 03:12:17 PM CEST
Install Reason  : Explicitly installed
Install Script  : No
Validated By    : Signature

[q@propc:~]$ pacman -Q --info gcc
Name            : gcc
Version         : 10.1.0-1
Description     : The GNU Compiler Collection - C and C++ frontends
Architecture    : x86_64
URL             : https://gcc.gnu.org
Licenses        : GPL  LGPL  FDL  custom
Groups          : base-devel
Provides        : gcc-multilib
Depends On      : gcc-libs=10.1.0-1  binutils>=2.28  libmpc
Optional Deps   : lib32-gcc-libs: for generating code for 32-bit ABI [installed]
Required By     : clang  dkms
Optional For    : clion  xorg-xrdb
Conflicts With  : None
Replaces        : gcc-multilib
Installed Size  : 147.19 MiB
Packager        : Bartłomiej Piotrowski <bpiotrowski@archlinux.org>
Build Date      : Fri 08 May 2020 01:14:50 PM CEST
Install Date    : Sat 16 May 2020 02:55:54 PM CEST
Install Reason  : Explicitly installed
Install Script  : No
Validated By    : Signature

[q@propc:~]$
```

## Usage
In order to use the efi-memory driver, you need to load it. First, obtain a copy of memory.efi ([compile it](https://github.com/SamuelTulach/efi-memory#compiling) or [download it from release section](https://github.com/SamuelTulach/efi-memory/releases)) and a copy of [EDK2 efi shell](https://github.com/tianocore/edk2/releases). Now follow these steps:

1. Extract downloaded efi shell and rename file Shell.efi (should be in folder UefiShell/X64) to bootx64.efi
2. Format some USB drive to FAT32
3. Create following folder structure:
```
USB:.
 │   memory.efi
 │
 └───EFI
      └───Boot
              bootx64.efi
```
4. Boot from the USB drive
5. An UEFI shell should start, change directory to your USB (FS0 should be the USB since we are booting from it) and list files:
```
FS0:
ls
```
6. You should see file memory.efi, if you do, load it:
```
load memory.efi
```
7. Now there should be a nice efi-memory ascii logo printed in your UEFI shell. If there is, the driver was loaded successfuly. If that is the case, type `exit` to start standard boot procedure (while Windows is booting the screen should go blue with confirmation text)

## Thanks
I would like to thank [@z175](https://github.com/z175/) for kdmapper project since that is a masterpiece. [@Mattiwatti](https://github.com/Mattiwatti/) for EfiGuard project and the idea of SetVariable hooking. Roderick W. Smith for [rodsbooks.com](http://rodsbooks.com/) (really useful site to read about EFI basics).

## License
This repo is licensed under MIT if not stated otherwise in subfolders.
