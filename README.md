# IronPawn

## Description

> **⚠️ IMPORTANT DISCLAIMER:** This project is **NOT** for cheating on Lichess. Using an engine on a normal human account will result in an immediate and permanent ban. This tool is designed *exclusively* to interface with Lichess's official [Bot API](https://lichess.org/api#tag/bot), allowing you to create a dedicated, legally marked "Bot Account". People can then play against your local bot on Lichess to learn chess, test engines, or just have fun!

IronPawn is a lightweight, fully cross-platform C++ "bridge" between chess engine and Lichess Bot API, an application designed to connect a local UCI chess engine (like Stockfish or any other UCI engine) directly to the Lichess Bot API, so the engine plays as a legally registered bot account on Lichess. See [Understanding the Lichess API](https://lichess.org/api), and see [How to setup the bot account on Lichess](#-running-the-bot)

It runs on Linux and Windows locally, continuously listens to the Lichess server for events via an NDJSON stream, and acts as the bridge. Whenever a challenge is received, it is automatically accepted. Once the game starts, the bridge spawns a local instance of the Stockfish engine in a subprocess, manages the bi-directional UCI communication through standard Pipes, and seamlessly proxies moves between the Lichess server and the Stockfish engine.

## Features
- **Cross-Platform:** Native support for both **Linux** (via POSIX `fork`/`pipe`) and **Windows** (via Win32 `CreateProcess`/`CreatePipe`).
- **HTTP Streaming:** Uses `libcurl` to handle asynchronous Server-Sent Events (SSE) / NDJSON streams from Lichess.
- **JSON Parsing:** Robust Lichess API communication parsed using `nlohmann/json`.
- **Multi-Game Support:** Handles active games concurrently using `std::thread` while keeping the main event stream open.
- **Memory & Thread Safe:** Engineered to prevent zombie processes, file descriptor leaks, and `SIGALRM` DNS-resolution crashes during multi-threading.
- **Automated Builds:** Pre-compiled binaries for Windows and Linux are automatically generated using GitHub Actions.

## Prerequisites
- A local installation of Stockfish
  - **Linux:** `sudo apt install stockfish` or `yay -S stockfish`
  - **Windows:** Download from [stockfishchess.org](https://stockfishchess.org/) and extract the `.exe`.
- **(For Compiling Manually):** `cmake`, a C++ compiler (`gcc`/`MSVC`), `libcurl`, and `nlohmann-json`.

## How to Get the App

### Option A: Download Pre-compiled Binary (Easiest)
Whenever code is updated, GitHub Actions automatically builds the latest version.
1. Go to the **Actions** tab on this GitHub repository.
2. Click on the latest successful workflow run.
3. Scroll down to the **Artifacts** section.
4. Download `IronPawn-Linux` or `IronPawn-Windows`.

### Option B: Build from Source
If you prefer to compile it yourself:

#### Linux Build Instructions
1. Install dependencies (e.g. on Debian/Ubuntu):
   ```bash
   sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev cmake g++
   ```
2. Generate build files and compile:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make -j4
   ```

#### Windows Build Instructions (Using Visual Studio)
1. Install **Visual Studio** with the "Desktop development with C++" workload.
2. Install `vcpkg` (C++ package manager) and install dependencies:
   ```cmd
   vcpkg install curl nlohmann-json:x64-windows
   ```
3. Open the repository folder in Visual Studio (or use Developer Command Prompt):
   ```cmd
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
   cmake --build build --config Release
   ```

## Configuration

Before running the bot, you must set your Lichess Bot API token (with the `bot:play` scope). You can configure the bot in one of three ways:

1. **Hardcode in Source Code:**
   Open `src/main.cpp` and replace the token string. Recompile afterwards.
2. **Environment Variables:**
   ```bash
   export LICHESS_BOT_TOKEN="your_token_here"
   export STOCKFISH_PATH="/path/to/stockfish"
   ```
3. **Command Line Arguments (Recommended for downloaded binaries):**
   ```bash
   # On Linux
   ./IronPawn <YOUR_LICHESS_BOT_TOKEN> /usr/bin/stockfish
   
   # On Windows
   IronPawn.exe <YOUR_LICHESS_BOT_TOKEN> C:\path\to\stockfish.exe
   ```

### Changing Engine Difficulty

You can adjust how Stockfish plays directly in the code in `src/bot/Bot.cpp` (requires recompiling).
* **Strength limit:** Search for `engine.setElo(1500)` to set a specific Elo. 
* **Time limits / Depth:** Search for `thinkTimeMs` and `depth` inside the file to change how long Stockfish thinks per move. 

## Running the Bot

> **CRITICAL:** Ensure your Lichess account has actually been upgraded to a "BOT" account! You can do this by running a one-time command in your terminal:
> `curl -d "" -H "Authorization: Bearer YOUR_TOKEN" https://lichess.org/api/bot/account/upgrade`

Once configured, simply run the executable and challenge your bot on Lichess!

## LICENSE
This project is licensed under the terms of the **MIT** license. See the [LICENSE](LICENSE) file for details.