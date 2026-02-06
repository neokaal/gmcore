function rect(x, y, w, h)
    for i = x, x + w do
        for j = y, y + h do
            px(i, j, 255, 255, 0)
        end
    end
end

local time = 0
function draw(dt)
    time = time + dt
    local rw = 100
    local rh = 100
    -- position of the rectangle is in the center of the screen, but it moves left and right over time
    local x = width / 2 - rw / 2 + math.floor(math.sin(time / 1000) * 150)
    local y = height / 2 - rh / 2

    bg(0, 0, 0)
    rect(x, y, rw, rh)
end
