print("hello")

function frame(t)
    local w = width()
    local h = height()

    for x = 0, w - 1 do
        for y = 0, h - 1 do
            if x == y then
                set_pixel(x, y, 255, 255, 255)
            end
        end
    end
end
