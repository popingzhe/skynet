local conns = {}
local serviceId

function OnInit(id)
    serviceId = id
    print("[lua] chat OnInit id:"..id)
    r = sunnet.Listen(8002,id)
    if r<=0 then
        print("[lua] listen error")
        return
    end
end

function OnAcceptMsg(listenfd,clientfd)
    print("[lua] chat OnAcceptMsg "..clientfd)
    conns[clientfd] = true
end

function OnSocketData(fd,buff)
    print("[lua] chat OnSocketData "..fd)
    for fd, _ in pairs(conns) do
        sunnet.Write(fd,buff)
    end
end


function OnSocketClose(fd)
    print("[lua] caht OnSocketClose "..fd)
    conns[fd] = nil
end