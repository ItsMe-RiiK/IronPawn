# IronPawn

> **⚠️ IMPORTANT DISCLAIMER:** This project is **NOT** for cheating on Lichess. Using an engine on a normal human account will result in an immediate and permanent ban. This tool is designed *exclusively* to interface with Lichess's official [Bot API](https://lichess.org/api#tag/bot), allowing you to create a dedicated, legally marked "Bot Account".

IronPawn is a lightweight, fully cross-platform C++ "bridge" application designed to connect a local UCI chess engine (like Stockfish) directly to **three** different platforms simultaneously:
1. **Discord Activities:** Play chess directly inside Discord text channels using a seamless embedded GUI via Discord's HTTP Interactions API.
2. **Local Web Server:** A standalone interactive web GUI that allows anyone on your network to play against Stockfish via `http://localhost:8080`.
3. **Lichess Bot API:** Runs as a legally registered bot account on Lichess so people can challenge your bot online.

It runs locally on Linux and Windows, continuously listens for events across all platforms, and manages bi-directional UCI communication with the Stockfish engine using subprocess pipes.

## Features
- **Multi-Platform Support:** Seamlessly handles concurrent games on Discord, Lichess, and Local Web.
- **Cross-Platform Compilation:** Native support for both **Linux** (via POSIX `fork`/`pipe`) and **Windows** (via Win32 `CreateProcess`/`CreatePipe`).
- **Interactive Frontend:** Built-in web UI using `chessboard.js` and `chess.js`, served statically from the `/public` directory.
- **Modern C++ Architecture:** Uses `httplib` for HTTP Server/Client, `dpp` (D++ library) for Discord integration, and `nlohmann/json` for robust API parsing.
- **Memory & Thread Safe:** Engineered with strict mutex locking, detached thread offloading for heavy calculations, and graceful resource cleanup to prevent zombie processes and memory leaks on abandoned games.

## Prerequisites
- A local installation of Stockfish
  - **Ubuntu/Debian:** `sudo apt install stockfish`
  - **Arch Linux:** `sudo pacman -S stockfish` or `yay -S stockfish`
  - **Windows:** Download from [stockfishchess.org](https://stockfishchess.org/) and extract the `.exe`.
- **(For Compiling Manually):** `cmake`, a C++ compiler (`gcc`/`MSVC`), `libcurl`, `nlohmann-json`, `cpp-httplib`, and `dpp`.

## Build Instructions

### Linux Build (Ubuntu / Debian & Arch Linux)
1. Install dependencies:
   - **Ubuntu/Debian:**
     ```bash
     sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev cmake g++
     ```
   - **Arch Linux:**
     ```bash
     sudo pacman -S curl nlohmann-json cmake gcc
     ```
   > **Note for other distros (Fedora, openSUSE, macOS, etc.):** Please use your system's respective package manager (`dnf`, `zypper`, `brew`, etc.) to install equivalent packages for `curl`, `nlohmann-json`, `cmake`, and a C++ compiler.

2. Generate build files and compile:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make -j$(nproc)
   ```

### Windows Build (Visual Studio)
1. Install **Visual Studio** with the "Desktop development with C++" workload.
2. Install `vcpkg` (C++ package manager) and install dependencies:
   ```cmd
   vcpkg install curl nlohmann-json dpp cpp-httplib --triplet x64-windows
   ```
3. Open the repository folder in Developer Command Prompt:
   ```cmd
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

### Safe Compilation Script (`run.sh`)
During development or debugging, it is common to recompile the bot frequently. To prevent orphaned "zombie" processes from lingering in the background (which hijack network ports and cause undefined engine behavior), always use the provided `run.sh` script to safely test your changes:
```bash
chmod +x run.sh
./run.sh
```

## Configuration

IronPawn is completely driven by a centralized configuration file.

1. Copy the example configuration file:
   ```bash
   cp config.example.jsonc config.jsonc
   ```
2. Open `config.jsonc` and configure your settings:
   - **`engine.path`**: Absolute path to your Stockfish executable (e.g. `/usr/bin/stockfish`).
   - **`engine.elo`**: Desired engine difficulty (e.g. `1500`).
   - **`engine.thinkTimeMs`**: Maximum time the bot is allowed to think per move.
   - **`lichess.token`**: Your Lichess Bot API token (leave empty to disable Lichess integration).
   - **`discord.token`**: Your Discord Bot token (leave empty to fallback to Local Web API only).

## Running the Bot

> **CRITICAL FOR LICHESS:** Ensure your Lichess account has actually been upgraded to a "BOT" account before playing online! You can do this by running a one-time command in your terminal using your Lichess Token:
> `curl -d "" -H "Authorization: Bearer YOUR_TOKEN" https://lichess.org/api/bot/account/upgrade`

Once `config.jsonc` is configured, simply run the executable:
```bash
./build/IronPawn
```

### How to Play
Currently available from these platfrom listed:
- **Local Web:** Open a browser and navigate to `http://localhost:8080`.
- **Discord:** Type `!play` in any text channel where the bot is invited. The bot will reply with a self-destructing link to launch the embedded Activity GUI.
- **Lichess:** Challenge your bot directly on Lichess.

## LICENSE
This project is licensed under the terms of the **MIT** license. See the [LICENSE](LICENSE) file for details.

## TOS and Privacy Policy

See [Term of Service](TermOfService.md) and [Privacy Policy](PrivacyPolicy.md).