#define NOMINMAX

#include "asm_check.h"
#include "pattern_check.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <set>
#include <vector>
#include <sstream>
#include <ThemidaSDK.h>

#ifndef XRAY_H
#define XRAY_H

// clang-format off
#include "checks.h"
#endif

using namespace std;

static const DWORD READ_ACCESS_FLAGS = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

extern "C" NTSTATUS NtQueryVirtualMemory(
	HANDLE                   ProcessHandle,
	PVOID                    BaseAddress,
	DWORD MemoryInformationClass,
	PVOID                    MemoryInformation,
	SIZE_T                   MemoryInformationLength,
	PSIZE_T                  ReturnLength
);

extern "C" NTSTATUS NtReadVirtualMemory(
	HANDLE               ProcessHandle,
	PVOID                BaseAddress,
	PVOID                Buffer,
	ULONG                NumberOfBytesToRead,
	PULONG               NumberOfBytesRead
);

/*

A   = Generic obfuscator (Lx/x;)
B   = Allatori obfuscation (string decrypt method opcodes)
C   = JNI_GetCreatedJavaVMs
D   = ReflectiveLoader function opcodes
E   = Aimbot (pi + rotationYaw)
F   = Reach (func_70111_Y() + mask + ()F)
G   = Reach (AxisAlignedBB math)
H   = SelfDestruct (System.gc() + System.runFinalization())
I   = Stringer obfuscation (decrypt method opcodes)
J   = Generic obfuscator (.jar!/x/a.class)
K   = Triggerbot/Autoclicker (attackEntity)

*/

static ss::pattern patterns[] =
{

	ss::pattern("obfuscated class names", "\x01\x00\x05\x01\x00\x05\x4C\x00\x2F\x00\x3B", "xxxxxxx?x?x", 11),
	ss::pattern("invalid opcodes", "\xB6\0\0\xB6\0\0\xB6\0\0\x59\xB6\0\0\x04\x64\x0D\x0C\x57", "x??x??x??xx??xxxxx", 18),
	ss::pattern("external injection", "\x4A\x61\x76\x61\x56\x4D\x73\x00\x00", "xxxxxxxxx", 9),
	ss::pattern("invalid opcodes 2", "\x48\x8B\x04\x24\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x4C\x24\x08", "xxxxxxxxxxxxxxxxxxxxx", 21),
	ss::pattern("suspicious player math", "\x06\x40\x09\x21\xFB\x54\x44\x2D\x18\x04\x42\xB4\x00\x00\x01\x00\x0D\x66\x69\x65\x6C\x64\x5F\x37\x30\x31\x37\x37\x5F\x7A", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 30),
	//ss::pattern("suspicious access calls", "\x66\x75\x6E\x63\x5F\x37\x30\x31\x31\x31\x5F\x59\x01\x00\x03\x28\x29\x46", "xxxxxxxxxxxxxxxxxx", 18),
	ss::pattern("suspicious math", "\x37\x32\x33\x32\x31\x5F\x61\x01\x00\x27\x28\x44\x44\x44\x29\x4C\x6E\x65\x74\x2F\x6D\x69\x6E\x65\x63\x72\x61\x66\x74\x2F\x75\x74\x69\x6C\x2F\x41\x78\x69\x73\x41\x6C\x69\x67\x6E\x65\x64\x42\x42\x3B\0\0\0\0\0\0\0\0\0\0\x01\x00\x0C\x66\x75\x6E\x63\x5F\x37\x32\x33\x31\x34\x5F\x62\0\0\0\0\0\0\0\0\0\0\x01\x00\x2C\x6E\x65\x74\x2F\x6D\x69\x6E\x65\x63\x72\x61\x66\x74\x2F\x63\x6C\x69\x65\x6E\x74\x2F\x6D\x75\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx??????????xxxxxxxxxxxxxxx??????????xxxxxxxxxxxxxxxxxxxxxxxxxxx", 111),
	ss::pattern("self destruct", "\x67\x63\0\0\0\0\0\0\0\0\0\0\x01\x00\x0F\x72\x75\x6E\x46\x69\x6E\x61\x6C\x69\x7A\x61\x74\x69\x6F\x6E", "xx??????????xxxxxxxxxxxxxxxxxx", 30),
	ss::pattern("bad opcodes", "\x3A\0\xBB\0\0\x59\x2A\xB7\0\0\xB0\x03\x36\0\x01\xA7\0\0\x03\x36\0\x01\xA7", "x?x??xxx??xxx?xx??xx?xx", 23),
	ss::pattern("suspicious class file names", "j.a.r.!./.../.a...c.l.a.s.s", "x?x?x?x?x???x?x?x?x?x?x?x?x", 27),
	ss::pattern("suspicious entity calls", "\x4D\x50\x00\x00\x00\x01\x00\x0C\x66\x75\x6E\x63\x5F\x37\x38\x37\x36\x34", "xxxxxxxxxxxxxxxxxx", 18)

};
void ss::run_scan(const HANDLE process, GenericCheck *generic_check)
{
	auto finished = false;

	set<int> checks_failed;

	MEMORY_BASIC_INFORMATION memory_info;
	for (char *address = nullptr; NtQueryVirtualMemory(process, address, 0, &memory_info, sizeof(MEMORY_BASIC_INFORMATION), nullptr) == 0; address += memory_info.RegionSize)
	{
		if (memory_info.State == MEM_COMMIT && memory_info.Protect & READ_ACCESS_FLAGS)
		{
			auto* buffer = new unsigned char[memory_info.RegionSize];
			ULONG bytes_read;

			if (NtReadVirtualMemory(process, address, buffer, memory_info.RegionSize, &bytes_read) == 0)
			{
				auto index = 0;

				for (auto pattern : patterns)
				{
					if (pattern.find_pattern(buffer, bytes_read))
						checks_failed.insert(index);

					index++;
				}
			}

			delete[] buffer;
		}
	}

	finished = true;

	//stringstream ss;

	if (checks_failed.empty())
	{
		generic_check->status = CheckStatus::kSuccess;
	}
	else
	{

		auto iter = checks_failed.begin();

		while (true)
		{	
			//ss << patterns[*iter].name;

			if (++iter == checks_failed.end()) {
				generic_check->status = CheckStatus::kFailed;
				generic_check->message = "Illegal modifications found!";
				break;
			}
			else {
				//ss << ", ";
			}
		}
	}
	
}
