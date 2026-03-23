#include "schema.h"

#include "interfaces.h"
CSchemaSystem* CSchemaSystem::Get()
{
    static const uintptr_t instance = (uintptr_t)I::schema;
    return reinterpret_cast<CSchemaSystem*>(instance);
}

CSchemaSystemTypeScope* CSchemaSystem::FindTypeScopeForModule(const char* moduleName)
{
    if (!I::schema) {
        printf("[SCHEMA ERROR] interfaces::schema is null! Make sure interfaces::init() was called.\n");
        return nullptr;
    }
    return I::schema->find_type_scope(moduleName);
}


namespace schema_manager {
    // Global instance

    SchemaManager::SchemaManager() : m_pSchemaSystem(nullptr) {
    }



    bool SchemaManager::Initialize() {
        // Find SchemaSystem using pattern scanning
        //m_pSchemaSystem = (CSchemaSystem*)memory::PatternScan("schemasystem.dll", "48 89 05 ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ?? 0F B6 45 ?? 4C 8D 45 ?? 33 F6");
        //
        //if (!m_pSchemaSystem) {
        //    std::cerr << "Failed to find SchemaSystem\n";
        //    return false;
        //}

        //// Wait a bit for schema to be ready

        return true;
    }

    bool SchemaManager::FindDeclaredClass(CSchemaSystemTypeScope* typeScope, const char* className, CSchemaDeclaredClass** outClass)
    {
        // Check if the provided type scope, class name, or output class pointer is null
        if (!typeScope || !className || !outClass)
            return false;

        // Loop through all the declared classes in the type scope
        for (uint16_t i = 0; i < typeScope->numDeclaredClasses; ++i)
        {
            // Retrieve the current declared class from the type scope
            CSchemaDeclaredClass* declaredClass = typeScope->declaredClasses[i].declaredClass;

            // Check if the class is valid and its name matches the provided className
            if (declaredClass && declaredClass->name && strcmp(declaredClass->name, className) == 0)
            {
                // If a match is found, set outClass to the found declared class and return true
                *outClass = declaredClass;
                return true;
            }
        }

        // Return false if no matching class is found
        return false;
    }

    // Function to retrieve the offset of a field from a given module, class, and field name
    uint32_t SchemaManager::GetOffset(const char* moduleName, const char* className, const char* fieldName)
    {
        uint32_t res = 0;  // Variable to hold the resulting offset

        // Check if interfaces are initialized
        if (!I::schema) {
            printf("[SCHEMA ERROR] interfaces::schema is null! Make sure interfaces::init() was called first.\n");
            return res;
        }

        // Retrieve the SchemaSystem instance (this is the interface to interact with schema data)
        CSchemaSystem* schemaSystem = CSchemaSystem::Get();
        if (!schemaSystem) {
            printf("[SCHEMA ERROR] SchemaSystem is null\n");
            return res;  // Return 0 if SchemaSystem cannot be obtained
        }

        // Find the type scope for the specified module (e.g., "client.dll")
        CSchemaSystemTypeScope* typeScope = schemaSystem->FindTypeScopeForModule(moduleName);
        if (!typeScope) {
            printf("[SCHEMA ERROR] TypeScope for module '%s' not found\n", moduleName);

            // Debug: Try to find available modules


            return res;  // Return 0 if type scope is not found for the module
        }

        // Declare a pointer to hold the declared class
        CSchemaDeclaredClass* declaredClass = nullptr;
        // Для вывода информации о typeScope, например его имени:
        // Use the FindDeclaredClass function to search for the class within the type scope
        if (!SchemaManager::FindDeclaredClass(typeScope, className, &declaredClass) || !declaredClass) {
            printf("[SCHEMA ERROR] Class '%s' not found in module '%s'\n", className, moduleName);



            return res;  // Return 0 if the class is not found
        }

        // Retrieve the SchemaClass from the declared class
        SchemaClassInfoData_t* schemaClass = declaredClass->mClass;

        // Check if the class and its fields are valid
        if (!schemaClass || !schemaClass->fields) {
            printf("[SCHEMA ERROR] SchemaClass or fields are invalid for class '%s'\n", className);
            return res;  // Return 0 if the class or its fields are invalid
        }

        // Loop through all the fields in the schema class
     //   printf("[SCHEMA DEBUG] Searching for field '%s' in class '%s' (%d fields)\n", fieldName, className, schemaClass->numFields);
        for (uint16_t i = 0; i < schemaClass->numFields; ++i)
        {
            // Retrieve the current field
            SchemaClassFieldData_t& field = schemaClass->fields[i];

            // Check if the field name matches the provided fieldName
            if (field.name && strcmp(field.name, fieldName) == 0)
            {
                // If a match is found, store the field's offset in the result variable
                res = field.offset;
                // printf("[SCHEMA SUCCESS] Found field '%s' with offset 0x%X\n", fieldName, res);
                break;  // Exit the loop since the field has been found
            }
        }

        if (res == 0) {
            printf("[SCHEMA ERROR] Field '%s' not found in class '%s'\n", fieldName, className);
        }

        // Return the found offset (or 0 if the field was not found)
        return res;
    }



}
