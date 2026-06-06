# LedArt

An ESP32-based LED wall art installation using WS2812B LEDs and sideglow fiber optics arranged in octagons.

Each octagon contains 32 WS2812B LEDs (4 per side). Each pair of LEDs is connected by a single glass fiber, making 16 fibers per octagon. Both LEDs of a pair always share the same color, so each fiber glows as one uniform color.

Inspired by: https://www.instructables.com/Fiber-Optic-and-LEDs-a-Wall-Decoration/

---

## Hardware

- **MCU:** ESP32 (Lolin32)
- **LEDs:** WS2812B, 32 per octagon
- **Layout:** 4 LEDs per side × 8 sides = 32 LEDs = 16 fiber pairs
- **Fiber:** Sideglow glass fiber, one per LED pair

### Wiring tips

- Place a **300–500Ω resistor** on each data line close to the ESP32 pin
- Add a **100µF capacitor** across the 5V supply near each strip to absorb current spikes
- Power the LED strips from a **dedicated 5V supply** — do not use the ESP32's onboard 5V/3.3V pins for more than a few LEDs
- ESP32 outputs 3.3V logic; WS2812B usually accepts this. If colors are glitchy, add a level shifter or a diode in series on the 5V supply to drop it to ~4.3V

### Safe GPIO pins for LED data lines

| Safe pins | Notes |
|-----------|-------|
| 4, 13, 14, 15, 16, **17**, 25, 26, 27, 32, 33 | No important default functions |

| Pins to avoid | Reason |
|---------------|--------|
| 6 – 11 | Connected to internal flash |
| 34 – 39 | Input only |
| 1, 3 | UART0 TX/RX (serial monitor) |
| 18, 19, 23, 5 | Default VSPI (SCK, MISO, MOSI, CS) |

Current project uses **GPIO 17** for octagon 1.

---

## Power consumption

WS2812B draws up to **60mA per LED** at full white (all channels 255). Typical colorful animations draw around **20mA per LED**.

| Octagons | Max current (A) | Typical current (A) | Max power @ 5V (W) |
|----------|----------------|---------------------|-------------------|
| 1 | 1.9 | 0.6 | 9.6 |
| 2 | 3.8 | 1.3 | 19.2 |
| 4 | 7.7 | 2.6 | 38.4 |
| 6 | 11.5 | 3.8 | 57.6 |
| 8 | 15.4 | 5.1 | 76.8 |

Size your 5V PSU for **max current** even if typical usage is lower. For multiple octagons, inject power at multiple points along the strip rather than running all current through one end.

---

## Maximum octagons per ESP32

The limiting factor is the **RMT peripheral**, which generates WS2812B timing. The ESP32 has 8 RMT channels, so a single ESP32 can drive up to **8 octagons** (one data line each).

For more than 8 octagons, use a **master/slave architecture**: one ESP32 as the master (runs all pattern logic) and additional ESP32s as slaves (each drives up to 8 octagons). Master communicates with slaves over UART or ESP-NOW.

---

## Software architecture

```
main/
├── main.cpp                    # app_main, octagon configs, main loop
├── octagon/
│   ├── fiber.h                 # Color and Fiber structs
│   ├── octagon_config.h        # FiberConfig and OctagonConfig structs
│   ├── octagon.h / .cpp        # Octagon class
└── patterns/
    ├── pattern.h               # Abstract Pattern base class
    ├── gradual_shift.h / .cpp  # GradualShift pattern
```

### Class overview

- **`Color`** — RGB value (r, g, b as uint8_t)
- **`Fiber`** — runtime state: two LED indices + one color
- **`OctagonConfig`** — static wiring config: GPIO pin + 16 fiber pairs
- **`Octagon`** — owns the LED strip, 16 fibers, and the active pattern. Calls `pattern.update()` each frame then pushes fiber colors to both LEDs in each pair
- **`Pattern`** (abstract) — base class for all animations. Subclasses implement `begin()` and `update()`

### Main loop

Runs at ~60 fps. Each iteration:
1. Calculates `delta_ms` since last frame
2. Calls `octagon.update(delta_ms)` → pattern updates fiber colors
3. Calls `octagon.render()` → writes colors to LED strip hardware

### Configuring an octagon

In `main.cpp`, define an `OctagonConfig` with the GPIO pin and the 16 fiber pairs. Each pair is two LED indices (0–31) within that octagon's strip. Both LEDs in a pair always receive the same color.

```cpp
static const OctagonConfig OCTAGON_1 = {
    .gpio_pin = 17,
    .fibers = {
        {0, 29}, {1, 18}, ...  // 16 pairs, all 32 LEDs covered exactly once
    }
};
```

### Adding a new pattern

1. Create `main/patterns/my_pattern.h` and `my_pattern.cpp`
2. Inherit from `Pattern` and implement `begin()` and `update()`
3. Add the `.cpp` to `SRCS` in `main/CMakeLists.txt`
4. Instantiate with `octagon.setPattern(new MyPattern())`

```cpp
class MyPattern : public Pattern {
public:
    void begin(Octagon& octagon) override { /* init per-fiber state */ }
    void update(Octagon& octagon, uint32_t delta_ms) override {
        // write to octagon.fibers[i].color
    }
};
```

---

## Build & flash

### First-time setup

```bash
make setup
```

### Common commands

```bash
make build          # compile
make flash          # flash to /dev/ttyUSB1
make flash-monitor  # flash and open serial monitor
make monitor        # open serial monitor without flashing
make clean          # remove build artifacts (keeps config)
make fullclean      # full clean — use when switching Python env or after errors
```

### Changing the port

Edit the `PORT` variable at the top of the `Makefile`:

```makefile
PORT := /dev/ttyUSB1
```

### Manual (without make)

```bash
source /home/chris/.espressif/v6.0.1/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyUSB1 flash monitor
```

> If you see a Python virtual environment error, run `make setup` (or `make fullclean` if the project was previously built with a different Python version) before building.
