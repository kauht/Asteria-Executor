-- test_sigs.lua
local ffi = require('ffi')
-- Tests if forum signatures for CUserCmd addressing resolve correctly.

if not CS2.ready then 
    print('[Sig] GameContext not ready.')
    return
end

local sigs = {
    first_array = "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF",
    cmd_tick    = "48 83 EC ? 4C 8B 0D",
    cmd_array   = "48 89 4C 24 ? 41 56",
    cmd_seq     = "40 53 48 83 EC"
}

for name, sig in pairs(sigs) do
    local addr = Asteria.FindPattern('client.dll', sig)
    if addr then
        print(string.format("[Sig] %s FOUND at %X", name, tonumber(ffi.cast('uintptr_t', addr))))
    else
        print(string.format("[Sig] %s FAILED", name))
    end
end
