local time = 0
local rw = 100
local rh = 100

function draw(dt)
    time = time + dt

    local x = canvas.width / 2 - rw / 2
    local y = canvas.height / 2 - rh / 2
    local dx = math.floor(math.sin(time / 500) * 200)

    canvas:clear(0, 0, 0)
    canvas:fill_rect(x + dx, y, rw, rh, 255, 255, 0)
end
