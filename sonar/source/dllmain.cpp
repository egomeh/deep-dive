#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <TlHelp32.h>
#include <shlobj.h>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cmath>

#include "mono_interaction.h"
#include "utilities.h"
#include "memory.h"
#include "comms.h"
#include "hook.h"

HMODULE self;

uint32_t return_address;
uint32_t return_address2;
uint32_t return_address3;

// I really want to find a better still reliable way.
int32_t snooped_ebp;
int32_t snooped_eax;

__declspec(naked) void helm_manager_fixed_update_hook()
{
    // save state and do original work from hooked code
    __asm
    {
        pushad
        mov dword ptr[ebp - 0x80], 0
    }

    __asm mov snooped_ebp, ebp;
    __asm mov snooped_eax, eax;
    HookManager::Get().SetEbp(snooped_ebp);
    HookManager::Get().SetEax(snooped_eax);
    HookManager::Get().ServiceHook(HookedFunction::HelmManagerFixedUpdate);

    // TIL: naked functions don't have a ret instruction
    __asm
    {
        popad
        push return_address
        ret
    };
}

__declspec(naked) void weapon_source_set_waypoint_hook()
{
    // save state and do original work from hooked code
    __asm
    {
        pushad
        mov ecx, [ebp - 0x40]
        mov[eax], ecx
        mov ecx, [ebp - 0x3C]
        mov[eax + 0x04], ecx
        mov ecx, [ebp - 0x38]
        mov[eax + 0x08], ecx
    }

    __asm mov snooped_ebp, ebp;
    __asm mov snooped_eax, eax;
    HookManager::Get().SetEbp(snooped_ebp);
    HookManager::Get().SetEax(snooped_eax);
    HookManager::Get().ServiceHook(HookedFunction::WeaponSourceSetWaypoint);

    // TIL: naked functions don't have a ret instruction
    __asm
    {
        popad
        push return_address2
        ret
    };
}

__declspec(naked) void translate_ship_forward_hook()
{
    // save state and do original work from hooked code
    __asm
    {
        pushad
        fld dword ptr [ebp - 0x248]
        fld dword ptr [edi - 0x168]
    }

    __asm mov snooped_ebp, ebp;
    __asm mov snooped_eax, eax;
    HookManager::Get().SetEbp(snooped_ebp);
    HookManager::Get().SetEax(snooped_eax);
    HookManager::Get().ServiceHook(HookedFunction::TranslateShipForward);

    // TIL: naked functions don't have a ret instruction
    __asm
    {
        popad
        push return_address3
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

    RegiterFunction("Assembly-CSharp", "HelmManager", "SetFixedDepth");
    RegiterFunction("Assembly-CSharp", "HelmManager", "SetDirectTelegraph");
    RegiterFunction("Assembly-CSharp", "HelmManager", "CancelAutoDiving");
    RegiterFunction("Assembly-CSharp", "HelmManager", "CancelAutoTurning");
    RegiterFunction("Assembly-CSharp", "HelmManager", "FixedUpdate");
    
    RegiterFunction("Assembly-CSharp", "PlayerFunctions", "DropNoisemaker");
    RegiterFunction("Assembly-CSharp", "PlayerFunctions", "ClickOnTube");

    RegiterFunction("Assembly-CSharp", "WeaponSource", "FireTube");
    RegiterFunction("Assembly-CSharp", "WeaponSource", "SetWeaponWaypointData");

    RegiterFunction("Assembly-CSharp", "VesselMovement", "TranslateShipForward");

    void* fixed_update_hook_point = (void*)((size_t)GetFunctionAddress("Assembly-CSharp", "HelmManager", "FixedUpdate") +0x43);
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

    void* set_weapon_waypoint_hook_point = (void*)((int)GetFunctionAddress("Assembly-CSharp", "WeaponSource", "SetWeaponWaypointData") + 0x173);
    return_address2 = (uint32_t)set_weapon_waypoint_hook_point + 7;
    uint32_t target_address2 = (uint32_t)&weapon_source_set_waypoint_hook;

    MemoryReplacement weapon_source_set_waypoint_data_replacement;
    weapon_source_set_waypoint_data_replacement.SetMemory(
        {
            0x68,                                       // push 4-byte imm
            FOUR_BYTES(target_address2),                // target address
            0xc3,                                       // ret
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
        }
    );
    weapon_source_set_waypoint_data_replacement.Emplace(set_weapon_waypoint_hook_point);

    uint32_t vessel_movement_translate_forward_address = (uint32_t)GetFunctionAddress("Assembly-CSharp", "VesselMovement", "TranslateShipForward");

    void* vessel_movement_translate_hook_point = (void*)(vessel_movement_translate_forward_address + 0x1ba);
    return_address3 = (uint32_t)vessel_movement_translate_hook_point + 7;
    uint32_t target_address3 = (uint32_t)&translate_ship_forward_hook;

    MemoryReplacement vessel_movement_replacement;
    vessel_movement_replacement.SetMemory(
        {
            0x68,                                       // push 4-byte imm
            FOUR_BYTES(target_address3),                // target address
            0xc3,                                       // ret
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
            0x90,                                       // nop
        }
    );
    vessel_movement_replacement.Emplace(vessel_movement_translate_hook_point);

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

            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                void* helm_manager = (void*)*(int*)(hook_data.ebp + 0x8);
                ((void(*)(void*, float))GetFunctionAddress("Assembly-CSharp", "HelmManager", "SetFixedDepth"))(helm_manager, depth);
                return true;
            });

            //set_fixed_depth_parameter = depth;
            //make_set_fixed_depth_call = true;
        }

        if (command_type == 3) // [setting] ahead / speed command
        {
            int speed = *(int*)data_from_buoy.data();

            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                void* helm_manager = (void*)*(int*)(hook_data.ebp + 0x8);
                ((void(*)(void*, int))GetFunctionAddress("Assembly-CSharp", "HelmManager", "SetDirectTelegraph"))(helm_manager, speed);
                return true;
            });

            // set_direct_telegraph_parameter = speed;
            // make_set_direct_telegraph_call = true;
        }

        if (command_type == 4) // Rudder
        {
            int angle = *(int*)data_from_buoy.data();

            uint32_t helm_manager = 0;
            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                helm_manager = (uint32_t)*(int*)(hook_data.ebp + 0x8);
                ((void(*)(uint32_t))GetFunctionAddress("Assembly-CSharp", "HelmManager", "CancelAutoTurning"))(helm_manager);
                return true;
            });

            if (helm_manager == 0)
                continue;

            if (angle < -30)
                angle = -30;

            if (angle > 30)
                angle = 30;

            float value_to_write_to_memory = (float)angle * .1f;
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
            
            uint32_t helm_manager = 0;
            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                helm_manager = (uint32_t) * (int*)(hook_data.ebp + 0x8);
                ((void(*)(uint32_t))GetFunctionAddress("Assembly-CSharp", "HelmManager", "CancelAutoDiving"))(helm_manager);
                return true;
            });

            if (angle < -30)
                angle = -30;

            if (angle > 30)
                angle = 30;

            float value_to_write_to_memory = angle * .1f;
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
            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                int helm_manager = (int)*(int*)(hook_data.ebp + 0x8);
                void* player_functions = (void*)*(int*)(helm_manager + 0xC);
                ((void(*)(void*))GetFunctionAddress("Assembly-CSharp", "PlayerFunctions", "DropNoisemaker"))(player_functions);
                return true;
            });
        }

        if (command_type == 7) // Set course
        {
            uint32_t helm_manager = 0;

            float bearing = (float)*((uint32_t*)data_from_buoy.data());

            if (bearing > 360.f)
                bearing = 360.f;

            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                helm_manager = *(uint32_t*)(hook_data.ebp + 0x8);
                return true;
            });

            unsigned char* auto_turning = (unsigned char*)(helm_manager + 0x5D);
            float* wanted_course = (float*)(helm_manager + 0x70);

            *auto_turning = 1;
            *wanted_course = bearing;
        }

        if (command_type == 8) // Shoot
        {
            int bearing = *(int*)data_from_buoy.data();
            float distance = *((float*)data_from_buoy.data() + 1);
            int tube = *((int*)data_from_buoy.data() + 2);
            --tube; // Tubes are zero-indexed

            int helm_manager = 0;
            int player_functions = 0;
            int player_vessel = 0;
            int vessel_movement = 0;
            int weapon_source = 0;

            // Get all the `this` pointers
            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                helm_manager = (int)*(int*)(hook_data.ebp + 0x8);
                player_functions = (int)*(int*)(helm_manager + 0xC);
                player_vessel = (int)*(int*)(player_functions + 0x24);
                vessel_movement = (int)*(int*)(player_vessel + 0x14);
                weapon_source = (int)*(int*)(vessel_movement + 0x10);
                return true;
            });

            float ship_x = 0.0f;
            float ship_y = 0.0f;

            HookManager::Get().ExecuteInHookSync(HookedFunction::TranslateShipForward,
            [&](const HookData hook_data)
            {
                int current_this = (int)*(int*)(hook_data.ebp + 0x8);

                // Keep going until we get the movement we want
                if (current_this != vessel_movement)
                    return false;

                ship_x = *(float*)(hook_data.eax);
                ship_y = *(float*)(hook_data.eax + 0x8);

                return true;
            });

            HookManager::Get().ExecuteInHookSync(HookedFunction::HelmManagerFixedUpdate,
            [&](const HookData hook_data)
            {
                ((void(*)(void*, int))GetFunctionAddress("Assembly-CSharp", "PlayerFunctions", "ClickOnTube"))((void*)player_functions, tube);

                HookManager::Get().ExecuteInHookAsync(HookedFunction::WeaponSourceSetWaypoint,
                [&](const HookData hook_data)
                {
                    float* x = (float*)(hook_data.eax);
                    float* y = (float*)(hook_data.eax + 8);

                    constexpr float pi = 3.14159265358979323846f;
                    float rotation = -((float)bearing / 360.0f) * 2.0f * pi;

                    *x = ship_x - std::sin(rotation) * distance;
                    *y = ship_y + std::cos(rotation) * distance;

                    return true;
                });

                ((void(*)(void*))GetFunctionAddress("Assembly-CSharp", "WeaponSource", "FireTube"))((void*)weapon_source);
                return true;
            });
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

