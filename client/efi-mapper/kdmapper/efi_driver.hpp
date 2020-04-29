#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <atlstr.h>

#include "service.hpp"
#include "utils.hpp"

namespace efi_driver
{
	typedef struct _MemoryCommand
	{
		int magic;
		int operation;
		unsigned long long data[10];
		int size;
	} MemoryCommand;

	#define VARIABLE_NAME L"yromeMifE" // EfiMemory
	#define COMMAND_MAGIC 0xDEAD

	#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
	#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
	#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004
	#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
	#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
	#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
	#define EFI_VARIABLE_APPEND_WRITE                          0x00000040
	#define ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

	#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE (22L)
	
	bool Init();
	NTSTATUS SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled);
	void SendCommand(MemoryCommand* cmd);

	bool MemCopy(HANDLE device_handle, uint64_t destination, uint64_t source, uint64_t size);
	bool SetMemory(HANDLE device_handle, uint64_t address, uint32_t value, uint64_t size);
	bool ReadMemory(HANDLE device_handle, uint64_t address, void* buffer, uint64_t size);
	bool WriteMemory(HANDLE device_handle, uint64_t address, void* buffer, uint64_t size);
	uint64_t AllocatePool(HANDLE device_handle, nt::POOL_TYPE pool_type, uint64_t size);
	bool FreePool(HANDLE device_handle, uint64_t address);
	uint64_t GetKernelModuleExport(HANDLE device_handle, uint64_t kernel_module_base, const std::string& function_name);
	bool GetNtGdiDdDDIReclaimAllocations2KernelInfo(HANDLE device_handle, uint64_t* out_kernel_function_ptr, uint64_t* out_kernel_original_function_address);
	bool GetNtGdiGetCOPPCompatibleOPMInformationInfo(HANDLE device_handle, uint64_t* out_kernel_function_ptr, uint8_t* out_kernel_original_bytes);
	bool ClearMmUnloadedDrivers(HANDLE device_handle);
}
