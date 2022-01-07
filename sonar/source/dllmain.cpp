#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <TlHelp32.h>
#include <shlobj.h>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "mono_interaction.h"
#include "utilities.h"
#include "memory.h"
#include "comms.h"

HMODULE self;

uint32_t return_address;
uint32_t helm_manager = 0;

uint32_t set_fixed_depth_address;
bool make_set_fixed_depth_call = false;
float set_fixed_depth_parameter = 0.f;

uint32_t set_direct_telegraph_address;
bool make_set_direct_telegraph_call = false;
int set_direct_telegraph_parameter = 0;

uint32_t drop_noise_maker_address;
bool make_drop_noise_maker_call = false;

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
        mov [eax], ecx
    }

    if (make_set_fixed_depth_call)
    {
        // Make the call to SetFixedDepth
        __asm
        {
            // prepare fixed depth call

            // push depth parameter
            lea eax, set_fixed_depth_parameter
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

    if (make_set_direct_telegraph_call)
    {
        // Make the call to SetDirectTelegraph
        __asm
        {
            // prepare fixed depth call

            // push depth parameter
            lea eax, set_direct_telegraph_parameter
            push[eax]

            // push helm manager
            mov eax, helm_manager
            push eax

            // call
            call set_direct_telegraph_address
            add esp, 0x8
        };

        make_set_direct_telegraph_call = false;
    }

    if (make_drop_noise_maker_call)
    {
        __asm
        {
            mov eax, helm_manager
            add eax, 0xC            // player functions address

            push eax
            call drop_noise_maker_address
            add esp, 0x4
        }
    }

    // TIL: naked functions don't have a ret instruction
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

    if (!connect_to_buoy())
        return;

    if (!InitMonoInteraction())
        return;

    void* image = FindMonoImage("Assembly-CSharp");
    
    if (!image)
        return;

    void* helm_manager_class = FindClassFromImage(image, "HelmManager");

    if (!helm_manager_class)
        return;

    void* helm_manager_set_fixed_depth = FindCodeAddress(helm_manager_class, "SetFixedDepth");

    if (!helm_manager_set_fixed_depth)
        return;

    set_fixed_depth_address = (uint32_t)helm_manager_set_fixed_depth;

    void* helm_manager_set_direct_telegraph = FindCodeAddress(helm_manager_class, "SetDirectTelegraph");

    if (!helm_manager_set_direct_telegraph)
        return;

    set_direct_telegraph_address = (uint32_t)helm_manager_set_direct_telegraph;

    void* helm_manager_fixed_update = FindCodeAddress(helm_manager_class, "FixedUpdate");

    if (!helm_manager_fixed_update)
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

    void* player_functions_class = FindClassFromImage(image, "PlayerFunctions");

    void* player_functions_drop_noise_maker = FindCodeAddress(player_functions_class, "DropNoisemaker");

    if (!player_functions_drop_noise_maker)
        return;

    drop_noise_maker_address = (uint32_t)player_functions_drop_noise_maker;

    std::vector<uint8_t> data_from_buoy;
    while (read_from_buoy(data_from_buoy))
    {
        if (data_from_buoy.size() == 0)
            break;

        int command_type = *((int*)data_from_buoy.data());
        data_from_buoy.erase(data_from_buoy.begin(), data_from_buoy.begin() + 4);

        if (command_type == 2) // make depth [number] feet
        {
            float depth = (float)*((int*)data_from_buoy.data());
            set_fixed_depth_parameter = depth;
            make_set_fixed_depth_call = true;
        }

        if (command_type == 3) // [setting] ahead / speed command
        {
            int speed = *(int*)data_from_buoy.data();
            set_direct_telegraph_parameter = speed;
            make_set_direct_telegraph_call = true;
        }

        if (command_type == 4) // Rudder
        {
            int angle = *(int*)data_from_buoy.data();
            if (helm_manager == 0)
                continue;

            if (angle < -30)
                angle = -30;

            if (angle > 30)
                angle = 30;

            float value_to_write_to_memory = (float)angle;
            value_to_write_to_memory = value_to_write_to_memory * .1f;
            __asm
            {
                mov eax, helm_manager
                mov eax, [eax + 0x0C]
                mov eax, [eax + 0x24]
                mov eax, [eax + 0x14]
                add eax, 0x80
                lea ecx, value_to_write_to_memory
                mov ecx, [ecx]
                mov [eax], ecx
            }
        }

        if (command_type == 5) // Dive planes
        {
            int angle = *(int*)data_from_buoy.data();
            if (helm_manager == 0)
                continue;

            if (angle < -30)
                angle = -30;

            if (angle > 30)
                angle = 30;

            float value_to_write_to_memory = (float)angle;
            value_to_write_to_memory = value_to_write_to_memory * .1f;
            __asm
            {
                mov eax, helm_manager
                mov eax, [eax + 0x0C]
                mov eax, [eax + 0x24]
                mov eax, [eax + 0x14]
                add eax, 0x88
                lea ecx, value_to_write_to_memory
                mov ecx, [ecx]
                mov[eax], ecx
            }
        }

        if (command_type == 6) // Drop noise maker
        {
            make_drop_noise_maker_call = true;
        }
    }

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

