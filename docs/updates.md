# Update History

## Version 2021.08.05 [*Update 1*]

This is the first true update to the raytracer! It's not a terribly focused update, and it includes commits from as far back as May.
The main results of the update are some significant optimization, several new primitives, refactoring and restructuring, and of course
some bug fixes. This update should make the raytracer run more quickly and be good groundwork for future features and optimization.

#### Interesting Changes
- Added spot lights and a new `spot_light` command
- Added circle primitives and a new `circle` command
- Added ellipse primitives and a new `ellipse` command
- Added some command line arguments for filtering output from the logger ("verbose" and "silent" modes)
- Started using dynamic thread allocation with OpenMP for increased thread efficiency
- Made several optimizations to object construction and copying that resulted in a huge speed increase, especially for larger scenes
- Clarified primitive abstraction, made adding new primitives much easier
- Changed how colors are handled to support features like high dynamic range (HDR), bloom, and colorblindness simulation in the future
- Fixed a bug where point lights found shadows where they shouldn't have
- Fixed a bug where scenes with 0 primitives would lead to an infinite recursion
