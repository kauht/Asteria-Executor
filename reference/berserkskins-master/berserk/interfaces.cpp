#include "interfaces.h"
#include <Windows.h>
#include <cstdint>
#include <type_traits>
#include <stdio.h>
#define INIT_IFACE(var, dll, ver) \
    var = get_interface<std::remove_pointer_t<decltype(var)>>(&g_modules->m_modules.dll, ver); \
    if (!var) { \
        printf("[I] interface %s not found!\n", #var); \
        ok = false; \
    }

constexpr int CREATE_INTERFACE_OFFSET = 3;
constexpr int LIST_OFFSET = 7;


struct InterfaceReg {
    std::add_pointer_t<uintptr_t()> createFn;
    const char* name;
    InterfaceReg* next;
};

inline uintptr_t GetInterface(const char* moduleName, const char* interfaceName) {
    uintptr_t res = 0;

    HMODULE moduleBase = GetModuleHandleA(moduleName);
    if (!moduleBase) return res;

    typedef void* (*CreateInterfaceFn)(const char*, int*);
    const auto createInterface = reinterpret_cast<CreateInterfaceFn>(
        GetProcAddress(moduleBase, "CreateInterface")
        );
    if (!createInterface) return res;

    const uintptr_t list = reinterpret_cast<uintptr_t>(createInterface) +
        *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(createInterface) + CREATE_INTERFACE_OFFSET) +
        LIST_OFFSET;

    for (const InterfaceReg* current = *reinterpret_cast<InterfaceReg**>(list); current; current = current->next) {
        if (strstr(current->name, interfaceName) != nullptr) {
            res = current->createFn();
            break;
        }
    }

    return res;
}

template <typename T>
inline T* GetInterfaceTyped(const char* moduleName, const char* interfaceName) {
    return reinterpret_cast<T*>(GetInterface(moduleName, interfaceName));
}

class c_dll {
private:
    const char* m_name;
public:
    c_dll(const char* name) : m_name(name) {}
    const char* get_name() const { return m_name; }
};

template <typename T>
inline T* get_interface(c_dll* dll, const char* name) {
    return GetInterfaceTyped<T>(dll->get_name(), name);
}

struct modules_t {
    c_dll engine2_dll;
    c_dll schemasystem_dll;
    c_dll particles_dll;
    c_dll materialsystem2_dll;
    c_dll scenesystem_dll;
    c_dll inputsystem_dll;
    c_dll client_dll;
    c_dll localize_dll;
    c_dll resourcesys_dll;
    c_dll filesystem_stdio;
    c_dll panorama_dll;

    modules_t() :
        client_dll("client.dll"),
        inputsystem_dll("inputsystem.dll"),
        engine2_dll("engine2.dll"),
        schemasystem_dll("schemasystem.dll"),
        particles_dll("particles.dll"),
        materialsystem2_dll("materialsystem2.dll"),
        scenesystem_dll("scenesystem.dll"),
        localize_dll("localize.dll"),
        resourcesys_dll("resourcesystem.dll"),
        filesystem_stdio("filesystem_stdio.dll"),
        panorama_dll("panorama.dll")
    {
    }
};

class c_modules {
public:
    modules_t m_modules;
};

inline c_modules* get_modules() {
    static c_modules instance;
    return &instance;
}

#define g_modules get_modules()
namespace I {


    bool initialize() {
        bool ok = true;

        INIT_IFACE(localize, localize_dll, "Localize_001");
        INIT_IFACE(file_system, filesystem_stdio, "VFileSystem017");
        INIT_IFACE(engine, engine2_dll, "Source2EngineToClient001");
        INIT_IFACE(iclient, client_dll, "Source2Client002");
        INIT_IFACE(panorama, panorama_dll, "PanoramaUIEngine001");
        INIT_IFACE(schema, schemasystem_dll, "SchemaSystem_001");
        INIT_IFACE(gameresource, engine2_dll, "GameResourceServiceClientV001");
        INIT_IFACE(inputsystem, inputsystem_dll, "InputSystemVersion001");

        eventmanager = *reinterpret_cast<i_event_manager**>(M::ResolveRelativeAddress(M::get_v_method<std::uint8_t*>(iclient, 14U) + 0x3E, 0x3, 0x7));
        if (!eventmanager) {
            printf("[I] interface eventmanager not found!\n");
            return false;
        }

        pvs = reinterpret_cast<i_pvs*>(M::ResolveRelativeAddress(reinterpret_cast<uint8_t*>(M::FindPattern(L"engine2.dll", "48 8D 0D ? ? ? ? 33 D2 FF 50")), 0x3, 0x7));
        if (!pvs) {
            printf("[I] interface pvs not found!\n");
            return false;
		}

        return ok;

    }

}