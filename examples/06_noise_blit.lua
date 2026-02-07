function draw(dt)
    local noise_data = noise()
    blit(noise_data, 0, 0, width, height)
end
