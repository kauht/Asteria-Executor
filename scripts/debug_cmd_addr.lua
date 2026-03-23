-- debug_cmd_addr.lua
-- Inspects CreateMove arguments to find which one is actually the CUserCmd pointer.

if not CS2.ready then
    print('[Debug] GameContext not ready.')
    return
end

pcall(function() Asteria.UnhookCreateMove() end)

local count = 0

local ok = Asteria.OnCreateMove(function(rcx, rdx, r8, r9)
    count = count + 1
    if count > 5 then return end -- Only run a few times

    print(string.format("[Args] Count %d | rcx=%.0f | rdx=%.0f | r8=%.0f | r9=%.0f", 
         count, rcx, rdx, r8, r9))

    -- Safe read contents of r8
    if r8 and r8 > 0 then
         local v1 = Asteria.ReadInt(r8)
         local v2 = Asteria.ReadInt(r8 + 4)
         local v_buttons = Asteria.ReadInt(r8 + 0x50)
         print(string.format("  -> r8 [+0]=%X | [+4]=%X | [+0x50 Buttons]=%X", v1, v2, v_buttons))
         
         -- Also try dereferencing r8 to see if it's a double pointer
         local p_sub = Asteria.ReadPtr(r8)
         if p_sub and p_sub > 0 then
              local s_buttons = Asteria.ReadInt(p_sub + 0x50)
              print(string.format("  -> *r8 [+0x50 Buttons]=%X", s_buttons))
         end
    end
end)

if ok then
    print('[Debug] Running arguments inspector. Look at Executor Console.')
else
    print('[Debug] Hook failed!')
end
