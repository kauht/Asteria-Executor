# Asteria API Documentation

## Modules & Handles

### `Asteria.GetModule(name: string) -> void*|nil`
Returns handle to a loaded module, or `nil` if not found.
```lua
local client = Asteria.GetModule("client.dll")
```

### `Asteria.GetProc(handle, name_or_id, signature?) -> function|cdata|nil`
Gets function address via `GetProcAddress`. Pass ordinal number or string name.
```lua
local ConMsg = Asteria.GetProc(tier0, 392)
```

### `Asteria.Print(text: string)`
Prints text securely to the Counter-Strike 2 engine developer console via native `ConMsg`.
```lua
Asteria.Print("[MyCheat] Initialized successfully!\n")
```

### `Asteria.SetSkin(defIndex: int, paintKit: int, wear: float, seed: int, statTrak: int)`
Locates the underlying weapon by Definition Index and injects a new visual paint kit onto it dynamically utilizing `RegenerateWeaponSkins`. `statTrak` can be set to `-1` to disable it.
```lua
Asteria.SetSkin(7, 282, 0.01, 0, 1337) -- AK-47 Redline, StatTrak 1337
```

---

## Pattern Scanner

### `Asteria.FindPattern(moduleName: string, pattern: string) -> void*|nil`
Scans a module for an IDA-style byte pattern. Supports `?` and `??` wildcards.
```lua
local addr = Asteria.FindPattern("client.dll", "48 89 5C 24 ?? 55 57 41 56")
```

---

## Memory Read/Write

| Function | Description |
|---|---|
| `Asteria.ReadPtr(addr)` | Read 8-byte pointer |
| `Asteria.ReadInt(addr)` | Read 4-byte int |
| `Asteria.ReadFloat(addr)` | Read 4-byte float |
| `Asteria.ReadString(addr, len?)` | Read string (default 128 chars) |
| `Asteria.WritePtr(addr, val)` | Write 8-byte pointer |
| `Asteria.WriteInt(addr, val)` | Write 4-byte int |
| `Asteria.WriteFloat(addr, val)` | Write 4-byte float |

---

## Entity System

### `Asteria.GetPlayers() -> table`
Returns array of valid player controllers with `.name`, `.health`, `.index`, `.addr`.
```lua
for _, p in ipairs(Asteria.GetPlayers()) do
    print(p.name .. " HP: " .. p.health)
end
```

---

## Hooking (MinHook)

### `Asteria.Hook(target: void*, detour: void*, ppOriginal: void**) -> bool`
Creates an inline hook via MinHook.

### `Asteria.Unhook(target: void*) -> bool`
Removes a hook.

```lua
local ffi = require('ffi')
ffi.cdef[[ typedef void (__fastcall *MyFn)(void*, float*); ]]
local original = ffi.new("void*[1]")
local detour = ffi.cast("MyFn", function(self, data) ... end)
Asteria.Hook(addr, ffi.cast("void*", detour), original)
```

---

## Saved Signatures
All patterns stored in `src/dll/offsets/signatures.hpp`:
- `create_move`, `create_move_mover1` — CreateMove variants
- `swapchain` — DX11 swapchain
- `entity_system`, `get_base_entity` — Entity access
- `get_view_angles`, `set_view_angles` — View angle control
- `get_is_in_game`, `get_is_connected` — Game state checks
- `screen_transform`, `get_matrix_for_view` — View matrix
