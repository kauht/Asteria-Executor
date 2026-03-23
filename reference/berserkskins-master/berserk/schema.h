#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <iostream>


#include <cstdio> 


// Forward declarations
struct SchemaClassInfoData_t;
struct SchemaClassFieldData_t;
struct CSchemaDeclaredClass;
struct CSchemaDeclaredClassEntry;
struct CSchemaSystemTypeScope;
struct CSchemaSystem;

// Schema class field data
struct SchemaClassFieldData_t {
    const char* name;      // Name of the variable (e.g., "m_iHealth")
    void* type;            // Pointer to the variable type
    uint32_t offset;       // Offset of the variable inside the class
    uint32_t metadataSize; // Metadata size
    void* metadata;                       // 0x0018
};

// Schema class info data
struct SchemaClassInfoData_t {
    void* ptr;             // Pointer to the class data
    const char* name;      // Name of the class (e.g., "C_BaseEntity")
    const char* moduleName;// Name of the module (e.g., "client.dll")
    uint32_t size;         // Class size
    uint16_t numFields;    // Number of fields in the class

    char padding1[0x2];

    uint16_t staticSize;
    uint16_t metadataSize;

    char padding2[0x4];

    SchemaClassFieldData_t* fields;        // 0x0060
    const char* get_name() {
        return *reinterpret_cast<const char**>((unsigned __int64)(this) + 0x8);
    };

};
struct CSchemaDeclaredClass {
    void* ptr;              // Pointer to class metadata
    const char* name;       // Class name
    const char* moduleName; // Module where the class is declared
    const char* unknownStr; // Unknown string (possibly related to debugging)
    SchemaClassInfoData_t* mClass;
};
struct CSchemaDeclaredClassEntry {

    uint64_t hash[2];            // Hash of the class name
    CSchemaDeclaredClass* declaredClass; // Pointer to the declared class
};
// Schema System Type Scope
struct CSchemaSystemTypeScope {
    void* ptr;
    char name[256];               // Module name

    char padding1[0x338];
    uint16_t numDeclaredClasses;  // Number of declared classes in the module


    char padding2[0x6];
    CSchemaDeclaredClassEntry* declaredClasses; // Pointer to the declared classes array


};

// Schema System
struct CSchemaSystem {
    static CSchemaSystem* Get();  // Function to get the instance of the SchemaSystem interface
    CSchemaSystemTypeScope* FindTypeScopeForModule(const char* moduleName);  // Function to find the type scope for a specific module

};


namespace schema_manager {
    class SchemaManager {
    private:
        CSchemaSystem* m_pSchemaSystem;
        std::unordered_map<std::string, uint32_t> m_fieldCache;

    public:
        SchemaManager();


        // Initialize the schema manager
        bool Initialize();

        // Find field offset in a class
        bool  FindDeclaredClass(CSchemaSystemTypeScope* typeScope, const char* className, CSchemaDeclaredClass** outClass);

        // Find field in type scope
        uint32_t GetOffset(const char* moduleName, const char* className, const char* fieldName);

        // Get schema system
        CSchemaSystem* GetSchemaSystem() const { return m_pSchemaSystem; }

        // Check if initialized
        bool IsInitialized() const { return m_pSchemaSystem != nullptr; }
    };

    inline SchemaManager g_schemaManager;


}


#define SCHEMA(type, name, className, fieldName) \
        type& name() { \
            static uint32_t offset = schema_manager::g_schemaManager.GetOffset("client.dll", className, fieldName); \
            return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); \
        }

#define SCHEMA_EX(type, name, className, fieldName, extraOffset) \
    type& name() { \
        static uint32_t offset = schema_manager::g_schemaManager.GetOffset("client.dll", className, fieldName); \
        return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset + extraOffset); \
    }
