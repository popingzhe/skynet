

print("run init.lua")
function OnInit(id)
    print("[lua] main OnInit id:"..id)
    local caht = sunnet.NewService("chat")
    -- local ping1 = sunnet.NewService("ping")
    -- print("[lua] new service ping1:"..ping1)

    -- local ping2 = sunnet.NewService("ping")
    -- print("[lua] new service ping2:"..ping2)

    -- local pong = sunnet.NewService("ping")
    -- print("[lua] new service pong:"..pong)

    -- sunnet.Send(ping1,pong,"Start")
    -- sunnet.Send(ping2,pong,"Start")
end

function OnExit()
    print("[lua] main OnExit")
end