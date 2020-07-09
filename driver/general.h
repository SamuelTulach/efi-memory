#ifndef GENERAL_H
#define GENERAL_H

// Since some retard decided to use M$ ABI in EFI standard
// instead of SysV ABI, we now have to do transitions
// GNU-EFI has a functionality for this (thanks god)
#define GNU_EFI_USE_MS_ABI 1
#define stdcall __attribute__((stdcall)) // wHy NoT tO jUsT uSe MsVc
#define fastcall __attribute__((fastcall))
// EFIAPI == __attribute__((ms_abi))

// Mandatory defines
#include <efi.h>
#include <efilib.h>

#endif