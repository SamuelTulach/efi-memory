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
}