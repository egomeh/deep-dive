#pragma once
#include "pch.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>


HMODULE self;

HANDLE gWaitForThisBastardBeforeLeaving;

#define MONO_JIT_INFO_TABLE_CHUNK_SIZE 64
#define MONO_ZERO_LEN_ARRAY 1
#define HAZARD_POINTER_COUNT 3

struct MonoJitInfo;
struct MonoJitInfoTableChunk;
struct MonoDomain;

// Mono
struct _MonoJitInfoTableChunk
{
	int		       refcount;
	volatile int           num_elements;
	volatile void* last_code_end;
	/* MonoJitInfo* */void* next_tombstone;
	MonoJitInfo* volatile data[MONO_JIT_INFO_TABLE_CHUNK_SIZE];
};

struct MonoJitInfoTable
{
	MonoDomain* domain;
	int			num_chunks;
	int			num_valid;
	MonoJitInfoTableChunk* chunks[MONO_ZERO_LEN_ARRAY];
};

typedef bool gboolean;
typedef uint32_t guint32;
struct MonoMethod;
struct MonoImage;
struct MonoAotModule;
struct MonoTrampInfo;
typedef void* gpointer;
typedef int MonoJitExceptionInfo;

typedef struct {
	gpointer hazard_pointers[HAZARD_POINTER_COUNT];
} MonoThreadHazardPointers;


struct MonoJitInfo {
	/* NOTE: These first two elements (method and
	   next_jit_code_hash) must be in the same order and at the
	   same offset as in RuntimeMethod, because of the jit_code_hash
	   internal hash table in MonoDomain. */
	union {
		MonoMethod* method;
		MonoImage* image;
		MonoAotModule* aot_info;
		MonoTrampInfo* tramp_info;
	} d;
	union {
		MonoJitInfo* next_jit_code_hash;
		MonoJitInfo* next_tombstone;
	} n;
	void*    code_start;
	guint32     unwind_info;
	int         code_size;
	guint32     num_clauses : 15;
	/* Whenever the code is domain neutral or 'shared' */
	gboolean    domain_neutral : 1;
	gboolean    has_generic_jit_info : 1;
	gboolean    has_try_block_holes : 1;
	gboolean    has_arch_eh_info : 1;
	gboolean    has_thunk_info : 1;
	gboolean    has_unwind_info : 1;
	gboolean    from_aot : 1;
	gboolean    from_llvm : 1;
	gboolean    dbg_attrs_inited : 1;
	gboolean    dbg_hidden : 1;
	/* Whenever this jit info was loaded in async context */
	gboolean    async : 1;
	gboolean    dbg_step_through : 1;
	gboolean    dbg_non_user_code : 1;
	/*
	 * Whenever this jit info refers to a trampoline.
	 * d.tramp_info contains additional data in this case.
	 */
	gboolean    is_trampoline : 1;
	/* Whenever this jit info refers to an interpreter method */
	gboolean    is_interp : 1;

	/* FIXME: Embed this after the structure later*/
	gpointer    gc_info; /* Currently only used by SGen */

	gpointer    seq_points;

	MonoJitExceptionInfo clauses[MONO_ZERO_LEN_ARRAY];
	/* There is an optional MonoGenericJitInfo after the clauses */
	/* There is an optional MonoTryBlockHoleTableJitInfo after MonoGenericJitInfo clauses*/
	/* There is an optional MonoArchEHJitInfo after MonoTryBlockHoleTableJitInfo */
	/* There is an optional MonoThunkJitInfo after MonoArchEHJitInfo */
};

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

	MonoJitInfoTable* table;
	MonoJitInfo* ji;
	MonoThreadHazardPointers* hp = mono_hazard_pointer_get();
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

	mono_domain_foreach(&ForeachDomain, NULL);
	
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

