function draw(dt)
    canvas:clear(0, 0, 0)
    for y = 0, canvas.height - 1 do
        for x = 0, canvas.width - 1 do
            local r = math.random(0, 255)
            local g = math.random(0, 255)
            local b = math.random(0, 255)
            canvas:set_pixel(x, y, r, g, b, 255)
        end
    end
end
