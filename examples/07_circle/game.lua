local t = 0

function draw(dt)
    t = t + dt
    gm:clear(0, 0, 0)

    local r = math.floor(t % 300)
    local cx = math.floor(gm.width / 2)
    local cy = math.floor(gm.height / 2)

    local miny = math.max(0, cy - r - 1)
    local maxy = math.min(gm.height - 1, cy + r + 1)
    local minx = math.max(0, cx - r - 1)
    local maxx = math.min(gm.width - 1, cx + r + 1)

    for y = miny, maxy do
        for x = minx, maxx do
            local dx = cx - x
            local dy = cy - y
            local d = math.floor(math.sqrt((dx * dx) + (dy * dy)))
            if d == r then
                gm:set_pixel(x, y, 255, 255, 255, 255)
            end
        end
    end
end
