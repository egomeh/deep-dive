#pragma once

#include <vector>
#include <string>
#include <map>

// Assembly -> class -> method -> address
// std::map<std::string, std::map<std::string, std::map<std::string, void*>>> function_address_map;

#define MONO_TABLE_TYPEDEF 2
#define MONO_TOKEN_TYPE_DEF 0x02000000

bool InitMonoInteraction();
void* FindMonoImage(const char* name);
void* FindClassFromImage(void* image, const char* name);
void* FindMethodInClass(void* mono_class, const char* name);
void* FindCodeAddress(void* mono_class, const char* name);

void RegiterFunction(std::string assembly, std::string mono_class, std::string function_name);
void* GetFunctionAddress(std::string assembly, std::string mono_class, std::string function_name);
