local function fillRect(x, y, w, h, r, g, b, a)
    a = a or 255
    for i = x, x + w do
        for j = y, y + h do
            gm:setPixel(i, j, r, g, b, a)
        end
    end
end

function draw(dt)
    fillRect(gm.width / 2 - 50, gm.height / 2 - 50, 100, 100, 128, 128, 0, 255)
    gm:saveFrame()
    gm:noLoop()
end
