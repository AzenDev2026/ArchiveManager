# ArchiveManager

A Windows 11 Explorer style file manager built on the real Dolphin engine (KDE).
Tabs, command bar, breadcrumb address bar, navigation sidebar, search, and
dark/light themes (dark is the default). Folders open in the vertical list view.

## Built-in tools (1.2)

- **Editable address bar** — click the breadcrumb (or press `Ctrl+L`) and type
  any path, with **folder autocomplete** as you type. Enter navigates there;
  a friendly message appears if the folder does not exist. In narrow windows
  long paths collapse to `root › … › last folders` instead of crowding.
- **Code editor** — open any file in an editor tab (KTextEditor): syntax
  highlighting, line numbers, Save, and a **Run** button that saves and runs
  the file in the built-in terminal (`.sh`, `.py`, `.js`, `.c`, `.cpp`).
- **Terminal panel** — a Konsole terminal docked at the bottom of the window,
  always one keystroke away (`Ctrl+\``). "cd here" switches it to the folder
  open in the current tab.
- **Responsive command bar** — in narrow windows the command buttons drop
  their text labels to icons and the search field shrinks, so nothing ever
  gets squeezed or clipped (Windows 11 style).

## Requirements (Arch / NVazen)

- dolphin (provides the `dolphinpart` engine)
- konsole (provides the `konsolepart` terminal engine)
- ktexteditor (code editor)
- qt6-base, kparts, kio, kxmlgui
- cmake, gcc, make

Install missing packages:

```sh
sudo pacman -S --needed dolphin konsole ktexteditor qt6-base kparts kio kxmlgui extra-cmake-modules cmake gcc make
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
| `--edit <file>` | Open a file in the built-in editor at startup |
| `--terminal` | Show the built-in terminal panel at startup |
| `--theme dark\|light` | Start with a specific theme |
| `--screenshot <file.png>` | Save a screenshot of the window and exit |
| `--dump-actions` | Print the actions the Dolphin engine provides and exit |

## Keyboard shortcuts

- `Ctrl+T` new tab · `Ctrl+W` close tab · `Ctrl+Tab` next tab
- `Ctrl+E` open the editor · `Ctrl+\`` show/hide the terminal
- `Ctrl+L` focus address bar · `Ctrl+F` focus search · `Ctrl+S` save in the editor
- `Alt+Left / Alt+Right / Alt+Up` back / forward / up

## GitHub

https://github.com/AzenDev2026/ArchiveManager
