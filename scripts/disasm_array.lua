-- disasm_array.lua
local ffi = require('ffi')

local addr_arr = Asteria.FindPattern('client.dll', "48 89 4C 24 ? 41 56")

if not addr_arr then
    print('[D] Signature for Array failed.')
    return
end

local addr_num = tonumber(ffi.cast('uintptr_t', addr_arr))
print(string.format("[D] GetCUserCmdArray Address: %X", addr_num))

-- Read 64 bytes from function entry point
print("[D] Function Bytes:")
local str = "  "
for i = 0, 63 do
    local b = Asteria.ReadInt(addr_num + i) % 256
    str = str .. string.format("%02X ", b)
    if (i + 1) % 16 == 0 then
        print(str)
        str = "  "
    end
end
if #str > 2 then print(str) end
