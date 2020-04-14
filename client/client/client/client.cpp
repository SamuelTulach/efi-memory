/*
    Copyright (c) 2020 Samuel Tulach
    Copyright (c) 2019 z175

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <iostream>
#include <iomanip>
#include "nt.h"
#include "driver.h"
#include "utils.h"
#include "helper.h"

#define EXPLORER_EXE L"explorer.exe"

int main()
{
    std::cout << "[>] Enabling SE_SYSTEM_ENVIRONMENT_PRIVILEGE..." << std::endl;
    bool status = Driver::Init();
    if (!status) 
    {
        std::cout << "[-] Failed to enable privilege" << std::endl;
        return -1;
    }

    std::cout << "[>] Testing driver..." << std::endl;
    status = Driver::Test();
    if (!status)
    {
        std::cout << "[-] Driver test failed" << std::endl;
        return -1;
    }

    std::cout << "[>] Getting current process PEPROCESS..." << std::endl;
    uintptr_t current = Helper::GetCurrentProcessKrnl();
    if (!current) 
    {
        std::cout << "[-] Failed to get current process" << std::endl;
        return -1;
    }
    
    std::cout << "[>] Getting explorer.exe PEPROCESS..." << std::endl;
    int pid = Utils::Find(EXPLORER_EXE);
    if (!pid) 
    {
        std::cout << "[-] Failed to find explorer.exe pid" << std::endl;
        return -1;
    }
    
    uintptr_t explorer = 0;
    Helper::LookupProcess(pid, &explorer);
    if (!explorer) 
    {
        std::cout << "[-] Failed to get explorer.exe PEPROCESS" << std::endl;
        return -1;
    }

    std::cout << "[>] Reading DOS header..." << std::endl;
    uintptr_t baseaddress = Utils::GetModuleBaseAddress(pid, EXPLORER_EXE);
    if (!baseaddress) 
    {
        std::cout << "[-] Failed to get explorer.exe base address" << std::endl;
        return -1;
    }
    
    IMAGE_DOS_HEADER header = { 0 };
    SIZE_T retsize = 0;
    NTSTATUS copystatus = Helper::CopyVirtualMemory(explorer, baseaddress, current, (uintptr_t)&header, sizeof(IMAGE_DOS_HEADER), 0, &retsize);

    std::cout << "[+] Test read:" << std::endl;
    std::cout << "\tStatus: " << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << copystatus << std::nouppercase << std::dec << std::endl;
    std::cout << "\tDOS magic: " << header.e_magic << std::endl;
    std::cout << "\tNT offset: " << header.e_lfanew << std::endl;
}
