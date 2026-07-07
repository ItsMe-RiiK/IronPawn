#include "bot/Bot.h"
#include "platform/DiscordPlatform.h"
#include "platform/LichessPlatform.h"
#include "platform/LocalWebPlatform.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  std::string lichessToken = "";
  std::string discordToken = "";
  std::string enginePath = "stockfish";

  int elo = 1500;
  int thinkTimeMs = 1000;
  int depth = 15;

  std::ifstream f("config.jsonc");
  if (!f.is_open())
  {
    std::cerr << "Warning: config.jsonc not found! Please copy config.example.jsonc to config.jsonc and add your tokens." << std::endl;
  }
  else
  {
    try
    {
      // 4th parameter `true` enables parsing comments (// and /* */) in JSON
      json config = json::parse(f, nullptr, true, true);
      if (config.contains("lichess") && config["lichess"].contains("token"))
      {
        lichessToken = config["lichess"]["token"];
      }
      if (config.contains("discord") && config["discord"].contains("token"))
      {
        discordToken = config["discord"]["token"];
      }
      if (config.contains("engine"))
      {
        if (config["engine"].contains("path"))
          enginePath = config["engine"]["path"];
        if (config["engine"].contains("elo"))
          elo = config["engine"]["elo"];
        if (config["engine"].contains("thinkTimeMs"))
          thinkTimeMs = config["engine"]["thinkTimeMs"];
        if (config["engine"].contains("depth"))
          depth = config["engine"]["depth"];
      }
    }
    catch (...)
    {
    }
  }

  std::vector<std::thread> botThreads;

  if (!lichessToken.empty())
  {
    std::cout << "Starting Lichess Platform Thread..." << std::endl;
    botThreads.emplace_back(
        [=]()
        {
          std::unique_ptr<IPlatform> platform = std::make_unique<LichessPlatform>(lichessToken);
          Bot bot(std::move(platform), enginePath, elo, thinkTimeMs, depth);
          bot.run();
        });
  }

  if (!discordToken.empty())
  {
    std::cout << "Starting Discord (and Local Web API) Platform on main thread..." << std::endl;
    std::unique_ptr<IPlatform> platform = std::make_unique<DiscordPlatform>(discordToken);
    Bot bot(std::move(platform), enginePath, elo, thinkTimeMs, depth);
    bot.run(); // This blocks the main thread
  }
  else
  {
    std::cout << "Discord token not found. Starting Local Web Platform on main thread..." << std::endl;
    std::unique_ptr<IPlatform> platform = std::make_unique<LocalWebPlatform>();
    Bot bot(std::move(platform), enginePath, elo, thinkTimeMs, depth);
    bot.run(); // This blocks the main thread
  }

  for (auto &t : botThreads)
  {
    if (t.joinable())
      t.join();
  }

  return 0;
}
