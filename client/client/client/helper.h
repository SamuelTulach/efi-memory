#pragma once

namespace Helper 
{
	uint64_t AllocatePool(nt::POOL_TYPE pool_type, uint64_t size)
	{
		if (!size)
			return 0;

		static uint64_t kernel_ExAllocatePool = 0;

		if (!kernel_ExAllocatePool)
			kernel_ExAllocatePool = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "ExAllocatePool");

		uint64_t allocated_pool = 0;

		if (!Utils::CallKernelFunction(&allocated_pool, kernel_ExAllocatePool, pool_type, size))
			return 0;

		return allocated_pool;
	}

	bool FreePool(uint64_t address)
	{
		if (!address)
			return 0;

		static uint64_t kernel_ExFreePool = 0;

		if (!kernel_ExFreePool)
			kernel_ExFreePool = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "ExFreePool");

		return Utils::CallKernelFunction<void>(nullptr, kernel_ExFreePool, address);
	}

	uint64_t GetCurrentProcessKrnl() 
	{
		static uint64_t kernel_IoGetCurrentProcess = 0;

		if (!kernel_IoGetCurrentProcess)
			kernel_IoGetCurrentProcess = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "IoGetCurrentProcess");

		uint64_t peprocess = 0;

		if (!Utils::CallKernelFunction<uint64_t>(&peprocess, kernel_IoGetCurrentProcess))
			return 0;

		return peprocess;
	}

	NTSTATUS LookupProcess(uint32_t pid, uintptr_t* peprocess) 
	{
		static uint64_t kernel_PsLookupProcessByProcessId = 0;

		if (!kernel_PsLookupProcessByProcessId)
			kernel_PsLookupProcessByProcessId = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "PsLookupProcessByProcessId");

		NTSTATUS status;

		if (!Utils::CallKernelFunction(&status, kernel_PsLookupProcessByProcessId, pid, peprocess))
			return 0;

		return status;
	}

	uint64_t GetSectionBase(uintptr_t peprocess)
	{
		if (!peprocess)
			return 0;

		static uint64_t kernel_PsGetProcessSectionBaseAddress = 0;

		if (!kernel_PsGetProcessSectionBaseAddress)
			kernel_PsGetProcessSectionBaseAddress = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "PsGetProcessSectionBaseAddress");

		uint64_t baseaddr = 0;

		if (!Utils::CallKernelFunction(&baseaddr, kernel_PsGetProcessSectionBaseAddress, peprocess))
			return 0;

		return baseaddr;
	}

	NTSTATUS CopyVirtualMemory(
		uintptr_t sourceprocess, 
		uintptr_t sourceaddress, 
		uintptr_t destinationprocess, 
		uintptr_t destinationaddress, 
		SIZE_T size, 
		uint8_t mode, 
		PSIZE_T returnsize)
	{
		static uint64_t kernel_MmCopyVirtualMemory = 0;

		if (!kernel_MmCopyVirtualMemory)
			kernel_MmCopyVirtualMemory = Utils::GetKernelModuleExport(Utils::GetKernelModuleAddress("ntoskrnl.exe"), "MmCopyVirtualMemory");

		NTSTATUS status;

		if (!Utils::CallKernelFunction(&status, kernel_MmCopyVirtualMemory, sourceprocess, sourceaddress, destinationprocess, destinationaddress, size, mode, returnsize))
			return 0;

		return status;
	}
}