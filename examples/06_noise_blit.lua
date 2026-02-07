function draw(dt)
    for y = 0, canvas.height - 1 do
        for x = 0, canvas.width - 1 do
            local v = math.random(0, 255)
            canvas:set_pixel(x, y, v, v, v, 255)
        end
    end
end
