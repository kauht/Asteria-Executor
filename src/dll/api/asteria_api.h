#pragma once

static const char* asteriaApi = R"(
local ffi = require('ffi')
ffi.cdef[[
    void* GetModuleHandleA(const char* lpModuleName);
    void* GetProcAddress(void* hModule, const char* lpProcName);
    typedef void* (*CallFnNativeFn)(void* addr, int argCount, uint64_t* args);

    void* GetCurrentProcess();
    int ReadProcessMemory(void* hProcess, void* lpBaseAddress, void* lpBuffer, size_t nSize, size_t* lpNumberOfBytesRead);
    int WriteProcessMemory(void* hProcess, void* lpBaseAddress, void* lpBuffer, size_t nSize, size_t* lpNumberOfBytesWritten);

    bool HookFunction(void* pTarget, void* pDetour, void** ppOriginal);
    bool UnhookFunction(void* pTarget);

    void* ScanPattern(const char* moduleName, const char* pattern);
    
    // User32 wrappers
    short GetAsyncKeyState(int vKey);
    void mouse_event(uint32_t dwFlags, uint32_t dx, uint32_t dy, uint32_t dwData, uintptr_t dwExtraInfo);
]]

local kernel32 = ffi.load('kernel32.dll')
local user32 = ffi.load('user32.dll')
local current_dll = ffi.load('executor_v2.dll')
local CallFnNative = ffi.cast('CallFnNativeFn', CallFnNativePtr)
local currentProcess = kernel32.GetCurrentProcess()

-- Memory read helpers (safe via ReadProcessMemory)
local function read_uintptr(addr)
    local buf = ffi.new('uintptr_t[1]')
    if kernel32.ReadProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 8, nil) == 0 then return 0 end
    return buf[0]
end

local function read_int(addr)
    local buf = ffi.new('int[1]')
    if kernel32.ReadProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 4, nil) == 0 then return 0 end
    return buf[0]
end

local function read_float(addr)
    local buf = ffi.new('float[1]')
    if kernel32.ReadProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 4, nil) == 0 then return 0.0 end
    return buf[0]
end

local function read_string(addr, max_len)
    max_len = max_len or 128
    local buf = ffi.new('char[?]', max_len)
    if kernel32.ReadProcessMemory(currentProcess, ffi.cast('void*', addr), buf, max_len, nil) == 0 then return nil end
    return ffi.string(buf)
end

local function write_int(addr, val)
    local buf = ffi.new('int[1]', val)
    return kernel32.WriteProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 4, nil) ~= 0
end

local function write_float(addr, val)
    local buf = ffi.new('float[1]', val)
    return kernel32.WriteProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 4, nil) ~= 0
end

local function write_uintptr(addr, val)
    local buf = ffi.new('uintptr_t[1]', val)
    return kernel32.WriteProcessMemory(currentProcess, ffi.cast('void*', addr), buf, 8, nil) ~= 0
end

Asteria = {
    GetModule = function(name)
        local h = kernel32.GetModuleHandleA(name)
        if h == nil then return nil end
        return ffi.cast('void*', h)
    end,

    GetProc = function(handle, name_or_id, signature)
        local addr
        if type(name_or_id) == 'number' then
            addr = kernel32.GetProcAddress(handle, ffi.cast('const char*', name_or_id))
        else
            addr = kernel32.GetProcAddress(handle, name_or_id)
        end
        if addr == nil then return nil end
        if signature then return ffi.cast(signature, addr) end
        return function(...)
            local args = {...}
            local count = #args
            local ffi_args = ffi.new('uint64_t[?]', count)
            for i=1, count do
                 if type(args[i]) == 'string' then
                      ffi_args[i-1] = ffi.cast('uint64_t', ffi.cast('const char*', args[i]))
                 elseif type(args[i]) == 'number' then
                      ffi_args[i-1] = ffi.cast('uint64_t', args[i])
                 elseif type(args[i]) == 'boolean' then
                      ffi_args[i-1] = args[i] and 1 or 0
                 else
                      ffi_args[i-1] = ffi.cast('uint64_t', args[i])
                 end
            end
            return CallFnNative(addr, count, ffi_args)
        end
    end,

    FindPattern = function(moduleName, pattern)
        local addr = current_dll.ScanPattern(moduleName, pattern)
        if addr == nil or addr == ffi.cast("void*", 0) then return nil end
        return addr
    end,

    GetEntity = function(entityList, index)
         if entityList == nil or entityList == 0 then return nil end
         local rank = bit.rshift(bit.band(index, 0x7FFF), 9)
         local listEntryPtr = read_uintptr(ffi.cast('uintptr_t', entityList) + 8 * rank + 16)
         if listEntryPtr == 0 then return nil end
         
         local controller = read_uintptr(ffi.cast('uintptr_t', listEntryPtr) + 112 * bit.band(index, 0x1FF))
         if controller == 0 then return nil end
         return controller
    end,

    Hook = function(pTarget, pDetour, ppOriginal)
        return current_dll.HookFunction(ffi.cast('void*', pTarget), ffi.cast('void*', pDetour), ffi.cast('void**', ppOriginal))
    end,

    Unhook = function(pTarget)
        return current_dll.UnhookFunction(ffi.cast('void*', pTarget))
    end,

    -- Public memory helpers
    ReadPtr = function(addr) return read_uintptr(ffi.cast('uintptr_t', addr)) end,
    ReadInt = function(addr) return read_int(ffi.cast('uintptr_t', addr)) end,
    ReadFloat = function(addr) return read_float(ffi.cast('uintptr_t', addr)) end,
    ReadString = function(addr, len) return read_string(ffi.cast('uintptr_t', addr), len) end,
    WritePtr = function(addr, val) return write_uintptr(ffi.cast('uintptr_t', addr), val) end,
    WriteInt = function(addr, val) return write_int(ffi.cast('uintptr_t', addr), val) end,
    WriteFloat = function(addr, val) return write_float(ffi.cast('uintptr_t', addr), val) end,

    -- Print to the CS2 in-game developer console
    ConMsg = function(...)
        local args = {...}
        local parts = {}
        for i = 1, #args do parts[i] = tostring(args[i]) end
        ConMsgCpp(table.concat(parts, '\t'))
    end,
}

-- ─── Pre-built CS2 context (populated once by C++ backend at DLL attach) ────
-- These are raw numeric addresses; do not dereference without nil/zero checks.
CS2 = {
    ready        = ContextReadyCpp(),
    entity_list  = GetEntityListCpp(),
    local_ctrl   = GetLocalControllerCpp(),
    client_base  = GetClientBaseCpp(),
    view_angles  = GetViewAnglesPtrCpp(),
}
-- ─────────────────────────────────────────────────────────────────────────────

local function read_uint8(addr)
    return bit.band(read_int(addr), 0xFF)
end

local function getPawn(controller)
    local ctrl = tonumber(ffi.cast('uintptr_t', controller))
    if not ctrl or ctrl == 0 then return nil end
    local handle = read_int(ctrl + 0x90C) -- m_hPlayerPawn
    if not handle or handle == 0 then return nil end
    local idx = bit.band(handle, 0x7FFF)
    if idx <= 0 or idx >= 2048 then return nil end
    local list = CS2.entity_list
    if list == 0 then return nil end
    local pawn = Asteria.GetEntity(ffi.cast('uintptr_t', list), idx)
    if not pawn or pawn == 0 then return nil end
    return tonumber(ffi.cast('uintptr_t', pawn))
end

local player_mt = {
    __index = function(p, k)
        local ep_pawn = getPawn(p.addr)
        if k == 'pawn' then
            return ep_pawn
        elseif k == 'health' then
            return ep_pawn and read_int(ep_pawn + 0x354) or 0
        elseif k == 'team' then
            return ep_pawn and read_uint8(ep_pawn + 0x3F3) or 0
        elseif k == 'is_alive' then
            return p.health > 0
        elseif k == 'is_enemy' then
            local lp_ctrl = Asteria.GetLocalPlayer()
            if not lp_ctrl then return false end
            local lp_pawn = getPawn(lp_ctrl)
            if not lp_pawn then return false end
            local lp_team = read_uint8(lp_pawn + 0x3F3)
            return p.team ~= lp_team
        elseif k == 'name' then
            return p.name_cached
        end
    end
}

Asteria.GetPlayers = function()
     local list = {}
     local entityList = CS2.entity_list
     if entityList == 0 then return list end

     for i = 1, 64 do
          local controller = Asteria.GetEntity(ffi.cast('uintptr_t', entityList), i)
          if controller and controller ~= 0 then
               local name = read_string(ffi.cast('uintptr_t', controller) + 0x6F8, 64)
               if name and #name > 0 and name:match('[%w%s]') then
                    table.insert(list, setmetatable({ addr = controller, index = i, name_cached = name }, player_mt))
               end
          end
     end
     return list
end

-- Returns the local player controller address as a uintptr_t number (pre-cached).
Asteria.GetLocalPlayer = function()
     local ctrl = CS2.local_ctrl
     if not ctrl or ctrl == 0 then return nil end
     return ctrl
end

-- Utilities
Asteria.Print = function(text)
    return PrintCpp(text)
end

Asteria.SetSkin = function(defIndex, paintKit, wear, seed, statTrak)
    return SetSkinCpp(defIndex, paintKit, wear, seed, statTrak)
end

Asteria.SetKnife = function(defIndex, paintKit, wear, seed)
    return SetKnifeCpp(defIndex, paintKit, wear, seed)
end

Asteria.SetGlove = function(defIndex, paintKit, wear, seed)
    return SetGloveCpp(defIndex, paintKit, wear, seed)
end

Asteria.AddInventoryItem = function(defIndex)
    return AddInventoryItemCpp(defIndex)
end

Asteria.GetInventoryCount = function()
    return GetInventoryCountCpp()
end

Asteria.GetLastErrorCode = function()
    return GetLastErrorCodeCpp()
end

Asteria.GetRawWord = function(index)
    return GetRawWordCpp(index)
end

Asteria.GetEconItemRawWord = function(itemIndex, wordIndex)
    return GetEconItemRawWordCpp(itemIndex, wordIndex)
end

Asteria.GetEconItemCacheCount = function()
    return GetEconItemCacheCountCpp()
end

Asteria.ScanInventoryManager = function()
    return ScanInventoryManagerCpp()
end

Asteria.GetScanResultCount = function()
    return GetScanResultCountCpp()
end

Asteria.GetScanResult = function(index)
    return GetScanResultCpp(index)
end

Asteria.GetCurrentUserCmd = function()
     local ctrl = Asteria.GetLocalPlayer()
     if not ctrl then return nil end
     return GetCurrentUserCmdCpp(ctrl)
end

Asteria.OnCreateMove = function(callback)
     if not callback then return false end
     return RegisterCreateMoveCallbackCpp(callback)
end

Asteria.UnhookCreateMove = function()
     return UnhookCreateMoveCpp()
end

Asteria.IsKeyDown = function(key)
     return bit.band(user32.GetAsyncKeyState(key), 0x8000) ~= 0
end

Asteria.Shoot = function(held)
     if held then
          user32.mouse_event(0x0002, 0, 0, 0, 0) -- MOUSEEVENTF_LEFTDOWN
     else
          user32.mouse_event(0x0004, 0, 0, 0, 0) -- MOUSEEVENTF_LEFTUP
     end
end

setmetatable(Asteria, {
     __index = function(t, k)
          if k == 'players' then
               return t.GetPlayers()
          end
     end
})
)";
