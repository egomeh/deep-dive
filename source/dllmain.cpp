#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include "mono_interaction.h"
#include "utilities.h"

HMODULE self;
HANDLE done_gone_exit_evnet;

uint32_t return_address;
uint32_t helm_manager;

uint32_t set_fixed_depth_address;

__declspec(naked) void helm_manager_fixed_update_hook()
{
    __asm
    {
        // Do the original work from the hook point
        mov dword ptr[ebp - 0x80], 0

        // Get the helm manager address
        lea eax, helm_manager
        mov ecx, [ebp + 0x8]
        mov [eax], ecx

        // prepare fixed depth call
        push 0x44000000
        mov eax, helm_manager
        push eax
        call set_fixed_depth_address
        add esp, 0x8
    };

    SetEvent(done_gone_exit_evnet);

    // TIL: naked functions don't have a ret instructio n
    __asm
    {
        push return_address
        ret
    };
}

void Entry()
{
    ON_EXIT
    {
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

    void* fixed_update_hook_point = (void*)((size_t)helm_manager_fixed_update + 0x43);
    return_address = (uint32_t)fixed_update_hook_point + 7;

    DWORD old_protection;
    VirtualProtect(fixed_update_hook_point, 0x1000, PAGE_EXECUTE_READWRITE, &old_protection);

    uint32_t target_address = (uint32_t)&helm_manager_fixed_update_hook;
    unsigned char fixed_update_hook[] =
    {
        0x68,                                       // push 4-byte imm
        FOUR_BYTES(target_address),                 // target address
        0xc3,                                       // ret
        0x90,                                       // nop
    };


    unsigned char old_data[sizeof(fixed_update_hook)];
    memcpy(old_data, fixed_update_hook_point, sizeof(fixed_update_hook));
    memcpy(fixed_update_hook_point, fixed_update_hook, sizeof(fixed_update_hook));

    WaitForSingleObject(done_gone_exit_evnet, -1);

    memcpy(fixed_update_hook_point, old_data, sizeof(fixed_update_hook));

    Sleep(500);

    DWORD dummy_protection;
    VirtualProtect(fixed_update_hook_point, 0x1000, old_protection, &dummy_protection);

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

