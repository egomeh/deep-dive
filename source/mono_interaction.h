#pragma once

#include <vector>

#define MONO_TABLE_TYPEDEF 2
#define MONO_TOKEN_TYPE_DEF 0x02000000

bool InitMonoInteraction();
void* FindMonoImage(const char* name);

void* FindClassFromImage(void* image, const char* name);

void* FindMethodInClass(void* c, const char* name);