# AnimatedWallpaper3DS

Current milestone: real animated-GIF playback app on Old/New 2DS/3DS hardware.

## Test
1. Copy `AnimatedWallpaper3DS.3dsx` to `SD:/3ds/AnimatedWallpaper3DS/AnimatedWallpaper3DS.3dsx`
2. Copy `sdroot/animatedwallpaper/wallpaper.awp` to `SD:/animatedwallpaper/wallpaper.awp`
3. Launch from Homebrew Launcher.
4. A pauses/resumes; START exits.

## Make your own wallpaper pack
`python tools/pack_gif.py your.gif wallpaper.awp`

The runtime format is deliberately simple so the exact same frame pack can later be consumed by the HOME Menu hook.
