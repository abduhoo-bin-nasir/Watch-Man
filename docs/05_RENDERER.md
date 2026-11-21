# 05 - How the renderer works (raycasting, explained from zero)

Read this with `src/Renderer.cpp` open. Function names are exact. **Everything here is [DONE].**

## 0. The idea in one paragraph
We do **not** build a 3D model of the building. The world is a flat 2D grid. To draw a frame we shoot **one ray per screen column** (320 rays) from the camera into the grid. Where the ray stops tells us how far away the thing in that column is, and from that distance we can compute how tall the wall looks on screen. The same trick, run on the rows below and above the wall, gives floors and ceilings. This is the technique of Wolfenstein 3D / Doom (1992-93), and it is why the game runs on weak laptops.

## 1. Space, camera and the screen

- The map is a grid of **cells** (one unit square each). `Level::cellAt(cx, cy)` returns a `Cell` with a type (`C_OPEN`, `C_WALL`, `C_DOOR`, `C_WINDOW`, `C_RAIL`), a floor height `F` and a ceiling height `C`.
- The camera has position `(px, py)`, a heading `yaw`, an eye height `eye = feet + EYE_H` and a horizon row `hz = RH/2 + bob + pitch` (looking up/down just moves the horizon; this "shear" is why pitch is cheap and why it has limits, `PITCH_MAX`).
- `CamState` is what `Game` hands to `Renderer::render`. The renderer builds a private `Cam`:
  `dir = (cos yaw, sin yaw)` and `plane = (-dir.y, dir.x) * PLANE` where `PLANE = 0.5 * RW / RH` (about 0.89). The plane is a line perpendicular to the view direction; its length sets the horizontal field of view.
- The internal image is **320 x 180** (`RW`, `RH`). It is uploaded to a texture and scaled up with nearest-neighbour filtering (the chunky pixels).

## 2. One ray per column (`renderColumn`)

For screen column `x`:

```
cs = 2*x/RW - 1          // -1 (left edge) .. +1 (right edge)
ray = dir + plane * cs   // the direction of this column's ray
```

Because `ray = dir + plane*cs`, the parameter `t` along the ray is already the **perpendicular depth** (distance along the view direction). That is why there is no "fish-eye" and no correction step.

## 3. DDA: walking the ray through the grid cell by cell

We don't test points along the ray; we jump from cell boundary to cell boundary. (DDA = Digital Differential Analyzer.)

- `ddx = |1 / ray.x|`: how much `t` increases to cross **one whole cell in x**. `ddy = |1 / ray.y|` likewise.
- `sdx`, `sdy`: the `t` at which the ray first crosses the next vertical / horizontal grid line.
- Each iteration: if `sdx < sdy` the next boundary is a vertical one: `tFar = sdx; sdx += ddx; mx += stepX`. Otherwise the horizontal one. `tFar` is where the ray leaves the current cell.

**Worked example.** Player at (3.0, 3.0), ray = (1, 0.5). `ddx = 1`, `ddy = 2`. Next vertical line is x = 4 -> `sdx = (4-3)*1 = 1`. Next horizontal line is y = 4 -> `sdy = (4-3)*2 = 2`. Since 1 < 2 we step in x first: we leave cell (3,3) at `t = 1` (world point (4.0, 3.5)) and enter (4,3). Then `sdx` becomes 2, which ties with `sdy`; the next crossing is at `t = 2`. Each step costs a couple of additions and a comparison, so a ray visiting 20 cells is cheap.

## 4. The twist that makes stairs free: every cell has its own heights

Classic raycasters assume all walls are the same height and the floor is flat. Here every cell stores its own floor `F` and ceiling `C`, like Doom's sectors. So `renderColumn` keeps a **window of undrawn screen rows** `[yT, yB)` and, for each cell the ray passes through:

1. **Ceiling span.** Project the cell's ceiling height at the near and far edge of the cell (`projY`) and fill the rows between with ceiling texture. Move `yT` down.
2. **Floor span.** Same for the floor; fill, move `yB` up.
3. **Look at the neighbour cell the ray enters next:**
   - **Solid** (wall, door, window, rail): its face fills **the whole remaining window** -> `drawWallRows`, record the wall distance in `zbuf[x]`, stop.
   - **Open but higher floor** (`nx.F > cur.F`): draw a **riser** (the vertical face of a step) -> `drawRiserRows`, shrink the window from below, keep going.
   - **Open but lower ceiling:** draw the **ceiling step**, shrink from above, keep going.
4. Continue into the next cell until the window is empty or a wall ends it.

**A staircase is therefore just a row of cells whose `F` rises by 0.2 each cell.** No special case in the renderer. (`Level::build` sets those heights from a `FlightDef`.)

### The projection formula (`projY`)
A point at height `h` and depth `t` appears on screen row

```
y = hz - (h - eye) * RH / t
```

Higher than your eye -> above the horizon; farther away (bigger `t`) -> closer to the horizon. Walls at depth `t` span `projY(ceilingHeight)` to `projY(floorHeight)`. This single formula draws everything.

## 5. Floors and ceilings (floor casting)
`drawFloorRow(c, x, y, cell, rx, ry)`: for a pixel row `y` below the horizon, the ground point it shows is at depth `t = (eye - F) * RH / (y + 0.5 - hz)`; its world position is `p + t*ray`, and `frac(world * 0.5)` picks the texel (a floor tile is 2x2 cells). Ceilings are the mirror image. For stair cells the tread texture is oriented by `cell.stairAxis` (steps run along x or y) and `cell.stairSlope` (which edge carries the bright nosing), which is why flights can run in any direction.

## 6. Walls (`drawWallRows`)
- **Horizontal texture coordinate:** `u` = fractional position where the ray hit the face. A tile is two cells wide, so `uu = (subCell*0.5) + u*0.5` makes one door/window texture span a whole tile.
- **Vertical coordinate:** `v = 1 - (h - refF)/WALL_H`, where `h` is the height of the pixel on the wall. Next to a staircase `refF` is a **smooth slope** through the step edges (`cell.rf0 + rfx*x + rfy*y`) instead of the jagged stepped floor, so the trim line and handrail run in clean diagonals.
- **Doors:** a door cell with `doorAmt > 0` draws the leaf narrowing toward its hinge and a lit room behind (`roomView`); lift doors are two panels sliding apart with the cabin behind (`cabinView`); when fully open the lift's door cells become `C_OPEN` so you can walk in.
- **Windows:** `outsideView` paints a procedural night sky, skyline, falling rain (only out to a limit) and lightning, depending on the direction of the ray.

## 7. Light and fog (`lampL`, `putPix`)
There is **no ambient light**. Each bulb lights a circle of radius `LAMP_R`; for a world point at squared distance `d` from a bulb with `R`: `k = 1 - d/R^2`, contribution `k*k*brightness`. Contributions add up (`lampL`). `putPix` multiplies the texel by (cool ambient + warm lamp), by a distance fog `exp(-t/16)`, a vignette, an ordered **Bayer dither** and film grain, then keeps 5 bits per channel for the retro look. Bulb glows are drawn afterwards additively (`drawGlows`). Lightning adds a cold tint through nearby window lamps.

## 8. People and props (`drawBillboards`)
Characters are **billboards**: two crossed vertical planes. One shows the front (or the back, depending on which side the camera is on), the other shows the side (mirrored from the far side). For each screen column we intersect the ray with both planes (a 2D line-line intersection), look up the sprite pixel, and draw it **only if it is nearer than `zbuf[x]`** (the wall distance recorded earlier), which is how a person hides behind a corner. Doorways need a small trick: an open apartment door sets `zbuf` deeper, so the resident standing inside the door frame is not hidden by the wall plane.
`nearBill` records which talkable billboard is in the centre of the screen and within 4.8 cells: that is what `E` talks to.

## 9. Why floors are separate grids
A column renderer draws **one surface per row of pixels**. A room directly above another room would need two floors and ceilings on the same screen row, which it cannot represent. So each floor is its own grid, the stairwell is a real flight laid into both, and walking onto the last steps swaps grids (with a short fade that hides the change). This is a deliberate trade-off.

## 10. Performance notes
- Cost per frame is roughly 320 rays x (cells visited) + 57,600 pixel writes. There are no allocations in the hot path.
- `lampL` loops over every lamp for every pixel; this is the most expensive part. If a map grows to hundreds of bulbs, give each cell a list of nearby lamps (a spatial index / grid bucket).
- `Renderer` never calls raylib; it only writes into `pix[]`. `Ui::draw` uploads it with `UpdateTexture`.

## 11. Things to try (see `07_CODE_TOUR.md`)
Change `PLANE` (field of view), the fog divisor `16` (visibility), `LAMP_R` (light radius), the vignette strength, the dither multiplier. Break things on purpose: set `EYE_H` to 0.5 and watch the world become giant.
