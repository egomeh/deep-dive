#pragma once
#include "pch.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

#define MONO_TABLE_TYPEDEF 2
#define MONO_TOKEN_TYPE_DEF 0x02000000

HMODULE self;

typedef void(__cdecl* GFunc) (void* data, void* user_data);

void* (*mono_get_root_domain)           (void);
void* (*mono_thread_attach)             (void*);

void  (*mono_assembly_foreach)          (GFunc func, void* user_data);
void* (*mono_assembly_get_name)         (void*);
char* (*mono_stringify_assembly_name)   (void*);
void* (*mono_assembly_get_image)        (void*);

char* (*mono_image_get_name)            (void*);
void* (*mono_image_get_table_info)      (void*, int);

int   (*mono_table_info_get_rows)       (void*);

void* (*mono_class_get)                 (void*,int);
char* (*mono_class_get_name)            (void*);

void __cdecl ForEachAssembly(void* assembly, std::vector<void*> *v)
{
    v->push_back(assembly);
}

void Entry()
{
	HMODULE hMono = GetModuleHandleA("mono.dll");

    if (!hMono)
    {
        FreeLibraryAndExitThread(self, 0);
        return;
    }

    mono_get_root_domain = (void* (__cdecl*)(void))GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach = (void* (__cdecl*)(void*))GetProcAddress(hMono, "mono_thread_attach");
    mono_assembly_foreach = (void(__cdecl*)(GFunc, void*))GetProcAddress(hMono, "mono_assembly_foreach");
    mono_assembly_get_name = (void*(__cdecl*)(void*))GetProcAddress(hMono, "mono_assembly_get_name");
    mono_stringify_assembly_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_stringify_assembly_name");
    mono_assembly_get_image = (void*(__cdecl*)(void*))GetProcAddress(hMono, "mono_assembly_get_image");
    mono_image_get_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_image_get_name");
    mono_image_get_table_info = (void*(__cdecl*)(void*, int))GetProcAddress(hMono, "mono_image_get_table_info");
    mono_table_info_get_rows = (int(__cdecl*)(void*))GetProcAddress(hMono, "mono_table_info_get_rows");
    mono_class_get = (void* (__cdecl*)(void*, int))GetProcAddress(hMono, "mono_class_get");
    mono_class_get_name = (char*(__cdecl*)(void*))GetProcAddress(hMono, "mono_class_get_name");

	void* rootDomain = mono_get_root_domain();
	void* monoThreadHandle = mono_thread_attach(rootDomain);

    std::vector<void*> assemblies;
    mono_assembly_foreach((GFunc)&ForEachAssembly, (void*)&assemblies);

    void* assembly_csharp = nullptr;
    void* image_csharp = nullptr;

    for (void* assembly : assemblies)
    {
        void* image = mono_assembly_get_image(assembly);
        char* image_name = mono_image_get_name(image);

        if (strcmp(image_name, "Assembly-CSharp") == 0)
        {
            assembly_csharp = assembly;
            image_csharp = image;
            break;
        }
    }

    void* tdef = mono_image_get_table_info(image_csharp, MONO_TABLE_TYPEDEF);

    if (!tdef)
    {
        FreeLibraryAndExitThread(self, 0);
        return;
    }

    int tdefcount = mono_table_info_get_rows(tdef);

    void* HelmManagerClass = nullptr;

    for (int i = 0; i < tdefcount; ++i)
    {
        void* c = mono_class_get(image_csharp, MONO_TOKEN_TYPE_DEF | i + 1);

        if (!c)
            continue;

        char* class_name = mono_class_get_name(c);

        if (strcmp(class_name, "HelmManager") == 0)
        {
            HelmManagerClass = c;
            break;
        }
    }

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

