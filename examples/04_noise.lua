function draw(dt)
    gm:clear(0, 0, 0)
    for y = 0, gm.height - 1 do
        for x = 0, gm.width - 1 do
            local r = math.random(0, 255)
            local g = math.random(0, 255)
            local b = math.random(0, 255)
            gm:set_pixel(x, y, r, g, b, 255)
        end
    end
end
