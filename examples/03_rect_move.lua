function rect(x, y, w, h)
    for i = x, x + w do
        for j = y, y + h do
            px(i, j, 255, 255, 0)
        end
    end
end

local time = 0
local rw = 100
local rh = 100

function draw(dt)
    -- update time
    time = time + dt
    -- position of the rectangle is in the center of the screen, but it moves left and right over time
    local x = width / 2 - rw / 2
    local y = height / 2 - rh / 2
    local dx = math.floor(math.sin(time / 500) * 200)

    bg(0, 0, 0)
    rect(x + dx, y, rw, rh)
end
