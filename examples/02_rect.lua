function draw(dt)
    -- rectangle dimensions are rw X rh
    local rw = 100
    local rh = 100
    -- rectangle position is in the center of the screen
    -- therefore its top-left is
    local x = width / 2 - rw / 2
    local y = height / 2 - rh / 2

    px(x, y, 255, 255, 0)
    for i = x, x + rw do
        for j = y, y + rh do
            px(i, j, 255, 255, 0)
        end
    end
end
