<p align="center">
  <img src="assets/logo.png"/>
</p>

Efi-memory is a proof-of-concept EFI runtime driver for reading and writing to virtual memory. It hooks SetVariable() to communicate with client program in the OS. 

## Repo content
driver/
- EFI driver itself

client/efi-mapper/
- [kdmapper](https://github.com/z175/kdmapper/) fork that uses efi-memory to manual map any Windows driver

## Compiling
Compiling any of the example client programs is pretty simple. Open the solution file in Visual Studio and compile the project with it's default settings.

Compiling the driver is also pretty simple. First you need a working Linux install (or you can use Linux subsystem for Windows) and install gnu-efi (commands for Ubuntu 18.04):

    apt install gnu-efi

That's all you need to install. Package manager (in the example apt) should take care of all the depencies for you. Once the installation is complete, clone this repo (make sure you have git installed):
    
    git clone https://github.com/SamuelTulach/efi-memory

Than navigate to the driver folder and compile the driver with make:

    cd efi-memory
    cd driver
    make

If the compile was successful, you should now see memory.efi in the driver folder.

## Usage
In order to use the efi-memory driver, you need to load it. First, obtain a copy of memory.efi ([compile it](https://github.com/SamuelTulach/efi-memory#compiling) or [download it from release section](https://github.com/SamuelTulach/efi-memory/releases)) and a copy of [EDK2 efi shell](https://github.com/tianocore/edk2/releases). Now follow these steps:

1. Extract downloaded efi shell and rename file Shell.efi (should be in folder UefiShell/X64) to bootx64.efi
2. Format some USB driver to FAT32
3. Create following folder structure:

    USB:.
    │ memory.efi
    │
    └───EFI
    └───Boot
    bootx64.efi
 