#include "general.h"

// Dummy hooks
#include "dummy.h"

// Since Windows does not want to allocate execusable memory for our driver
// in new versions of the OS, then we have to do it ourselves (I guess)
// If you are on Windows 1909 and bellow, please use older version of the mapper
// that use ExAllocatePool
#define DRIVER_SIZE 26214400 // 25mb should be enough
__attribute__((section(".text"))) char DriverBuffer[DRIVER_SIZE]; // It has to be in the text section so it's executable (surprise)

// Our protocol GUID (should be different for every driver)
static const EFI_GUID ProtocolGuid
    = { 0x2b479eea, 0x0ecf, 0x4a46, {0x96, 0x84, 0x8f, 0x14, 0xed, 0x92, 0xd9, 0xec} };

// VirtualAddressMap GUID (gEfiEventVirtualAddressChangeGuid)
static const EFI_GUID VirtualGuid
    = { 0x13FA7698, 0xC831, 0x49C7, { 0x87, 0xEA, 0x8F, 0x43, 0xFC, 0xC2, 0x51, 0x96 }};

// ExitBootServices GUID (gEfiEventExitBootServicesGuid)
static const EFI_GUID ExitGuid
    = { 0x27ABF055, 0xB1B8, 0x4C26, { 0x80, 0x48, 0x74, 0x8F, 0x37, 0xBA, 0xA2, 0xDF }};

// Dummy protocol struct
typedef struct _DummyProtocalData{
    UINTN blank;
} DummyProtocalData;

// Pointers to original functions
static EFI_SET_VARIABLE oSetVariable = NULL;

// Global declarations
static EFI_EVENT NotifyEvent = NULL;
static EFI_EVENT ExitEvent = NULL;
static BOOLEAN Virtual = FALSE;
static BOOLEAN Runtime = FALSE;

// Defines used to check if call is really coming from client
#define VARIABLE_NAME L"yromeMifE" // EfiMemory
#define COMMAND_MAGIC 0xDEAD

// Struct containing data used to communicate with the client
typedef struct _MemoryCommand 
{
    int magic;
    int operation;
    unsigned long long data[10];
    int size;
} MemoryCommand;

// Functions (Windows only)
typedef uintptr_t (stdcall *ExAllocatePool)(int type, uintptr_t size);
typedef void (stdcall *ExFreePool)(uintptr_t address);
typedef void (stdcall *StandardFuncStd)();
typedef void (fastcall *StandardFuncFast)();
typedef unsigned long (stdcall *DriverEntry)(void* driver, void* registry);

// Function that actually performs the r/w
EFI_STATUS
RunCommand(MemoryCommand* cmd) 
{
    // Check if the command has right magic
    // (just to be sure again)
    if (cmd->magic != COMMAND_MAGIC) 
    {
        return EFI_ACCESS_DENIED;
    }

    // Copy operation
    if (cmd->operation == 0) 
    {
        // Same as memcpy function
        CopyMem(cmd->data[0], cmd->data[1], cmd->size);    

        return EFI_SUCCESS;
    }

    // "Allocate" memory
    if (cmd->operation == 1) 
    {
        if (cmd->data[2] < DRIVER_SIZE)  // Only small driver allowed, big drivers bad
        {
            *(uintptr_t*)cmd->data[3] = &DriverBuffer; // Get rekt windows
        }      
    }

    // Call any void function (__stdcall)
    if (cmd->operation == 3) 
    {
        void* function = cmd->data[0];
        StandardFuncStd stand = (StandardFuncStd)function;
        stand();
    }

    // Call any void function (__fastcall)
    if (cmd->operation == 4) 
    {
        void* function = cmd->data[0];
        StandardFuncFast stand = (StandardFuncFast)function;
        stand();
    }

    // Call driver entry
    if (cmd->operation == 5) 
    {
        void* function = cmd->data[0];
        DriverEntry entry = (DriverEntry)function;
        
        // gcc compiles long as 8 byte
        // msvc compiles long as 4 byte
        // we are gonna use int
        // you can't even imagine how long I was fking 
        // with this
        int status = entry(0, 0);
        *(int*)cmd->data[1] = status;
    }

    // Invalid command
    return EFI_UNSUPPORTED;
}

// Hooked EFI function SetVariable()
// Can be called from Windows with NtSetSystemEnvironmentValueEx
EFI_STATUS
EFIAPI
HookedSetVariable(
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    IN UINT32 Attributes,
    IN UINTN DataSize,
    IN VOID *Data
	  ) 
{
    // Use our hook only after we are in virtual address-space
    if (Virtual && Runtime) 
    {       
        // Check of input is not null
        if (VariableName != NULL && VariableName[0] != CHAR_NULL && VendorGuid != NULL) 
        {                     
            // Check if variable name is same as our declared one
            // this is used to check if call is really from our program
            // running in the OS (client)
            if (StrnCmp(VariableName, VARIABLE_NAME, 
                (sizeof(VARIABLE_NAME) / sizeof(CHAR16)) - 1) == 0) 
            {              
                if (DataSize == 0 && Data == NULL)
                {
                    // Skip no data
                    return EFI_SUCCESS;
                }

                // Check if the data size is correct
                if (DataSize == sizeof(MemoryCommand)) 
                {
                    // We did it!
                    // Now we can call the magic function
                    return RunCommand((MemoryCommand*)Data);
                }
            }
        }
    }
    
    // Call the original SetVariable() function
    return oSetVariable(VariableName, VendorGuid, Attributes, DataSize, Data);
}

// Event callback when SetVitualAddressMap() is called by OS
VOID
EFIAPI
SetVirtualAddressMapEvent(
    IN EFI_EVENT Event,
    IN VOID* Context
    )
{  
    // Convert orignal SetVariable address
    RT->ConvertPointer(0, &oSetVariable);
    
    // Convert all other addresses
    RT->ConvertPointer(0, &oGetTime);
    RT->ConvertPointer(0, &oSetTime);
    RT->ConvertPointer(0, &oGetWakeupTime);
    RT->ConvertPointer(0, &oSetWakeupTime);
    RT->ConvertPointer(0, &oSetVirtualAddressMap);
    RT->ConvertPointer(0, &oConvertPointer);
    RT->ConvertPointer(0, &oGetVariable);
    RT->ConvertPointer(0, &oGetNextVariableName);
    //RT->ConvertPointer(0, &oSetVariable);
    RT->ConvertPointer(0, &oGetNextHighMonotonicCount);
    RT->ConvertPointer(0, &oResetSystem);
    RT->ConvertPointer(0, &oUpdateCapsule);
    RT->ConvertPointer(0, &oQueryCapsuleCapabilities);
    RT->ConvertPointer(0, &oQueryVariableInfo);
    
    // Convert runtime services pointer
    RtLibEnableVirtualMappings();

    // Null and close the event so it does not get called again
    NotifyEvent = NULL;

    // We are now working in virtual address-space
    Virtual = TRUE;
}

// Event callback after boot process is started
VOID
EFIAPI
ExitBootServicesEvent(
	IN EFI_EVENT Event,
	IN VOID* Context
	)
{
    // This event is called only once so close it
    BS->CloseEvent(ExitEvent);
	ExitEvent = NULL;

    // Boot services are now not avaible
    BS = NULL;
	
    // We are booting the OS now
    Runtime = TRUE;

    // Print some text so we know it works (300iq)
    ST->ConOut->SetAttribute(ST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);
	ST->ConOut->ClearScreen(ST->ConOut);
    Print(L"Driver seems to be working as expected! Windows is booting now...\n");
}

// Replaces service table pointer with desired one
// returns original
VOID*
SetServicePointer(
    IN OUT EFI_TABLE_HEADER *ServiceTableHeader,
    IN OUT VOID **ServiceTableFunction,
    IN VOID *NewFunction
    )
{
    // We don't want to fuck up the system
    if (ServiceTableFunction == NULL || NewFunction == NULL || *ServiceTableFunction == NULL)
        return NULL;

    // Make sure boot services pointers are not null
    ASSERT(BS != NULL);
    ASSERT(BS->CalculateCrc32 != NULL);

    // Raise task priority level
    CONST EFI_TPL Tpl = BS->RaiseTPL(TPL_HIGH_LEVEL);

    // Swap the pointers
    // GNU-EFI and InterlockedCompareExchangePointer 
    // are not friends
    VOID* OriginalFunction = *ServiceTableFunction;
    *ServiceTableFunction = NewFunction;

    // Change the table CRC32 signature
    ServiceTableHeader->CRC32 = 0;
    BS->CalculateCrc32((UINT8*)ServiceTableHeader, ServiceTableHeader->HeaderSize, &ServiceTableHeader->CRC32);

    // Restore task priority level
    BS->RestoreTPL(Tpl);

    return OriginalFunction;
}

// EFI driver unload routine
static
EFI_STATUS
EFI_FUNCTION
efi_unload(IN EFI_HANDLE ImageHandle)
{
    // We don't want our driver to be unloaded 
    // until complete reboot
    return EFI_ACCESS_DENIED;
}

// EFI entry point
EFI_STATUS
efi_main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) 
{
    // Initialize internal GNU-EFI functions
    InitializeLib(ImageHandle, SystemTable);

    // Get handle to this image
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_STATUS status = BS->OpenProtocol(ImageHandle, &LoadedImageProtocol,
                                        (void**)&LoadedImage, ImageHandle,
                                        NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    
    // Return if protocol failed to open
    if (EFI_ERROR(status)) 
    {
        Print(L"Can't open protocol: %d\n", status);
        return status;
    }

    // Install our protocol interface
    // This is needed to keep our driver loaded
    DummyProtocalData dummy = { 0 };
    status = LibInstallProtocolInterfaces(
      &ImageHandle, &ProtocolGuid,
      &dummy, NULL);

    // Return if interface failed to register
    if (EFI_ERROR(status)) 
    {
        Print(L"Can't register interface: %d\n", status);
        return status;
    }

    // Set our image unload routine
    LoadedImage->Unload = (EFI_IMAGE_UNLOAD)efi_unload;

    // Create global event for VirtualAddressMap
    status = BS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                                TPL_NOTIFY,
                                SetVirtualAddressMapEvent,
                                NULL,
                                VirtualGuid,
                                &NotifyEvent);

    // Return if event create failed
    if (EFI_ERROR(status)) 
    {
        Print(L"Can't create event (SetVirtualAddressMapEvent): %d\n", status);
        return status;
    }

    // Create global event for ExitBootServices
    status = BS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                                TPL_NOTIFY,
                                ExitBootServicesEvent,
                                NULL,
                                ExitGuid,
                                &ExitEvent);

    // Return if event create failed (yet again)
    if (EFI_ERROR(status)) 
    {
        Print(L"Can't create event (ExitBootServicesEvent): %d\n", status);
        return status;
    }

    // Hook SetVariable (should not fail)
    oSetVariable = (EFI_SET_VARIABLE)SetServicePointer(&RT->Hdr, (VOID**)&RT->SetVariable, (VOID**)&HookedSetVariable);

    // Hook all the other runtime services functions
    oGetTime = (EFI_GET_TIME)SetServicePointer(&RT->Hdr, (VOID**)&RT->GetTime, (VOID**)&HookedGetTime);
    oSetTime = (EFI_SET_TIME)SetServicePointer(&RT->Hdr, (VOID**)&RT->SetTime, (VOID**)&HookedSetTime);
    oGetWakeupTime = (EFI_SET_TIME)SetServicePointer(&RT->Hdr, (VOID**)&RT->GetWakeupTime, (VOID**)&HookedGetWakeupTime);
    oSetWakeupTime = (EFI_SET_WAKEUP_TIME)SetServicePointer(&RT->Hdr, (VOID**)&RT->SetWakeupTime, (VOID**)&HookedSetWakeupTime);
    oSetVirtualAddressMap = (EFI_SET_VIRTUAL_ADDRESS_MAP)SetServicePointer(&RT->Hdr, (VOID**)&RT->SetVirtualAddressMap, (VOID**)&HookedSetVirtualAddressMap);
    oConvertPointer = (EFI_CONVERT_POINTER)SetServicePointer(&RT->Hdr, (VOID**)&RT->ConvertPointer, (VOID**)&HookedConvertPointer);
    oGetVariable = (EFI_GET_VARIABLE)SetServicePointer(&RT->Hdr, (VOID**)&RT->GetVariable, (VOID**)&HookedGetVariable);
    oGetNextVariableName = (EFI_GET_NEXT_VARIABLE_NAME)SetServicePointer(&RT->Hdr, (VOID**)&RT->GetNextVariableName, (VOID**)&HookedGetNextVariableName);
    //oSetVariable = (EFI_SET_VARIABLE)SetServicePointer(&RT->Hdr, (VOID**)&RT->SetVariable, (VOID**)&HookedSetVariable);
    oGetNextHighMonotonicCount = (EFI_GET_NEXT_HIGH_MONO_COUNT)SetServicePointer(&RT->Hdr, (VOID**)&RT->GetNextHighMonotonicCount, (VOID**)&HookedGetNextHighMonotonicCount);
    oResetSystem = (EFI_RESET_SYSTEM)SetServicePointer(&RT->Hdr, (VOID**)&RT->ResetSystem, (VOID**)&HookedResetSystem);
    oUpdateCapsule = (EFI_UPDATE_CAPSULE)SetServicePointer(&RT->Hdr, (VOID**)&RT->UpdateCapsule, (VOID**)&HookedUpdateCapsule);
    oQueryCapsuleCapabilities = (EFI_QUERY_CAPSULE_CAPABILITIES)SetServicePointer(&RT->Hdr, (VOID**)&RT->QueryCapsuleCapabilities, (VOID**)&HookedQueryCapsuleCapabilities);
    oQueryVariableInfo = (EFI_QUERY_VARIABLE_INFO)SetServicePointer(&RT->Hdr, (VOID**)&RT->QueryVariableInfo, (VOID**)&HookedQueryVariableInfo);

    // Print confirmation text
    Print(L"efi-memory (build on: %a in: %a)\n", __DATE__, __TIME__);
    Print(L"https://github.com/SamuelTulach/efi-memory\n");  

    return EFI_SUCCESS;
}