#include "schema.h" // Ensure the correct header file is included for CSchemaSystemTypeScope
#include "memory.h"
class i_schemasystem {
public:

    CSchemaSystemTypeScope* find_type_scope(const char* moduleName) { // Use the fully qualified name to resolve ambiguity
        return M::call_virtual<CSchemaSystemTypeScope*>(this, 13, moduleName, nullptr);


    }
};
