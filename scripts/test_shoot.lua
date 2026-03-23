-- test_shoot.lua
local ffi = require('ffi')
-- Clicks via on-demand GetCurrentUserCmd pointer resolving.
-- Shoots a 0.5 second burst every 2 seconds to verify button memory toggles.

local last_click = 0
local is_clicking = false
local click_duration_end = 0
local attack_tick_state = false

if not CS2.ready then
    print('[Test] GameContext not ready.')
    return
end

-- CreateMove timing-callback
local ok = Asteria.OnCreateMove(function()
    local now = os.clock()
    
    local cmd = Asteria.GetCurrentUserCmd()
    if not cmd then return end -- Cmd not ready for current tick
    
    local raw = ffi.cast('uint32_t*', cmd)

    -- Trigger click burst every 2 seconds
    if not is_clicking and (now - last_click > 2.0) then
        is_clicking = true
        click_duration_end = now + 0.5 -- Burst duration 500ms
        last_click = now
        print('[Test] Starting Burst (GetCurrentUserCmd)')
    end

    if is_clicking then
        if now < click_duration_end then
            attack_tick_state = not attack_tick_state
            if attack_tick_state then
                raw[20] = bit.bor(raw[20], 1) -- IN_ATTACK
                raw[22] = bit.bor(raw[22], 1) -- m_nChanged
            else
                raw[20] = bit.band(raw[20], bit.bnot(1))
                raw[22] = bit.band(raw[22], bit.bnot(1))
            end
        else
            is_clicking = false
            attack_tick_state = false
            print('[Test] Burst Finished')
        end
    end
end)

if ok then
    print('[Test] On-demand Auto-shoot test running. Shoots random bursts iteratively. Look at Executor CLI.')
else
    print('[Test] Hook failed!')
end
