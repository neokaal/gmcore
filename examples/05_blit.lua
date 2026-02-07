-- examples/05_dump.lua

function draw(t)
    bg(0, 0, 0)

    local img_w = 50
    local img_h = 50
    local img = {}

    for j = 0, img_h - 1 do
        for i = 0, img_w - 1 do
            table.insert(img, i * 5)
            table.insert(img, j * 5)
            table.insert(img, 255)
        end
    end

    blit(img, 100, 100, img_w, img_h)
end
