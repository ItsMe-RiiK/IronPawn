#pragma once

#include "../http/LichessClient.h"
#include "IPlatform.h"

#include <memory>

class LichessPlatform : public IPlatform
{
public:
  LichessPlatform(const std::string &token);
  ~LichessPlatform() override;

  void startListening(std::function<void(const ChessEvent &)> callback) override;
  bool acceptChallenge(const std::string &challengeId) override;
  void makeMove(const std::string &gameId, const std::string &move) override;

private:
  std::unique_ptr<LichessClient> client;
  std::string botId;
  std::function<void(const ChessEvent &)> eventCallback;

  // Thread management for specific games
  void streamGame(const std::string &gameId);
};
