#include "mono_interaction.h"

#include <Windows.h>

#include <vector>

HMODULE hMono;

typedef void(__cdecl* GFunc) (void* data, void* user_data);

static void* (*mono_get_root_domain)            (void);
static void* (*mono_thread_attach)              (void*);

static void  (*mono_assembly_foreach)           (GFunc func, void* user_data);
static void* (*mono_assembly_get_name)          (void*);
static char* (*mono_stringify_assembly_name)    (void*);
static void* (*mono_assembly_get_image)         (void*);

static char* (*mono_image_get_name)             (void*);
static void* (*mono_image_get_table_info)       (void*, int);
 
static int   (*mono_table_info_get_rows)        (void*);

static void* (*mono_class_get)                  (void*, int);
static char* (*mono_class_get_name)             (void*);

static void* (*mono_class_get_methods)          (void*, void**);
static char* (*mono_method_get_name)            (void*);

bool InitMonoInteraction()
{
    hMono = GetModuleHandleA("mono.dll");

    if (!hMono)
        return false;

    mono_get_root_domain = (void* (__cdecl*)(void))GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach = (void* (__cdecl*)(void*))GetProcAddress(hMono, "mono_thread_attach");
    mono_assembly_foreach = (void(__cdecl*)(GFunc, void*))GetProcAddress(hMono, "mono_assembly_foreach");
    mono_assembly_get_name = (void* (__cdecl*)(void*))GetProcAddress(hMono, "mono_assembly_get_name");
    mono_stringify_assembly_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_stringify_assembly_name");
    mono_assembly_get_image = (void* (__cdecl*)(void*))GetProcAddress(hMono, "mono_assembly_get_image");
    mono_image_get_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_image_get_name");
    mono_image_get_table_info = (void* (__cdecl*)(void*, int))GetProcAddress(hMono, "mono_image_get_table_info");
    mono_table_info_get_rows = (int(__cdecl*)(void*))GetProcAddress(hMono, "mono_table_info_get_rows");
    mono_class_get = (void* (__cdecl*)(void*, int))GetProcAddress(hMono, "mono_class_get");
    mono_class_get_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_class_get_name");
    mono_class_get_methods = (void* (__cdecl*)(void*, void**))GetProcAddress(hMono, "mono_class_get_methods");
    mono_method_get_name = (char* (__cdecl*)(void*))GetProcAddress(hMono, "mono_method_get_name");

    return true;
}

static void __cdecl ForEachAssembly(void* assembly, std::vector<void*>* v)
{
    v->push_back(assembly);
}

void* FindMonoImage(const char* name)
{
    if (!name)
        return nullptr;

    std::vector<void*> assemblies;
    mono_assembly_foreach((GFunc)&ForEachAssembly, (void*)&assemblies);

    void* assembly_csharp = nullptr;

    for (void* assembly : assemblies)
    {
        void* image = mono_assembly_get_image(assembly);
        char* image_name = mono_image_get_name(image);

        if (strcmp(image_name, name) == 0)
            return image;
    }

    return nullptr;
}

void* FindClassFromImage(void* image, const char* name)
{
    void* tdef = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);

    if (!tdef)
        return nullptr;

    int tdefcount = mono_table_info_get_rows(tdef);

    for (int i = 0; i < tdefcount; ++i)
    {
        void* c = mono_class_get(image, MONO_TOKEN_TYPE_DEF | i + 1);

        if (!c)
            continue;

        char* class_name = mono_class_get_name(c);

        if (strcmp(class_name, name) == 0)
            return c;
    }

    return nullptr;
}

void* FindMethodInClass(void* c, const char* name)
{
    if (!name)
        return nullptr;

    if (!c)
        return nullptr;

    void* iterator = nullptr;

    while (true)
    {
        void* method = mono_class_get_methods(c, &iterator);

        if (method)
        {
            char* method_name = mono_method_get_name(method);

            if (strcmp(method_name, name) == 0)
                return method;
        }
    }

    return nullptr;
}
