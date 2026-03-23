#include "memory.h"
#include "interfaces.h"
CEconItem* CEconItem::CreateInstance()
{

	using fnCreateSharedObjectSubclassEconItem = CEconItem * (__cdecl*)();
	static fnCreateSharedObjectSubclassEconItem oCreateSharedObjectSubclassEconItem = reinterpret_cast<fnCreateSharedObjectSubclassEconItem>(M::FindPattern(L"client.dll", "48 83 EC 28 B9 48 00 00 00 E8 ? ? ? ? 48 85"));

	return oCreateSharedObjectSubclassEconItem();
}
void SetDynamicAttributeValueString(CEconItem* econ, int index, const char* value) {
	if (!econ)
		return;

	C_EconItemSchema* pItemSchema = I::iclient->get_econ_item_system()->get_econ_item_schema();
	if (!pItemSchema)
		return;


	void* pAttributeDefinitionInterface =
		pItemSchema->GetAttributeDefinitionInterface(index);
	if (!pAttributeDefinitionInterface)
		return;

	//uint8_t attributestringdata[0x100] = {};

	//hooks::m_init_attribute_string(attributestringdata, 0, false);

	//*reinterpret_cast<uint32_t*>(attributestringdata + 0x10) = 1;

	//hooks::m_fill_attribute_string(reinterpret_cast<void*>(attributestringdata + 0x20), value);
	//*reinterpret_cast<void**>(attributestringdata + 0x18) = reinterpret_cast<void*>(attributestringdata + 0x20);

	//hooks::set_dynamic_attribute_value_string(econ, pAttributeDefinitionInterface, attributestringdata);
}

void CEconItem::SetDynamicAttributeValue(int index, void* value)
{
	C_EconItemSchema* pItemSchema = I::iclient->get_econ_item_system()->get_econ_item_schema();

	if (!pItemSchema)
		return;

	void* pAttributeDefinitionInterface =
		pItemSchema->GetAttributeDefinitionInterface(index);

	if (!pAttributeDefinitionInterface)
		return;
	using SetDynamicAttributeValueUint_t = __int64(__fastcall*)(void* pThis, void* pAttributeDef, void* value);

	void* addr = M::FindPattern(L"client.dll", "48 89 6C 24 ? 57 41 56 41 57 48 81 EC ? ? ? ? 48 8B FA C7 44 24 ? ? ? ? ? 4D 8B F8 4C 8D 0D ? ? ? ? 48 8B E9 4C 8D 05 ? ? ? ? 33 D2 48 8B 4F ? E8 ? ? ? ? 4C 8B F0 48 85 C0 0F 84 ? ? ? ? 48 8B 55 ? 48 89 9C 24 ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 89 B4 24 ? ? ? ? 48 85 D2 74 ? 0F B7 4A ? 48 8D 5A ? 0F B6 F1 48 C1 E6 ? 48 03 F3 48 3B DE 73 ? 90 ? ? ? 48 8B CF ? ? ? ? ? 74 ? 48 83 C3 ? 48 3B DE 72 ? EB ? 48 85 DB 75 ? 48 8B CD E8 ? ? ? ? 48 8B D8");

	if (!addr) {
		return;
	}

	SetDynamicAttributeValueUint_t fnSetDynamicAttributeValueUint =
		reinterpret_cast<SetDynamicAttributeValueUint_t>(addr);

	fnSetDynamicAttributeValueUint(this, pAttributeDefinitionInterface, value);


}
