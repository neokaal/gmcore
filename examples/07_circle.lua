local t = 0

function draw(dt)
    t = t + dt
    canvas:clear(0, 0, 0)

    local r = math.floor(t % 300)
    local cx = math.floor(canvas.width / 2)
    local cy = math.floor(canvas.height / 2)

    local miny = math.max(0, cy - r - 1)
    local maxy = math.min(canvas.height - 1, cy + r + 1)
    local minx = math.max(0, cx - r - 1)
    local maxx = math.min(canvas.width - 1, cx + r + 1)

    for y = miny, maxy do
        for x = minx, maxx do
            local dx = cx - x
            local dy = cy - y
            local d = math.floor(math.sqrt((dx * dx) + (dy * dy)))
            if d == r then
                canvas:set_pixel(x, y, 255, 255, 255, 255)
            end
        end
    end
end
