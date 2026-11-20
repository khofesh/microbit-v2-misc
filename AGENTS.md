# agents

## Response Style

- Be extremely concise. No pleasantries, no filler.
- When asked to write code, return code only unless explanation is explicitly requested.
- No sycophantic preambles ("Sure!", "Great question!", "Absolutely!").
- No "Here's a function that..." preambles.
- Don't restate the question before answering.
- No "Note:", "Tip:", or "Remember:" appendices unless asked.
- No usage examples unless asked.
- No unsolicited suggestions or improvements beyond what was asked.
- Use short variable names where meaning is clear from context.

## Project Structure

Fork of `lancaster-university/microbit-v2-samples`. Target: BBC micro:bit V2 (nRF52833, Cortex-M4F) using the CODAL C++ runtime. Build: CMake + arm-none-eabi-gcc, driven by `build.py`.

| Path | Owner | Edit |
|---|---|---|
| `source/main.cpp` | User | Yes — program entry (`MicroBit uBit; int main()`) |
| `source/*.{h,c,cpp}` | User | Yes — every file under `source/` is compiled automatically |
| `source/samples/` | Upstream | Reference samples (`Tests.h` declares them); call from `main.cpp`, don't rewrite |
| `codal.json` | User | Yes — target + CODAL `config` flags (`MICROBIT_BLE_ENABLED`, ...) |
| `codal.dev.json`, `codal.ble.json` | Upstream | Templates to copy into `codal.json` (debug/DMESG, BLE) |
| `libraries/codal-*` | Upstream (git submodules) | Never — read-only API reference |
| `CMakeLists.txt`, `build.py`, `utils/` | Upstream | Rarely |
| `.vscode/` | User | IntelliSense (`build/compile_commands.json`), cortex-debug launch configs |
| `build/` | Generated | Never |
| `MICROBIT.hex`, `MICROBIT.bin` | Generated | Never — build output |

Submodules: `codal-microbit-v2` (board: `MicroBit`, display, buttons, radio, audio), `codal-core` (drivers, events, fibers), `codal-nrf52` (MCU HAL, nrfx), `codal-microbit-nrf5sdk` (SoftDevice/BLE). Keep submodule commits in line with `libraries/codal-microbit-v2/target.json`.

## Build & Flash

```bash
python build.py                 # -> MICROBIT.hex in repo root
python build.py --clean         # clean rebuild
cp MICROBIT.hex /run/media/$USER/MICROBIT/ && sync    # flash via DAPLink drive
pyocd flash -t nrf52833 MICROBIT.hex                  # flash via pyOCD
```

After adding/removing files under `source/`, re-run CMake configure (VS Code task "IntelliSense: regenerate compile database") so `compile_commands.json` stays current.

## Code Style

- C++ (CODAL), formatting per `.clang-format`: 4-space indent, Allman braces, 100 cols
- One global `MicroBit uBit;` in `main.cpp`; call `uBit.init()` first; other files use `extern MicroBit uBit;`
- Never return from `main()` — end in a loop, `release_fiber()`, or `microbit_panic()`
- Use `uBit.sleep(ms)` (fiber-aware), not busy-wait delays
- Event-driven code: `uBit.messageBus.listen(id, value, handler)`; keep handlers short, no blocking work in IRQ context
- Use `create_fiber()` for concurrent tasks; CODAL scheduler is cooperative — long loops must yield (`uBit.sleep`/`schedule()`)
- Use CODAL APIs (`uBit.display`, `uBit.io.P0`, `uBit.i2c`, `uBit.radio`, ...) before touching nrfx/registers
- Check `DEVICE_OK` return codes from CODAL calls
- Prefer static/stack buffers; RAM is 128 KB shared with CODAL and SoftDevice (when BLE on). Use `ManagedString`/`ManagedBuffer` over raw `new`
- BLE and radio are mutually exclusive; toggle via `codal.json` config, not `#define` in source
- Fixed-width types from `<stdint.h>` for hardware/data values
- `static` for file-local functions and variables
- Debug output: `DMESG(...)` (needs `DMESG_ENABLE` in `codal.json`) or `uBit.serial.printf(...)`
- Minimal inline comments — only for genuinely complex logic or hardware quirks

## Context Retrieval Policy

Always retrieve the smallest amount of information necessary.
Escalate only when necessary. Stop escalating as soon as sufficient information has been obtained.

Preferred order:

1. Need code structure? `graph.json`.
2. Need symbols? `ast-grep`.
3. Need repository metadata? `rtk`.
4. Need implementation? Source files.
5. Use repository-wide search as last resort

Avoid reading entire directories or the whole repository unless explicitly requested.
For CODAL APIs, read the specific header in `libraries/` (e.g. `libraries/codal-microbit-v2/model/MicroBit.h`) — never scan the submodules.

## RTK (Rust Token Killer) - Token-Optimized CLI

`rtk` is a CLI proxy that filters and compresses command outputs, saving 60-90% tokens.

**Always** prefix commands with `rtk`. If RTK has a dedicated filter, it uses it. If not, it passes through unchanged. This means RTK is always safe to use.

Instead of:

```
ls -la .
git status
git log -10
docker ps
```

Use:

```
rtk ls -la .
rtk git status
rtk git log -10
rtk docker ps
```

### RTK commands example

```bash
rtk ls <path>
rtk read <file>
rtk find <pattern>

rtk err <cmd>           # Filter errors only from any command
rtk log <file>          # Deduplicated logs with counts
rtk json <file>         # JSON structure without values

rtk curl <url>          # Compact HTTP responses

rtk docker ps           # Compact container list
rtk docker images       # Compact image list
rtk docker logs <c>     # Deduplicated logs
```

## Graphify - Codebase context & knowledge graph protocol

A pre-computed AST knowledge graph (`graph.json`) is available at: `graphify-out/graph.json`
Use `graph.json` before searching or reading multiple source files.

Scope: `source/` and `build.py`. `libraries/`, `utils/`, `build/` are excluded via `.graphifyignore`.
Rebuild after code changes: `graphify update .`

Workflow:

1. Read `graph.json`.
2. Identify the relevant symbols, files, and dependency paths.
3. Read only the source files required for the task.
4. Avoid scanning unrelated files.

Use the graph for:

- dependency tracing
- call-path discovery
- identifying high-centrality modules
- impact analysis before refactoring
- locating symbol definitions

Never read an entire directory simply to locate a symbol. Use `graph.json` to locate the symbol first, then read only the relevant files.

The graph is generated from AST extraction and represents extracted structural relationships only. Treat graph edges as authoritative for structural relationships.
Do not infer dependencies that are absent from the graph.

## ast-grep

Prefer `ast-grep` over `grep` whenever searching source code.

Use `ast-grep` for:

- locating function definitions
- locating class definitions
- locating method implementations
- finding imports
- matching AST patterns
- performing structural search

Use `grep` only for:

- Markdown
- JSON
- YAML
- log files
- generated text
- plain-text configuration

Avoid using `grep` to search programming language source code when `ast-grep` can answer the query.
