local function test_api()
    print("Testing API bindings...")
    
    if type(Asteria.SetKnife) == "function" then
        print("Asteria.SetKnife IS a function.")
        Asteria.SetKnife(507, 568, 0.01, 0)
        Asteria.Print("[MyCheat] Set Karambit Emerald!\n")
    else
        print("ERROR: Asteria.SetKnife is not found!")
    end

    if type(Asteria.SetSkin) == "function" then
        print("Asteria.SetSkin IS a function.")
        Asteria.SetSkin(7, 282, 0.01, 0, 1337)
        print("Asteria.SetSkin executed!")
    else
        print("ERROR: Asteria.SetSkin is not found!")
    end
end

test_api()