#include "bot/Bot.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
  // --- CONFIGURATION ---

  // Method 3: Hardcode in Source Code
  // You can permanently write your Lichess API token here so you don't have to pass it in the terminal.
  std::string token = "YOUR_LICHESS_BOT_TOKEN_HERE";
  // The path to your stockfish executable. "stockfish" works if it's already installed in your system PATH.
  std::string enginePath = "stockfish";

  // Method 2: Environment Variables
  // (This will automatically override the hardcoded token above if they exist)
  const char *envToken = std::getenv("LICHESS_BOT_TOKEN");
  if (envToken)
  {
    token = envToken;
  }

  const char *envEngine = std::getenv("STOCKFISH_PATH");
  if (envEngine)
  {
    enginePath = envEngine;
  }

  // Method 1: Command Line Arguments
  // (This is the highest priority and will override both Environment Variables and the Hardcoded token)
  if (argc > 1)
  {
    token = argv[1];
  }
  if (argc > 2)
  {
    enginePath = argv[2];
  }

  if (token == "YOUR_LICHESS_BOT_TOKEN_HERE" || token.empty())
  {
    std::cerr << "Usage: " << argv[0] << " <lichess_bot_token> [stockfish_path]" << std::endl;
    std::cerr << "Alternatively, hardcode the token in src/main.cpp!" << std::endl;
    return 1;
  }

  if (enginePath.empty())
  {
    enginePath = "stockfish"; // default to PATH
  }

  Bot bot(token, enginePath);
  bot.run();

  return 0;
}
