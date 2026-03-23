#pragma once
#include "hook_manager.h"

/*
 * HOOK MACROS - Упрощенные макросы для работы с хуками
 * 
 * Эти макросы упрощают объявление и использование хуков
 */

// ============================================
// МАКРОСЫ ДЛЯ ОБЪЯВЛЕНИЯ ХУКОВ
// ============================================

// Объявляет тип функции и хук объект в namespace H
#define DECLARE_HOOK(name, returnType, callingConvention, ...) \
    typedef returnType (callingConvention* name##_t)(__VA_ARGS__); \
    namespace H { inline CBaseHookObject<name##_t> hk##name = {}; }

// ============================================
// МАКРОСЫ ДЛЯ DETOUR ФУНКЦИЙ
// ============================================

// Начинает определение detour функции
#define DETOUR_DECL(name, returnType, callingConvention, ...) \
    returnType callingConvention name##_Detour(__VA_ARGS__)

// Вызов оригинальной функции
#define CALL_ORIGINAL(name, ...) \
    H::hk##name.GetOriginal()(__VA_ARGS__)

// ============================================
// МАКРОСЫ ДЛЯ СОЗДАНИЯ ХУКОВ
// ============================================

// Создает хук (использовать в H::Setup())
#define CREATE_HOOK(name, address) \
    if (!H::hk##name.Create(reinterpret_cast<void*>(address), &name##_Detour)) \
    { \
        printf("[!] Failed to create " #name " hook\n"); \
        return false; \
    } \
    printf("[+] " #name " hook created\n");

// ============================================
// МАКРОСЫ ДЛЯ УДАЛЕНИЯ ХУКОВ
// ============================================

// Удаляет хук (использовать в H::Destroy())
#define REMOVE_HOOK(name) \
    if (H::hk##name.IsHooked()) \
    { \
        H::hk##name.Remove(); \
        printf("[+] " #name " hook removed\n"); \
    }

// ============================================
// ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ МАКРОСОВ
// ============================================

/*

// 1. ОБЪЯВЛЕНИЕ ХУКА:
DECLARE_HOOK(DrawObject, void, __fastcall, void* thisptr, int a1, int a2)
DECLARE_HOOK(GetFOV, float, __fastcall, void* pCameraServices)
DECLARE_HOOK(SetHealth, void, __stdcall, void* player, float health)

// Это создаст:
// - typedef void (__fastcall* DrawObject_t)(void* thisptr, int a1, int a2);
// - namespace H { inline CBaseHookObject<DrawObject_t> hkDrawObject = {}; }


// 2. СОЗДАНИЕ DETOUR ФУНКЦИИ:
DETOUR_DECL(DrawObject, void, __fastcall, void* thisptr, int a1, int a2)
{
    printf("[*] DrawObject called\n");
    
    // Вызов оригинала
    CALL_ORIGINAL(DrawObject, thisptr, a1, a2);
}

DETOUR_DECL(GetFOV, float, __fastcall, void* pCameraServices)
{
    float fov = CALL_ORIGINAL(GetFOV, pCameraServices);
    return fov + 10.0f; // Изменяем FOV
}


// 3. СОЗДАНИЕ ХУКА В H::Setup():
bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
        return false;

    CREATE_HOOK(DrawObject, 0x12345678)
    CREATE_HOOK(GetFOV, 0x87654321)

    return true;
}


// 4. УДАЛЕНИЕ ХУКА В H::Destroy():
void H::Destroy()
{
    REMOVE_HOOK(DrawObject)
    REMOVE_HOOK(GetFOV)

    MH_Uninitialize();
}

*/

// ============================================
// ПОЛНЫЙ ПРИМЕР С МАКРОСАМИ
// ============================================

/*

// В заголовочном файле или в начале cpp:

// Объявляем хуки
DECLARE_HOOK(MyFunction, void, __fastcall, int param)
DECLARE_HOOK(GetValue, int, __stdcall, void* obj)

// Создаем detour функции
DETOUR_DECL(MyFunction, void, __fastcall, int param)
{
    printf("MyFunction: param = %d\n", param);
    CALL_ORIGINAL(MyFunction, param);
}

DETOUR_DECL(GetValue, int, __stdcall, void* obj)
{
    int value = CALL_ORIGINAL(GetValue, obj);
    return value * 2; // Удваиваем значение
}

// В hook_manager.cpp:

bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
        return false;

    // Создаем хуки одной строкой
    CREATE_HOOK(MyFunction, 0x12345678)
    CREATE_HOOK(GetValue, 0x87654321)

    printf("[+] All hooks created\n");
    return true;
}

void H::Destroy()
{
    printf("[*] Destroying hooks...\n");

    // Удаляем хуки одной строкой
    REMOVE_HOOK(MyFunction)
    REMOVE_HOOK(GetValue)

    MH_Uninitialize();
    printf("[+] All hooks destroyed\n");
}

*/



