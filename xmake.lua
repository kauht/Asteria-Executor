set_project("LuaExecutor")
set_version("1.0.0")
set_languages("cxx23")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })
set_targetdir("bin/$(mode)")

-- grouped package declarations
add_requires("luajit", "minhook", "imgui", "glfw")

-- small helper to add the project's common include directory
local function use_common_includes()
    add_includedirs("src/common")
end

-- DLL Target (Executor)
target("executor_dll")
    set_kind("shared")
    add_files("src/dll/**.cpp")
    use_common_includes()
    add_includedirs("src/dll")
    add_packages("luajit", "minhook")
    set_filename("executor_v2.dll")

    if is_plat("windows") then
        add_syslinks("user32", "kernel32", "advapi32")
    end

-- EXE Target (Controller / UI)
target("executor_ui")
    set_kind("binary")
    add_files("src/exe/**.cpp")
    use_common_includes()
    add_includedirs("src/exe")
    add_packages("imgui", "glfw")

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "shell32", "opengl32")
    end
