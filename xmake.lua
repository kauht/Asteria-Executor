set_project("LuaExecutor")
set_version("1.0.0")
set_languages("cxx23")

add_rules("mode.debug", "mode.release")

set_targetdir("bin/$(mode)")

-- Add packages
add_requires("luajit")
add_requires("imgui")
add_requires("glfw")
add_requires("minhook")

-- Common target for headers
target("common")
set_kind("headeronly")
add_headerfiles("src/common/**.h")
add_includedirs("src/common")

-- DLL Target (The Executor)
target("executor_dll")
set_kind("shared")
add_files("src/dll/**.cpp")
add_includedirs("src/common")
add_includedirs("src/dll")
add_packages("luajit")
add_packages("minhook")
set_filename("executor_v2.dll")

-- Windows specific
if is_plat("windows") then
    add_syslinks("user32", "kernel32", "advapi32")
end

-- EXE Target (The Controller/UI)
target("executor_ui")
set_kind("binary")
add_files("src/exe/**.cpp")
add_includedirs("src/common")
add_includedirs("src/exe")
add_packages("imgui", "glfw")

-- Windows specific
if is_plat("windows") then
    add_syslinks("user32", "gdi32", "shell32", "opengl32")
end
