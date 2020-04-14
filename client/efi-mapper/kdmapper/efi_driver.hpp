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
		unsigned long long data1;
		unsigned long long data2;
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
	bool ClearMmUnloadedDrivers(HANDLE device_handle);

	template<typename T, typename ...A>
	bool CallKernelFunction(HANDLE device_handle, T* out_result, uint64_t kernel_function_address, const A ...arguments)
	{
		constexpr auto call_void = std::is_same_v<T, void>;

		if constexpr (!call_void)
		{
			if (!out_result)
				return false;
		}
		else
		{
			UNREFERENCED_PARAMETER(out_result);
		}

		if (!kernel_function_address)
			return false;

		// Setup function call 

		const auto NtGdiDdDDIReclaimAllocations2 = reinterpret_cast<void*>(GetProcAddress(LoadLibrary("gdi32full.dll"), "NtGdiDdDDIReclaimAllocations2"));

		if (!NtGdiDdDDIReclaimAllocations2)
		{
			std::cout << "[-] Failed to get export gdi32full.NtGdiDdDDIReclaimAllocations2" << std::endl;
			return false;
		}

		// Get function pointer (@win32kbase!gDxgkInterface table) used by NtGdiDdDDIReclaimAllocations2 and save the original address (dxgkrnl!DxgkReclaimAllocations2)

		uint64_t kernel_function_ptr = 0;
		uint64_t kernel_original_function_address = 0;

		if (!GetNtGdiDdDDIReclaimAllocations2KernelInfo(device_handle, &kernel_function_ptr, &kernel_original_function_address))
			return false;

		// Overwrite the pointer with kernel_function_address

		if (!WriteMemory(device_handle, kernel_function_ptr, &kernel_function_address, sizeof(kernel_function_address)))
			return false;

		// Call function 

		if constexpr (!call_void)
		{
			using FunctionFn = T(__stdcall*)(A...);
			const auto Function = static_cast<FunctionFn>(NtGdiDdDDIReclaimAllocations2);

			*out_result = Function(arguments...);
		}
		else
		{
			using FunctionFn = void(__stdcall*)(A...);
			const auto Function = static_cast<FunctionFn>(NtGdiDdDDIReclaimAllocations2);

			Function(arguments...);
		}

		// Restore the pointer

		WriteMemory(device_handle, kernel_function_ptr, &kernel_original_function_address, sizeof(kernel_original_function_address));
		return true;
	}
}