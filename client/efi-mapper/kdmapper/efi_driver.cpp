#include "efi_driver.hpp"

NTSTATUS efi_driver::SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled)
{
	if (WasEnabled != nullptr)
		*WasEnabled = FALSE;

	BOOLEAN SeSystemEnvironmentWasEnabled;
	const NTSTATUS Status = nt::RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
		Enable,
		FALSE,
		&SeSystemEnvironmentWasEnabled);

	if (NT_SUCCESS(Status) && WasEnabled != nullptr)
		*WasEnabled = SeSystemEnvironmentWasEnabled;

	return Status;
}

GUID DummyGuid
= { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };
void efi_driver::SendCommand(MemoryCommand* cmd) 
{
	UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
	nt::NtSetSystemEnvironmentValueEx(&VariableName,
		&DummyGuid,
		cmd,
		sizeof(MemoryCommand),
		ATTRIBUTES);
}

bool efi_driver::Init()
{
	BOOLEAN SeSystemEnvironmentWasEnabled;
	NTSTATUS status = SetSystemEnvironmentPrivilege(true, &SeSystemEnvironmentWasEnabled);
	return NT_SUCCESS(status);
}

bool efi_driver::MemCopy(HANDLE device_handle, uint64_t destination, uint64_t source, uint64_t size)
{
	MemoryCommand* cmd = new MemoryCommand();
	cmd->operation = 0;
	cmd->magic = COMMAND_MAGIC;
	
	uintptr_t data[10];
	data[0] = destination;
	data[1] = source;

	memcpy(&cmd->data, &data[0], sizeof(data));

	cmd->size = (int)size;

	SendCommand(cmd);

	return true; // yolo
}

bool efi_driver::SetMemory(HANDLE device_handle, uint64_t address, uint32_t value, uint64_t size)
{
	for (int i = 0; i < size; i++) 
	{
		MemCopy(device_handle, address + i, (uintptr_t)&value, sizeof(uint32_t));
	}

	return true;
}

bool efi_driver::ReadMemory(HANDLE device_handle, uint64_t address, void* buffer, uint64_t size)
{
	return MemCopy(device_handle, reinterpret_cast<uint64_t>(buffer), address, size);
}

bool efi_driver::WriteMemory(HANDLE device_handle, uint64_t address, void* buffer, uint64_t size)
{
	return MemCopy(device_handle, address, reinterpret_cast<uint64_t>(buffer), size);
}

uint64_t efi_driver::AllocatePool(HANDLE device_handle, nt::POOL_TYPE pool_type, uint64_t size)
{
	if (!size)
		return 0;

	static uint64_t kernel_ExAllocatePool = 0;

	if (!kernel_ExAllocatePool)
		kernel_ExAllocatePool = GetKernelModuleExport(device_handle, utils::GetKernelModuleAddress("ntoskrnl.exe"), "ExAllocatePool");

	uint64_t allocated_pool = 0;

	MemoryCommand* cmd = new MemoryCommand();
	cmd->operation = 1;
	cmd->magic = COMMAND_MAGIC;

	uintptr_t data[10];
	data[0] = kernel_ExAllocatePool;
	data[1] = pool_type;
	data[2] = size;
	data[3] = (uintptr_t)&allocated_pool;

	memcpy(&cmd->data, &data[0], sizeof(data));

	SendCommand(cmd);

	return allocated_pool;
}

bool efi_driver::FreePool(HANDLE device_handle, uint64_t address)
{
	if (!address)
		return 0;

	static uint64_t kernel_ExFreePool = 0;

	if (!kernel_ExFreePool)
		kernel_ExFreePool = GetKernelModuleExport(device_handle, utils::GetKernelModuleAddress("ntoskrnl.exe"), "ExFreePool");

	MemoryCommand* cmd = new MemoryCommand();
	cmd->operation = 2;
	cmd->magic = COMMAND_MAGIC;

	uintptr_t data[10];
	data[0] = kernel_ExFreePool;
	data[1] = address;

	memcpy(&cmd->data, &data[0], sizeof(data));

	SendCommand(cmd);

	return true; // yolo?
}

uint64_t efi_driver::GetKernelModuleExport(HANDLE device_handle, uint64_t kernel_module_base, const std::string & function_name)
{
	if (!kernel_module_base)
		return 0;

	IMAGE_DOS_HEADER dos_header = { 0 };
	IMAGE_NT_HEADERS64 nt_headers = { 0 };

	if (!ReadMemory(device_handle, kernel_module_base, &dos_header, sizeof(dos_header)) || dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
		!ReadMemory(device_handle, kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) || nt_headers.Signature != IMAGE_NT_SIGNATURE)
		return 0;

	const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (!export_base || !export_base_size)
		return 0;

	const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	if (!ReadMemory(device_handle, kernel_module_base + export_base, export_data, export_base_size))
	{
		VirtualFree(export_data, 0, MEM_RELEASE);
		return 0;
	}

	const auto delta = reinterpret_cast<uint64_t>(export_data) - export_base;

	const auto name_table = reinterpret_cast<uint32_t*>(export_data->AddressOfNames + delta);
	const auto ordinal_table = reinterpret_cast<uint16_t*>(export_data->AddressOfNameOrdinals + delta);
	const auto function_table = reinterpret_cast<uint32_t*>(export_data->AddressOfFunctions + delta);

	for (auto i = 0u; i < export_data->NumberOfNames; ++i)
	{
		const std::string current_function_name = std::string(reinterpret_cast<char*>(name_table[i] + delta));

		if (!_stricmp(current_function_name.c_str(), function_name.c_str()))
		{
			const auto function_ordinal = ordinal_table[i];
			const auto function_address = kernel_module_base + function_table[function_ordinal];

			if (function_address >= kernel_module_base + export_base && function_address <= kernel_module_base + export_base + export_base_size)
			{
				VirtualFree(export_data, 0, MEM_RELEASE);
				return 0; // No forwarded exports on 64bit?
			}

			VirtualFree(export_data, 0, MEM_RELEASE);
			return function_address;
		}
	}

	VirtualFree(export_data, 0, MEM_RELEASE);
	return 0;
}

bool efi_driver::GetNtGdiDdDDIReclaimAllocations2KernelInfo(HANDLE device_handle, uint64_t * out_kernel_function_ptr, uint64_t * out_kernel_original_function_address)
{
	// 488b05650e1400 mov     rax, qword ptr [rip+offset]
	// ff150f211600   call    cs:__guard_dispatch_icall_fptr

	static uint64_t kernel_function_ptr = 0;
	static uint64_t kernel_original_function_address = 0;

	if (!kernel_function_ptr || !kernel_original_function_address)
	{
		const uint64_t kernel_NtGdiDdDDIReclaimAllocations2 = GetKernelModuleExport(device_handle, utils::GetKernelModuleAddress("win32kbase.sys"), "NtGdiDdDDIReclaimAllocations2");

		if (!kernel_NtGdiDdDDIReclaimAllocations2)
		{
			std::cout << "[-] Failed to get export win32kbase.NtGdiDdDDIReclaimAllocations2" << std::endl;
			return false;
		}

		const uint64_t kernel_function_ptr_offset_address = kernel_NtGdiDdDDIReclaimAllocations2 + 0x7;
		int32_t function_ptr_offset = 0; // offset is a SIGNED integer

		if (!ReadMemory(device_handle, kernel_function_ptr_offset_address, &function_ptr_offset, sizeof(function_ptr_offset)))
			return false;

		kernel_function_ptr = kernel_NtGdiDdDDIReclaimAllocations2 + 0xB + function_ptr_offset;

		if (!ReadMemory(device_handle, kernel_function_ptr, &kernel_original_function_address, sizeof(kernel_original_function_address)))
			return false;
	}

	*out_kernel_function_ptr = kernel_function_ptr;
	*out_kernel_original_function_address = kernel_original_function_address;

	return true;
}

bool efi_driver::GetNtGdiGetCOPPCompatibleOPMInformationInfo(HANDLE device_handle, uint64_t* out_kernel_function_ptr, uint8_t* out_kernel_original_bytes)
{
	// 48ff2551d81f00   jmp	cs:__imp_NtGdiGetCOPPCompatibleOPMInformation
	// cccccccccc       padding

	static uint64_t kernel_function_ptr = 0;
	static uint8_t kernel_original_jmp_bytes[12] = { 0 };

	if (!kernel_function_ptr || kernel_original_jmp_bytes[0] == 0)
	{
		const uint64_t kernel_NtGdiGetCOPPCompatibleOPMInformation = GetKernelModuleExport(device_handle, utils::GetKernelModuleAddress("win32kfull.sys"), "NtGdiGetCOPPCompatibleOPMInformation");

		if (!kernel_NtGdiGetCOPPCompatibleOPMInformation)
		{
			std::cout << "[-] Failed to get export win32kfull.NtGdiGetCOPPCompatibleOPMInformation" << std::endl;
			return false;
		}

		kernel_function_ptr = kernel_NtGdiGetCOPPCompatibleOPMInformation;

		if (!ReadMemory(device_handle, kernel_function_ptr, kernel_original_jmp_bytes, sizeof(kernel_original_jmp_bytes)))
			return false;
	}

	*out_kernel_function_ptr = kernel_function_ptr;
	memcpy(out_kernel_original_bytes, kernel_original_jmp_bytes, sizeof(kernel_original_jmp_bytes));

	return true;
}

bool efi_driver::ClearMmUnloadedDrivers(HANDLE device_handle)
{
	ULONG buffer_size = 0;
	void* buffer = nullptr;

	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemExtendedHandleInformation), buffer, buffer_size, &buffer_size);

	while (status == nt::STATUS_INFO_LENGTH_MISMATCH)
	{
		VirtualFree(buffer, 0, MEM_RELEASE);

		buffer = VirtualAlloc(nullptr, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemExtendedHandleInformation), buffer, buffer_size, &buffer_size);
	}

	if (!NT_SUCCESS(status))
	{
		VirtualFree(buffer, 0, MEM_RELEASE);
		return false;
	}

	uint64_t object = 0;

	auto system_handle_inforamtion = static_cast<nt::PSYSTEM_HANDLE_INFORMATION_EX>(buffer);

	for (auto i = 0u; i < system_handle_inforamtion->HandleCount; ++i)
	{
		const nt::SYSTEM_HANDLE current_system_handle = system_handle_inforamtion->Handles[i];

		if (current_system_handle.UniqueProcessId != reinterpret_cast<HANDLE>(static_cast<uint64_t>(GetCurrentProcessId())))
			continue;

		if (current_system_handle.HandleValue == device_handle)
		{
			object = reinterpret_cast<uint64_t>(current_system_handle.Object);
			break;
		}
	}

	VirtualFree(buffer, 0, MEM_RELEASE);

	if (!object)
		return false;

	uint64_t device_object = 0;

	if (!ReadMemory(device_handle, object + 0x8, &device_object, sizeof(device_object)))
		return false;

	uint64_t driver_object = 0;

	if (!ReadMemory(device_handle, device_object + 0x8, &driver_object, sizeof(driver_object)))
		return false;

	uint64_t driver_section = 0;

	if (!ReadMemory(device_handle, driver_object + 0x28, &driver_section, sizeof(driver_section)))
		return false;

	UNICODE_STRING us_driver_base_dll_name = { 0 };

	if (!ReadMemory(device_handle, driver_section + 0x58, &us_driver_base_dll_name, sizeof(us_driver_base_dll_name)))
		return false;

	us_driver_base_dll_name.Length = 0;

	if (!WriteMemory(device_handle, driver_section + 0x58, &us_driver_base_dll_name, sizeof(us_driver_base_dll_name)))
		return false;

	return true;
}