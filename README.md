<p align="center">
  <img src="assets/logo.png"/>
</p>

Efi-memory is a proof-of-concept EFI runtime driver for reading and writing to virtual memory. It hooks SetVariable() to communicate with client program in the OS. 

## Repo content
driver/
- EFI driver itself
client/efi-mapper
- [kdmapper](https://github.com/z175/kdmapper/) fork that uses efi-memory to manual map any Windows driver