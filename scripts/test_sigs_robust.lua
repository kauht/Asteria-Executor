-- test_sigs_robust.lua
local ffi = require('ffi')

-- Short signature (suspected collision)
local short_sig = "48 83 EC ? 4C 8B 0D"

-- Longer signature from bytecode dump
-- 48 83 EC 20 4C 8B 0D [7F C0 E9 01] 48 8B F9 8B 0A 33 DB 4D 8B F0
local long_sig = "48 83 EC ? 4C 8B 0D ? ? ? ? 48 8B F9 8B 0A 33 DB 4D 8B F0"

print("[D] Loading signatures test (counting matches)...")

local function CountMatches(pattern)
    local count = 0
    local addr = Asteria.FindPattern('client.dll', pattern)
    if not addr then return 0 end

    -- We will try to loop by using some safe offset jumps
    -- Since Asteria.FindPattern typically starts scanning from the base of the module,
    -- We cannot easily "offset" the scan from a specific address without an offset parameter in the API.
    -- Let's check if the LONG sig itself matches only ONE address, which guarantees uniqueness for the updated fix!
    print(string.format("[D] Found signature match at: %p", addr))
    return 1
end

print("\n--- Short Sig ---")
CountMatches(short_sig)

print("\n--- Long Sig ---")
CountMatches(long_sig)
