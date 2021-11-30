#pragma once
#include "pch.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

HMODULE self;

HANDLE gWaitForThisBastardBeforeLeaving;

//
// sanity
//
typedef unsigned __int64 QWORD, * PQWORD;

//
// mono declarations
//
DWORD(*mono_security_get_mode)();
VOID(*mono_security_set_mode)(DWORD mode);

// Function to iterate over domains, though what the heck is a domain...?
void(*mono_domain_foreach)(void* func, void* user_data);
char*(*mono_pmip)(void* address);

VOID(*mono_jit_info_table_foreach_internal)(void*, void*, void*);

//PVOID(*mono_domain_get)();
//PVOID(*mono_domain_assembly_open)(PVOID domain, PCHAR file);
//PVOID(*mono_assembly_get_image)(PVOID assembly);
//PVOID(*mono_class_from_name)(PVOID image, PCHAR Mnamespace, PCHAR name);
//PVOID(*mono_class_get_method_from_name)(PVOID Mclass, PCHAR name, DWORD param_count);
//PVOID(*mono_runtime_invoke)(PVOID method, PVOID instance, PVOID* params, PVOID exc);

//
// data for our payload
//
DWORD dwReturn;
DWORD dwSecurity;

struct MemoryRegion
{
	uint32_t Base;
	uint32_t End;
};

std::vector<void*> MonoDomains;
std::vector<MemoryRegion> ExecutableRegions;

void ForeachDomain(void* MonoDomain, void* user_data)
{
	MonoDomains.push_back(MonoDomain);
}

char* methodName;

__declspec(naked) int MonoInject() {
	__asm {
		// make use of the call gate
		push dwReturn;
		// push the execution state
		pushfd;
		pushad;
	};

	// goodbye security
	//mono_security_set_mode(NULL);

	//mono_domain_foreach(&ForeachDomain, NULL);
	
	methodName = mono_pmip((void*)0x28107178);

	SetEvent(gWaitForThisBastardBeforeLeaving);

	__asm {
		// restore the execution state
		popad;
		popfd;
		// go about original game 'bidness
		ret;
	}
}

bool IsExecutable(DWORD protection)
{
	if (protection == PAGE_EXECUTE)
		return true;

	if (protection == PAGE_EXECUTE_READ)
		return true;

	if (protection == PAGE_EXECUTE_READWRITE)
		return true;

	if (protection == PAGE_EXECUTE_WRITECOPY)
		return true;

	return false;
}

void Entry()
{
	QWORD time;
	DWORD id, pid;
	HMODULE hMono;
	HANDLE hSs, hThread;
	THREADENTRY32 t;
	t.dwSize = sizeof(THREADENTRY32);
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;
	CONTEXT context = { CONTEXT_CONTROL };

	// poll for mono, but we need not really poll, we just need to get it
	while ((hMono = GetModuleHandleA("mono.dll")) == NULL) Sleep(10);

	mono_security_set_mode = (VOID(__cdecl*)(DWORD))GetProcAddress(hMono, "mono_security_set_mode");
	mono_domain_foreach = (void(__cdecl*)(void* func, void* user_data))GetProcAddress(hMono, "mono_domain_foreach");
	mono_pmip = (char*(__cdecl*)(void* address))GetProcAddress(hMono, "mono_pmip");


	mono_jit_info_table_foreach_internal = (void(__cdecl*)(void*, void*, void*))GetProcAddress(hMono, "mono_jit_info_table_foreach_internal");
	//MONO_PROC(mono_domain_get);
	//MONO_PROC(mono_domain_assembly_open);
	//MONO_PROC(mono_assembly_get_image);
	//MONO_PROC(mono_class_from_name);
	//MONO_PROC(mono_class_get_method_from_name);
	//MONO_PROC(mono_runtime_invoke);

	gWaitForThisBastardBeforeLeaving = CreateEventA(NULL, TRUE, FALSE, "BastardToWaitForEvent");
	if (!gWaitForThisBastardBeforeLeaving)
		FreeLibraryAndExitThread(self, 0);

	// mono executes with thread local data, get the main thread.
	id = NULL;
	pid = GetCurrentProcessId();
	hSs = CreateToolhelp32Snapshot(TH32CS_SNAPALL, pid);
	if (hSs) {

		// set time to max value so we can find the oldest thread
		time = 0xFFFFFFFFFFFFFFFF;
		if (Thread32First(hSs, &t)) {
			do {
				if (t.th32OwnerProcessID == pid) {
					hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, t.th32ThreadID);
					if (hThread) {
						if (GetThreadTimes(hThread, &CreationTime, &ExitTime, &KernelTime, &UserTime)) {
							// nasty casting lol
							if (time > *(PQWORD)&CreationTime) {
								time = *(PQWORD)&CreationTime;
								id = t.th32ThreadID;
							}
						}
						CloseHandle(hThread);
					}
				}
			} while (Thread32Next(hSs, &t));
		}
	}


	if (id) {
		// hijack the main thread and have it inject our c# code
		hThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, id);
		if (hThread) {
			SuspendThread(hThread);
			if (GetThreadContext(hThread, &context)) {
				dwReturn = context.Eip;
				context.Eip = (DWORD)&MonoInject;
				SetThreadContext(hThread, &context);
			}
			ResumeThread(hThread);
			CloseHandle(hThread);
		}
	}

	DWORD result = WaitForSingleObject(gWaitForThisBastardBeforeLeaving, INFINITE);

	switch (result)
	{
	case WAIT_OBJECT_0:
		break;
	default:
		break;
	}

	Sleep(500);

	CloseHandle(gWaitForThisBastardBeforeLeaving);
    FreeLibraryAndExitThread(self, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        self = hModule;
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&Entry, NULL, NULL, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

