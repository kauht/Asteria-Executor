-- disasm_seq.lua
local ffi = require('ffi')

local addr_seq = Asteria.FindPattern('client.dll', "40 53 48 83 EC")

if not addr_seq then
    print('[D] Signature for Seq failed.')
    return
end

local addr_num = tonumber(ffi.cast('uintptr_t', addr_seq))
print(string.format("[D] GetCUserCmdBySequenceNumber Address: %X", addr_num))

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
