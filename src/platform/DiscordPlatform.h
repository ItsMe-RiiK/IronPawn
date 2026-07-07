#pragma once

#include "IPlatform.h"

#include <dpp/dpp.h>
#include <httplib.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class DiscordPlatform : public IPlatform
{
public:
  DiscordPlatform(const std::string &token);
  ~DiscordPlatform() override;

  void startListening(std::function<void(const ChessEvent &)> callback) override;
  bool acceptChallenge(const std::string &challengeId) override;
  void makeMove(const std::string &gameId, const std::string &move) override;

private:
  std::unique_ptr<dpp::cluster> bot;
  std::function<void(const ChessEvent &)> eventCallback;

  std::unordered_map<std::string, std::string> activeGames;
  std::unordered_map<std::string, std::string> latestBotMoves;
  std::mutex gamesMutex;

  httplib::Server svr;
  std::thread webThread;
};
