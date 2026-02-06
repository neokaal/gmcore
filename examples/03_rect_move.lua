function rect(x, y, w, h)
    for i = x, x + w do
        for j = y, y + h do
            px(i, j, 255, 255, 0)
        end
    end
end

function draw(dt)
    -- rectangle dimensions are rw X rh
    local rw = 100
    local rh = 100
    -- rectangle position is in the center of the screen
    -- therefore its top-left is
    local x = width / 2 - rw / 2 + math.floor(math.sin(t) * 150)
    local y = height / 2 - rh / 2

    bg(0, 0, 0)
    rect(x, y, rw, rh)
end
