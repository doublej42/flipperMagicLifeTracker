# MTG Life Tracker for Flipper Zero

A Magic: The Gathering life tracker app for the Flipper Zero.

- On startup, choose **Standard** or **Commander**.
- **Standard**: 2-player life tracker, both players start at 20.
  - Player 1 (top number, `< ... >` indicator): **Left** = -1, **Right** = +1.
  - Player 2 (bottom number, `^ ... v` indicator): **Up** = +1, **Down** = -1.
  - Hold a direction to keep adjusting. **OK** resets both to 20, **Back** returns to the menu.
- **Commander**: 4-player life tracker, all players start at 40.
  - **Select mode**: move the frame with the **arrow keys**, **OK** to edit the
    framed total, **Back** returns to the menu.
  - **Adjust mode** (framed total is inverted): **Up** = +1, **Down** = -1
    (hold to keep adjusting), **OK** or **Back** returns to select mode.
  - There is no reset inside Commander mode — go back to the menu and re-enter
    to start a new game at 40.

## Project layout

```text
.
├── application.fam   # Flipper app manifest (build config)
├── magic_life.c      # App source (menu + standard tracker + commander grid)
├── magic_life.png    # 10x10 app-list icon (letter M, black on white)
└── README.md
```

## Prerequisites

- A Flipper Zero (stock or Momentum firmware) and a USB cable.
- Python 3.8+ with `pip`.
- [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) (micro Flipper Build Tool),
  plus the image libraries the app icon build step needs:

```sh
pip install ufbt pillow heatshrink2
```

- Optional: [qFlipper](https://flipperzero.one/update) for manual file install, and
  VS Code (run `ufbt vscode_dist` in this folder for IDE integration).

> If `ufbt` is not recognized in your terminal, use `python -m ufbt` instead —
> it does exactly the same thing. (This usually means the `ufbt.exe` launcher is
> missing from Python's `Scripts` folder; `pip install --force-reinstall ufbt`
> normally restores it.)

## Build

1. **Pick the SDK matching your firmware** (one-time setup — uFBT remembers it):

   ```sh
   # Momentum firmware:
   ufbt update --index-url=https://up.momentum-fw.dev/firmware/directory.json --channel=release

   # Stock firmware:
   ufbt update --channel=release
   ```

   The first run downloads the SDK and the ARM toolchain automatically.

2. **Compile**, from this folder:

   ```sh
   ufbt
   ```

   A successful build ends with an `APPCHK` line, e.g. `Target: 7, API: 87.1`
   (Momentum) — the number must be no newer than your firmware's API, otherwise
   the Flipper warns the app is "too new". The compiled app is written to
   `dist/magic_life.fap`.

## Deploy to the device

Pick one option:

### Option A: Upload and run in one step (recommended)

Connect the Flipper Zero over USB (close qFlipper first — only one tool can use
the serial port at a time), then:

```sh
ufbt launch
```

This rebuilds if needed, uploads, and immediately starts the app on the device.

### Option B: Permanent install with qFlipper

1. Build the app (`ufbt`).
2. Open qFlipper, go to **File Manager > SD Card > apps > Games**
   (create the `Games` folder if it does not exist).
3. Drag `dist/magic_life.fap` into that folder.
4. On the Flipper, open **Apps > Games > MTG Life Tracker**.

## Troubleshooting

- **"App too new" warning on the Flipper**: the app was built against a newer SDK
  than the firmware supports. Switch to the matching SDK (see Build step 1) and
  rebuild.
- **`ufbt` not recognized**: use `python -m ufbt`, or reinstall to restore the
  launcher (`pip install --force-reinstall ufbt`) and open a fresh terminal.
- **Flipper not detected**: try another USB cable/port and make sure qFlipper is
  closed while `ufbt launch` runs.
- **First build fails with a toolchain error** (e.g. a missing `as.exe` right
  after the toolchain downloads): just run `ufbt` again — the toolchain
  occasionally needs a second run to settle after a fresh download.

