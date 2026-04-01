<p align="center">
  <img
    src="polyedit.png"
    alt="polyedit"
    width="120"
  />

</p>

<h1 align="center">polyedit</h1>

<p align="center">
   Minimal; simple.
</p>

> [!CAUTION]
> polyedit is unfinished. Expect bugs; do not use this for high value files. Linux only.
---

## What it is:
- Minimal TUI editor.
- Developed for systems based on the Linux kernel. Compatibility with other operating systems (e.g., Windows, BSD) is neither guaranteed nor supported.
- Aims to be lighter than alternatives like nano, micro, emacs and vim while maintaining speed.

## Features:
- Full keybind support NOT added yet.
- Basic keybinds (C- refers to CTRL):
  - `C-s` -> save file
  - `C-c` -> quit w/o saving

- Movement keybinds:
  - `C-n` -> next line
  - `C-p` -> previous line
  - `C-b` -> back one character
  - `C-f` -> forward one character
  - `C-j` -> terminal default bound to RET (Enter)
  - Arrow keys are also supported.

- Note: TAB is automatically set to 4 spaces.
- Partial (dumb) syntax highlighting targeted for the C language. It can be turned off with `-n` flag after the file specification.

## Installation:
- `git clone https://github.com/Polymorqhism/polyedit`
  - Clone the GitHub repo.
- `cd polyedit`
  - Change directory into the cloned repo.
- `make`
  - Use the Makefile to make the project into a binary.

- The built binary resides in `build/polyedit`. Optionally, copy it to `/usr/bin/` to access it globally.
  - `sudo cp build/polyedit /usr/bin/`


## Possible Issues:
- If the terminal running this is too small, you may encounter issues.

---
v1.1.2-beta
- Starting from v1.0.0-beta, polyedit will be written using polyedit itself.
- GPL-3.0 license. See LICENSE for more information.
