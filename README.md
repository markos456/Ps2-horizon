# PS2 Horizon — first build

This is a first playable **2.5D-style racing prototype** for PlayStation 2 homebrew.
It includes a title menu, pseudo-perspective road, a stylized car, steering/acceleration/braking,
checkpoint progression, a timer, and a finish state. It is not a full polygonal 3D open world yet.

## Build on GitHub Actions

1. Copy `source/main.c`, `Makefile`, and `.github/workflows/build.yml` into the repository,
   keeping the paths exactly as shown.
2. Commit the files.
3. Open **Actions** and select **Build PS2 Horizon**.
4. Download artifact `ps2-horizon-elf` after a successful run; it contains `horizon.elf`.

The workflow uses the public PS2DEV development container and PS2SDK/gsKit.
The code has not been compiled in this environment; inspect the first Actions log and fix
any SDK-version/API mismatch based on the actual error output.

## Controls

- D-pad left/right: steer
- X: accelerate / start from title menu
- Circle: brake
- Start: return to title menu

## Important

This ELF is homebrew and does not include commercial game assets or an ISO.
Run it in PCSX2 using its ELF/homebrew launch option, or on hardware through a homebrew loader
compatible with your setup.
