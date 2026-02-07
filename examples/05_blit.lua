function draw(dt)
    canvas:clear(0, 0, 0)

    local img_w = 50
    local img_h = 50

    for j = 0, img_h - 1 do
        for i = 0, img_w - 1 do
            canvas:set_pixel(100 + i, 100 + j, i * 5, j * 5, 255, 255)
        end
    end
end