print("hello")

function frame(t)
    local w = width()
    local h = height()

    for x = 0, w - 1 do
        for y = 0, h - 1 do
            if x == y then
                px(x, y, 255, 255, 0)
            end
        end
    end
end
