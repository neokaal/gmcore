function draw(dt)
    bg(0, 0, 0)
    for i = 1, width do
        for j = 1, height do
            local r = math.random(0, 255)
            local g = math.random(0, 255)
            local b = math.random(0, 255)
            px(i, j, r, g, b)
        end
    end
end
