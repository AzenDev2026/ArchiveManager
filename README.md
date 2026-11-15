# Dolphin Shell

A Windows 11 Explorer style shell for Dolphin (the KDE file manager).
The real Dolphin engine runs inside a Win11-style window with tabs,
command bar, breadcrumb address bar, sidebar, and dark/light themes
(dark is the default). Folders open in the vertical list view (Win11 style);
you can still switch view modes from the View menu.

The launcher uses the system file-manager icon. To install the menu entry
and a desktop shortcut:

```sh
cp data/org.azendev.DolphinShell.desktop ~/.local/share/applications/
cp data/org.azendev.DolphinShell.desktop ~/Desktop/
chmod +x ~/Desktop/org.azendev.DolphinShell.desktop
```

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
./build/dolphin-shell
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
