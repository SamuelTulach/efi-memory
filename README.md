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

Compiling the driver is also pretty simple. First you need a working Linux install (or you can use subsystem for Windows) and install gnu-efi (commands for Ubuntu 18.04):

  apt install gnu-efi