#pragma once

#include "IPlatform.h"

#include <httplib.h>
#include <mutex>
#include <string>

class LocalWebPlatform : public IPlatform
{
public:
  LocalWebPlatform();
  ~LocalWebPlatform() override;

  void startListening(std::function<void(const ChessEvent &)> callback) override;
  bool acceptChallenge(const std::string &challengeId) override;
  void makeMove(const std::string &gameId, const std::string &move) override;

private:
  httplib::Server svr;
  std::function<void(const ChessEvent &)> eventCallback;

  std::string currentMoves;
  std::string latestBotMove;
  std::mutex gameMutex;
};
