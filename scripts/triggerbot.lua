-- triggerbot_cm.lua
-- Auto-fires using CreateMove button flags writes instead of Hardware Simulation.

local ffi = require('ffi')

pcall(function()
    ffi.cdef[[
        double atan2(double y, double x);
        double sqrt(double x);
        double fabs(double x);
    ]]
end)

local m = ffi.load('msvcrt')

-- ─── Config ──────────────────────────────────────────────────────────────────
local FOV    = 2.0      -- Triggerbot FOV (degrees)
local HOTKEY = 0x12     -- VK_MENU (ALT key). Set to 0 to always trigger.
-- ─────────────────────────────────────────────────────────────────────────────

-- Math constants & helpers
local PI = math.pi
local function todeg(r) return r * 180 / PI end

local function calcAngle(sx, sy, sz, ex, ey, ez)
    local dx, dy, dz = ex-sx, ey-sy, ez-sz
    return todeg(m.atan2(dy, dx)), todeg(m.atan2(-dz, m.sqrt(dx*dx + dy*dy)))
end

local function angDiff(a, b)
    local d = (a - b) % 360
    if d > 180 then d = d - 360 end
    return m.fabs(d)
end

local OFF_VIEW_OFFSET = 0xD58
local OFF_ORIGIN      = 0x1588
local OFF_SCENE_NODE  = 0x338

local function getBonePosition(pawn_addr, index)
    if not pawn_addr or pawn_addr == 0 then return nil end
    local scene_node = Asteria.ReadPtr(pawn_addr + OFF_SCENE_NODE)
    if not scene_node or scene_node == 0 then return nil end
    local bone_cache = Asteria.ReadPtr(scene_node + 0x1E0)
    if not bone_cache or bone_cache == 0 then return nil end
    local offset = index * 32
    return Asteria.ReadFloat(bone_cache + offset),
           Asteria.ReadFloat(bone_cache + offset + 4),
           Asteria.ReadFloat(bone_cache + offset + 8)
end

local function readVec(base, off)
    return Asteria.ReadFloat(base + off),
           Asteria.ReadFloat(base + off + 4),
           Asteria.ReadFloat(base + off + 8)
end

-- Toggles every shoot frame to force semi-auto continuous firing 
local attack_tick_state = false

if not CS2.ready then
    print('[TB-CM] GameContext not ready.')
    return
end

-- CreateMove is hooked automatically on C++ attach! Just bind callback.
local ok = Asteria.OnCreateMove(function()
    local cmd = Asteria.GetCurrentUserCmd()
    if not cmd then return end -- Cmd not ready for current tick

    local raw = ffi.cast('uint32_t*', cmd)
    local data = ffi.cast('float*', cmd)
    
    local vp, vy = data[4], data[5] -- Pitch, Yaw

    -- Hotkey checks
    if HOTKEY > 0 then
        if not Asteria.IsKeyDown(HOTKEY) then
            -- Make sure we release attack state if hotkey is let go
            attack_tick_state = false
            return
        end
    end

    local lp_ctrl = Asteria.GetLocalPlayer()
    if not lp_ctrl then return end
    
    local lp_pawn_addr = nil
    for _, p in ipairs(Asteria.players) do
        if tonumber(ffi.cast('uintptr_t', p.addr)) == lp_ctrl then
            lp_pawn_addr = p.pawn
            break
        end
    end
    if not lp_pawn_addr or lp_pawn_addr == 0 then return end

    local lx, ly, lz = readVec(lp_pawn_addr, OFF_ORIGIN)
    local vx, vyo, vz = readVec(lp_pawn_addr, OFF_VIEW_OFFSET)
    local eye_x, eye_y, eye_z = lx + vx, ly + vyo, lz + vz

    local trigger_target_found = false

    -- Declarative Entity Loop
    for _, p in ipairs(Asteria.players) do
        if p.is_enemy and p.is_alive then
            local hx, hy, hz = getBonePosition(p.pawn, 6)
            if hx and hx ~= 0 then
                local ay, ap = calcAngle(eye_x, eye_y, eye_z, hx, hy, hz)
                if angDiff(vy, ay) < FOV and angDiff(vp, ap) < FOV then
                    trigger_target_found = true
                    break
                end
            end
        end
    end

    if trigger_target_found then
        -- Toggle state high/low alternate frames to force semi-auto cycling
        attack_tick_state = not attack_tick_state
        if attack_tick_state then
            -- Set IN_ATTACK (1) in nButtons.nValue
            raw[20] = bit.bor(raw[20], 1)
            -- Also set nButtons.m_nChanged (offset 0x58 -> index 22)
            raw[22] = bit.bor(raw[22], 1)
        else
            raw[20] = bit.band(raw[20], bit.bnot(1))
            raw[22] = bit.band(raw[22], bit.bnot(1))
        end
    else
        attack_tick_state = false
    end
end)

if ok then
    print('[TB-CM] CreateMove Triggerbot active. FOV=' .. FOV .. ' deg. Hotkey=ALT')
else
    print('[TB-CM] ERROR: hook failed!')
end
