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
#define PRINT_HEX(x) std::hex << std::setw(8) << std::setfill('0') << std::uppercase << x << std::nouppercase << std::dec

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
    std::cout << "[+] Current PEPROCESS 0x" << PRINT_HEX(current) << std::endl;
    
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
    std::cout << "[+] Target PEPROCESS 0x" << PRINT_HEX(explorer) << std::endl;

    std::cout << "[>] Getting process base..." << std::endl;
    uintptr_t baseaddress = Helper::GetSectionBase(explorer);
    if (!baseaddress)
    {
        std::cout << "[-] Failed to get base address" << std::endl;
        return -1;
    }
    std::cout << "[+] Explorer.exe base 0x" << PRINT_HEX(baseaddress) << std::endl;

    std::cout << "[>] Reading DOS header..." << std::endl;
    IMAGE_DOS_HEADER* header = new IMAGE_DOS_HEADER;
    SIZE_T retsize = 0;
    NTSTATUS copystatus = Helper::CopyVirtualMemory(explorer, baseaddress, current, (uintptr_t)header, sizeof(IMAGE_DOS_HEADER), 0, &retsize);

    std::cout << "[+] Test read:" << std::endl;
    std::cout << "\tStatus:     0x" << PRINT_HEX(copystatus) << std::endl;
    std::cout << "\tRead:       0x" << PRINT_HEX(retsize) << std::endl;
    std::cout << "\tDOS magic:  0x" << PRINT_HEX(header->e_magic) << std::endl;
    std::cout << "\tNT offset:  0x" << PRINT_HEX(header->e_lfanew) << std::endl;
}
