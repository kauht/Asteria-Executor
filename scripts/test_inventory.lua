print("[MyCheat] Starting True Inventory Changer Test...")

-- Scan and print manager structure
Asteria.ScanInventoryManager()
local count = Asteria.GetEconItemCacheCount()
print("[MyCheat] Inventory Cache Count: " .. tostring(count))

print("[MyCheat] === DUMPING UP TO 20 CECONITEMS ===")
for i = 0, 20 do
    local out = "Item " .. i .. ":\n  "
    for w = 0, 17 do -- 18 words * 4 bytes = 72 bytes
        local val = Asteria.GetEconItemRawWord(i, w)
        out = out .. string.format("[%02d] %08X  ", w * 4, val)
    end
    print(out)
end
print("[MyCheat] =================================")

-- Add AK-47 Redline (defIndex = 7)
Asteria.AddInventoryItem(7)

local countAfter = Asteria.GetInventoryCount()
local errCode = Asteria.GetLastErrorCode()
print("[MyCheat] Inventory Count AFTER Add: " .. countAfter)
print("[MyCheat] Last Error Code: " .. errCode)

if errCode == 0 then
    print("[MyCheat] SUCCESS: AddItemToInventory returned true!")
else
    local errMsg = {
        [6] = "SOCache null",
        [7] = "SOTypeCache null",
        [8] = "AddObject returned false",
        [13] = "SOCreated exception",
    }
    print("[MyCheat] FAIL: Error " .. errCode .. " - " .. (errMsg[errCode] or "Unknown"))
end
