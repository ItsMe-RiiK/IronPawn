#ifndef BOT_H
#define BOT_H

#include "../engine/UciEngine.h"
#include "../platform/IPlatform.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class Bot
{
public:
  Bot(std::unique_ptr<IPlatform> platform, const std::string &enginePath, int elo, int thinkTimeMs, int depth);
  ~Bot();

  void run();

private:
  std::unique_ptr<IPlatform> platform;
  std::string enginePath;
  int elo;
  int thinkTimeMs;
  int depth;

  std::unordered_map<std::string, std::shared_ptr<UciEngine>> activeGames;
  std::mutex gamesMutex;

  void handleEvent(const ChessEvent &evt);
  void processMoveAsync(std::string gameId, std::string moves);
};

#endif // BOT_H
