# ArchiveManager

A Windows 11 Explorer style file manager built on the real Dolphin engine (KDE).
Tabs, command bar, breadcrumb address bar, navigation sidebar, search, and
dark/light themes (dark is the default). Folders open in the vertical list view.

## Requirements (Arch / NVazen)

- dolphin (provides the `dolphinpart` engine)
- qt6-base, kparts, kio, kxmlgui
- cmake, gcc, make

Install missing packages:

```sh
sudo pacman -S --needed dolphin qt6-base kparts kio kxmlgui extra-cmake-modules cmake gcc make
```

## Build

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

## Run

```sh
./build/archive-manager
```

## Options

| Flag | What it does |
| --- | --- |
| `--url <location>` | Open a location at startup (e.g. `/home/you/Documents`, `trash:/`) |
| `--theme dark\|light` | Start with a specific theme |
| `--screenshot <file.png>` | Save a screenshot of the window and exit |
| `--dump-actions` | Print the actions the Dolphin engine provides and exit |

## Keyboard shortcuts

- `Ctrl+T` new tab · `Ctrl+W` close tab · `Ctrl+Tab` next tab
- `Ctrl+L` focus address bar · `Ctrl+F` focus search
- `Alt+Left / Alt+Right / Alt+Up` back / forward / up

## GitHub

https://github.com/AzenDev2026/ArchiveManager
