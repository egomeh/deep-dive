#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include "mono_interaction.h"
#include "utilities.h"
#include "memory.h"

HMODULE self;
HANDLE done_gone_exit_evnet;

uint32_t return_address;
uint32_t helm_manager;

uint32_t set_fixed_depth_address;

bool make_set_fixed_depth_call = true;
float set_fiex_depth_parameter = 400.f;

__declspec(naked) void helm_manager_fixed_update_hook()
{
    // Do the original work from the hook point
    __asm
    {
        pushad
        mov dword ptr[ebp - 0x80], 0
    }

    // Get the helm manager address
    __asm
    {
        lea eax, helm_manager
        mov ecx, [ebp + 0x8]
        mov[eax], ecx
    }

    if (make_set_fixed_depth_call)
    {
        // Make the call to SetFixedDepth
        __asm
        {
            // prepare fixed depth call

            // push depth parameter
            lea eax, set_fiex_depth_parameter
            push [eax]

            // push helm manager
            mov eax, helm_manager
            push eax

            // call
            call set_fixed_depth_address
            add esp, 0x8
        };

        make_set_fixed_depth_call = false;
    }

    // For now, we just dart after calling this once
    SetEvent(done_gone_exit_evnet);

    // TIL: naked functions don't have a ret instructio n
    __asm
    {
        popad
        push return_address
        ret
    };
}

void Entry()
{
    ON_EXIT
    {
        Sleep(500);
        FreeLibraryAndExitThread(self, 0);
    };

    if (!InitMonoInteraction())
        return;

    void* image = FindMonoImage("Assembly-CSharp");
    
    if (!image)
        return;

    void* helm_manager = FindClassFromImage(image, "HelmManager");

    if (!helm_manager)
        return;

    void* helm_manager_set_fixed_depth = FindCodeAddress(helm_manager, "SetFixedDepth");

    if (!helm_manager_set_fixed_depth)
        return;

    set_fixed_depth_address = (uint32_t)helm_manager_set_fixed_depth;

    void* helm_manager_fixed_update = FindCodeAddress(helm_manager, "FixedUpdate");

    if (!helm_manager_fixed_update)
        return;

    done_gone_exit_evnet = CreateEventA(NULL, FALSE, FALSE, "DoneGoneExitEvent");

    if (!done_gone_exit_evnet)
        return;

    void* fixed_update_hook_point = (void*)((size_t)helm_manager_fixed_update + 0x43);
    return_address = (uint32_t)fixed_update_hook_point + 7;
    uint32_t target_address = (uint32_t)&helm_manager_fixed_update_hook;

    MemoryReplacement helm_manager_fixed_update_replacement;
    helm_manager_fixed_update_replacement.SetMemory(
        {
            0x68,                                       // push 4-byte imm
            FOUR_BYTES(target_address),                 // target address
            0xc3,                                       // ret
            0x90,                                       // nop
        }
    );
    helm_manager_fixed_update_replacement.Emplace(fixed_update_hook_point);

    WaitForSingleObject(done_gone_exit_evnet, -1);

    return;
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

