#pragma once

namespace Utils 
{
	uint64_t GetKernelModuleAddress(const std::string& module_name)
	{
		void* buffer = nullptr;
		DWORD buffer_size = 0;

		NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buffer, buffer_size, &buffer_size);

		while (status == nt::STATUS_INFO_LENGTH_MISMATCH)
		{
			VirtualFree(buffer, 0, MEM_RELEASE);

			buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buffer, buffer_size, &buffer_size);
		}

		if (!NT_SUCCESS(status))
		{
			VirtualFree(buffer, 0, MEM_RELEASE);
			return 0;
		}

		const auto modules = static_cast<nt::PRTL_PROCESS_MODULES>(buffer);

		for (auto i = 0u; i < modules->NumberOfModules; ++i)
		{
			const std::string current_module_name = std::string(reinterpret_cast<char*>(modules->Modules[i].FullPathName) + modules->Modules[i].OffsetToFileName);

			if (!_stricmp(current_module_name.c_str(), module_name.c_str()))
			{
				const uint64_t result = reinterpret_cast<uint64_t>(modules->Modules[i].ImageBase);

				VirtualFree(buffer, 0, MEM_RELEASE);
				return result;
			}
		}

		VirtualFree(buffer, 0, MEM_RELEASE);
		return 0;
	}

	uint64_t GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name)
	{
		if (!kernel_module_base)
			return 0;

		IMAGE_DOS_HEADER dos_header = { 0 };
		IMAGE_NT_HEADERS64 nt_headers = { 0 };

		Driver::ReadMemory(kernel_module_base, &dos_header, sizeof(dos_header)); 
		
		if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
			return 0;
		
		Driver::ReadMemory(kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers));
		
		if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
			return 0;

		const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

		if (!export_base || !export_base_size)
			return 0;

		const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

		Driver::ReadMemory(kernel_module_base + export_base, export_data, export_base_size);

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

	bool GetNtGdiDdDDIReclaimAllocations2KernelInfo(uint64_t* out_kernel_function_ptr, uint64_t* out_kernel_original_function_address)
	{
		// 488b05650e1400 mov     rax, qword ptr [rip+offset]
		// ff150f211600   call    cs:__guard_dispatch_icall_fptr

		static uint64_t kernel_function_ptr = 0;
		static uint64_t kernel_original_function_address = 0;

		if (!kernel_function_ptr || !kernel_original_function_address)
		{
			const uint64_t kernel_NtGdiDdDDIReclaimAllocations2 = GetKernelModuleExport(GetKernelModuleAddress("win32kbase.sys"), "NtGdiDdDDIReclaimAllocations2");

			if (!kernel_NtGdiDdDDIReclaimAllocations2)
			{
				std::cout << "[-] Failed to get export win32kbase.NtGdiDdDDIReclaimAllocations2" << std::endl;
				return false;
			}

			const uint64_t kernel_function_ptr_offset_address = kernel_NtGdiDdDDIReclaimAllocations2 + 0x7;
			int32_t function_ptr_offset = 0; // offset is a SIGNED integer

			Driver::ReadMemory(kernel_function_ptr_offset_address, &function_ptr_offset, sizeof(function_ptr_offset));

			kernel_function_ptr = kernel_NtGdiDdDDIReclaimAllocations2 + 0xB + function_ptr_offset;

			Driver::ReadMemory(kernel_function_ptr, &kernel_original_function_address, sizeof(kernel_original_function_address));
		}

		*out_kernel_function_ptr = kernel_function_ptr;
		*out_kernel_original_function_address = kernel_original_function_address;

		return true;
	}
}