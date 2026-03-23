-- disasm_tick.lua
local ffi = require('ffi')

local addr_tick = Asteria.FindPattern('client.dll', "48 83 EC ? 4C 8B 0D")

if not addr_tick then
    print('[D] Signature failed.')
    return
end

local addr_num = tonumber(ffi.cast('uintptr_t', addr_tick))
print(string.format("[D] GetCUserCmdTick Address: %X", addr_num))

-- Read 32 bytes from function entry point
print("[D] Function Bytes:")
local str = "  "
for i = 0, 31 do
    local b = Asteria.ReadInt(addr_num + i) % 256 -- read 1 byte
    str = str .. string.format("%02X ", b)
    if (i + 1) % 16 == 0 then
        print(str)
        str = "  "
    end
end
if #str > 2 then print(str) end
