-- search_offsets.lua
local ffi = require('ffi')

local function readFile(path)
    local f = io.open(path, 'r')
    if not f then print('Fail open ' .. path); return '' end
    local c = f:read('*all')
    f:close()
    return c
end

local content = readFile('C:\\Users\\sam\\Desktop\\cs2-exec\\scripts\\client_dll.hpp')
if content == '' then return end

local function findOffset(cls, var)
    -- Matches: constexpr std::ptrdiff_t var_name = 0x1234;
    -- Wait, our client_dll.hpp from earlier is typically structured like namespace offsets.
    -- Let's just find the line containing the variable name and print it.
    for line in content:gmatch("[^\r\n]+") do
        if line:match(var) then
             print(line)
        end
    end
end

print("--- Searching for m_pGameSceneNode ---")
findOffset("", "m_pGameSceneNode")

print("\n--- Searching for m_modelState ---")
findOffset("", "m_modelState")

print("\n--- Searching for m_vecAbsOrigin ---")
findOffset("", "m_vecAbsOrigin")
