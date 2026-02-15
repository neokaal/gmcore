local time = 0
local rw = 100
local rh = 100

local function fillRect(x, y, w, h, r, g, b, a)
    a = a or 255
    for i = x, x + w do
        for j = y, y + h do
            gm:setPixel(i, j, r, g, b, a)
        end
    end
end

function draw(dt)
    time = time + dt

    local x = gm.width / 2 - rw / 2
    local y = gm.height / 2 - rh / 2
    local dx = math.floor(math.sin(time / 500) * 200)

    gm:clear(0, 0, 0)
    fillRect(x + dx, y, rw, rh, 255, 255, 0)
end
