#pragma once

namespace Driver 
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

	GUID DummyGuid
		= { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };

	#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
	#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
	#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004
	#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
	#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
	#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
	#define EFI_VARIABLE_APPEND_WRITE                          0x00000040
	#define ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

	NTSTATUS SetSystemEnvironmentPrivilege(BOOLEAN Enable, PBOOLEAN WasEnabled)
	{
		if (WasEnabled != nullptr)
			*WasEnabled = FALSE;

		BOOLEAN SeSystemEnvironmentWasEnabled;
		const NTSTATUS Status = nt::RtlAdjustPrivilege(22L, // SE_SYSTEM_ENVIRONMENT_PRIVILEGE
			Enable,
			FALSE,
			&SeSystemEnvironmentWasEnabled);

		if (NT_SUCCESS(Status) && WasEnabled != nullptr)
			*WasEnabled = SeSystemEnvironmentWasEnabled;

		return Status;
	}

	void SendCommand(MemoryCommand* cmd) 
	{
		UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
		NTSTATUS status = nt::NtSetSystemEnvironmentValueEx(&VariableName,
			&DummyGuid,
			&cmd,
			sizeof(MemoryCommand),
			ATTRIBUTES);
	}

	bool Init() 
	{
		BOOLEAN SeSystemEnvironmentWasEnabled;
		NTSTATUS status = SetSystemEnvironmentPrivilege(TRUE, &SeSystemEnvironmentWasEnabled);
		return NT_SUCCESS(status);
	}

	bool Test() 
	{
		uintptr_t read = 0;
		uintptr_t value = 123;

		MemoryCommand cmd;
		cmd.operation = 0;
		cmd.magic = COMMAND_MAGIC;
		cmd.data1 = (uintptr_t)&read;
		cmd.data2 = (uintptr_t)&value;
		cmd.size = sizeof(uintptr_t);

		SendCommand(&cmd);

		return (read == 123);
	}
}