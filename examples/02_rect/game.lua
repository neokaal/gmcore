local function fillRect(x, y, w, h, r, g, b, a)
    a = a or 255
    for i = x, x + w do
        for j = y, y + h do
            gm:setPixel(i, j, r, g, b, a)
        end
    end
end

function draw(dt)
    gm:clear(0, 0, 0)
    fillRect(100, 100, 80, 80, 255, 255, 0)
end
