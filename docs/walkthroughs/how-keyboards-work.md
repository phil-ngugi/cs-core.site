# How Keyboards Work

---

## 1. Hardware — Key Detection

- Under each key is a switch that closes on press
- The keyboard's onboard chip detects which key was pressed by sending electrical pulses along a grid of rows and columns — a completed circuit at an intersection identifies the key
- This scanning happens continuously in a loop — typically 1000 times per second (1000Hz polling rate)
- The chip debounces the signal — physical switches bounce (rapidly open/close) on press, the firmware filters this out to register a single clean keypress

---

## 2. Scancode — From Keyboard to Kernel

- The chip transmits a **scancode** to the computer via USB (or PS/2, Bluetooth, etc.) by raising a hardware interrupt
- A scancode is a raw number identifying the physical key position — it says nothing about what character that key represents
- The same physical key produces different scancodes on different keyboard layouts (QWERTY vs AZERTY etc.)
- USB keyboards send scancodes as HID (Human Interface Device) reports — a standard protocol so any USB keyboard works without a custom driver

---

## 3. Kernel — Interrupt Handling

- The CPU stops its current work, saves its state, and jumps to the kernel's interrupt handler
- The kernel's keyboard driver handles the interrupt and converts the raw scancode into a **keycode** — a logical, layout-independent identifier for the key
- The kernel then applies the **keymap** — a table that maps keycodes to actual characters, accounting for modifier keys (Shift, Ctrl, Alt, CapsLock)
- On Linux this is handled by the `evdev` subsystem — the driver produces input events (`struct input_event`) placed on the input event queue
- The kernel generates two events per key: `KEY_DOWN` (press) and `KEY_UP` (release) — applications see both

---

## 4. From Kernel to Windowing System

- On Linux, the windowing system (X11 or Wayland) owns the input devices
- X11: the X server reads from `/dev/input/eventX` directly, translates keycodes into X key events, and routes them to the focused window
- Wayland: the compositor reads input events and routes them to the focused surface — there is no separate server, the compositor handles everything
- On Windows: the kernel delivers a `WM_KEYDOWN` message to the focused window's message queue via the Win32 subsystem
- On macOS: the event is delivered to the focused application via the `NSEvent` system

---

## 5. Routing to the Right Application — Focus

- The OS uses the concept of **keyboard focus** to decide where to send the keycode
- The window manager tracks whichever window the user last interacted with and routes input to that window's process
- Focus is entirely a userspace/windowing-system convention — the kernel has no concept of it
- Each application maintains its own **focus tree** internally — the windowing system delivers the event to the focused application, and the application further routes it to the focused widget (a text input, a button etc.)
- If you build a GUI from scratch you must maintain this focus state yourself — it is just a pointer to whichever widget currently owns input

---

## 6. Terminal Applications — Different Path

- In a terminal, the terminal emulator (e.g. iTerm2, GNOME Terminal) receives the keypress as a GUI app via the normal focus mechanism
- It does not handle the key itself — it writes the key as bytes into a **PTY** (pseudo-terminal), a kernel-managed pair of file descriptors
- The kernel's **line discipline** sits between the two sides — it handles buffering, echoing characters, and translating special keys into signals (Ctrl+C → `SIGINT`, Ctrl+Z → `SIGTSTP`, Ctrl+D → EOF)
- The shell or application running inside the terminal reads raw bytes from the slave PTY via `read()` on stdin — it never sees a "keypress", just bytes
- Non-character keys (arrows, F-keys) are sent as **escape sequences** — multi-byte strings starting with `\x1b[` (e.g. up arrow = `\x1b[A`) that the application must parse itself
- Libraries like **ncurses** and **readline** exist specifically to abstract over this complexity

---

## 7. The Full Path

```
key press
    ↓
keyboard switch closes
    ↓
onboard chip detects via row/column grid → scancode
    ↓
USB HID report sent to host → hardware interrupt
    ↓
kernel interrupt handler → keycode (layout independent)
    ↓
kernel applies keymap → character (accounting for modifiers)
    ↓
windowing system (X11 / Wayland / Win32) → focused window's event queue
    ↓
application event loop → focused widget
    ↓
character inserted into text buffer, caret advances, screen redraws
```