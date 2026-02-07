function draw(dt)
    for y = 0, gm.height - 1 do
        for x = 0, gm.width - 1 do
            local v = math.random(0, 255)
            gm:set_pixel(x, y, v, v, v, 255)
        end
    end
end
