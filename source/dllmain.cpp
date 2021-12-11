#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include "mono_interaction.h"
#include "utilities.h"

HMODULE self;

__declspec(naked) void helm_manager_fixed_update_hook()
{
    self = self;
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

    void* helm_manager_fixed_update = FindCodeAddress(helm_manager, "FixedUpdate");

    if (!helm_manager_fixed_update)
        return;

    void* fixed_update_hook_point = (void*)((size_t)helm_manager_fixed_update + 0x43);

    DWORD old_protection;
    VirtualProtect(fixed_update_hook_point, 4096, PAGE_EXECUTE_READWRITE, &old_protection);

    uint32_t target_address = (uint32_t)&helm_manager_fixed_update_hook;
    unsigned char fixed_update_hook[] =
    {
        0x68,                                       // push 4-byte imm
        (unsigned char)(target_address >>  0),      // target
        (unsigned char)(target_address >>  8),      // target
        (unsigned char)(target_address >> 16),      // target
        (unsigned char)(target_address >> 24),      // target
        0xc3,                                       // ret
        0x90,                                       // nop
    };

    memcpy(fixed_update_hook_point, fixed_update_hook, sizeof(fixed_update_hook));

    DWORD dummy_protection;
    VirtualProtect(fixed_update_hook_point, 4096, old_protection, &dummy_protection);

    while (true)
        Sleep(500);

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

