# gmcore

gmcore is a tiny, bare-bones game engine designed for learning.

gmcore prioritizes minimal design, clarity, simplicity, and direct control.

# Features

- easy to build/install/distribute
- `lua` as the primary programming language
- intentionally minimal API
  - you need something, you build it yourself
  - learning by building is the goal
- direct pixel access to the fixed-size game canvas

# Possible future additions

- basic image loading (likely PPM)
- simple sound generation via minimal api

# Usage

- create a project directory
- place a `game.lua` file in the directory
- run `gmcore` in the directory
- a game window opens
- edit `game.lua` and see the changes immediately
- iterate...

# API

## Program Structure

- _gmcore_ expects the `game.lua` file to define a `draw(dt)` function
  which will be called every frame.
- the engine will pass the _deltatime in milliseconds_ to the `draw` function through the `dt` argument.

```lua
function draw(dt)
  -- game drawing code goes here
end
```

## The `gm` object

All _gmcore_ functions and settings are accessed through the global
`gm` object.

## `gm.width` and `gm.height` - Get the dimension of the game canvas in pixels.

## `gm:clear(r, g, b, a)` - Clear the screen

This function clears the screen by settings the given red, green, blue, (and optional alpha) values to every pixel on the canvas.

The range of each colour value is **0-255 inclusive**.

The default value of alpha `a` is 255 if not provided.

## `gm:setPixel(x, y, r, g, b, a)` - Set a single pixel

This function sets a single pixel value at the location given by `x, y` with the colour value `r, g, b, a`. `a` is optional and its default value is 255.

## `gm:noLoop()` - Stop the game loop

The game loop will stop, no more frames will be drawn till the `game.lua` is reloaded.

# Status

This project is an early concept, so the API and the features list is unstable. Expect breaking changes.
