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
}