#pragma once

#include <functional>
#include <string>

enum class ChessEventType
{
  CHALLENGE_RECEIVED,
  GAME_STARTED,
  MOVE_RECEIVED,
  GAME_ENDED
};

struct ChessEvent
{
  ChessEventType type;
  std::string gameId;
  std::string challengeId;
  std::string moves; // Space separated e.g. "e2e4 e7e5"
  bool isBotTurn;
  std::string status; // "started", "mate", "resign"
  int elo = -1; // -1 means use the base config ELO
  bool isWhite = true;
};

class IPlatform
{
public:
  virtual ~IPlatform() = default;

  // Connect to the platform and start listening. Blocks or runs in background.
  // The callback is triggered whenever something happens (challenge, move, etc).
  virtual void startListening(std::function<void(const ChessEvent &)> callback) = 0;

  // Actions the engine/bot can take
  virtual bool acceptChallenge(const std::string &challengeId) = 0;
  virtual void makeMove(const std::string &gameId, const std::string &move) = 0;
};
