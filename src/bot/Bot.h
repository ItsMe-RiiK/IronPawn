#ifndef BOT_H
#define BOT_H

#include "../engine/UciEngine.h"
#include "../http/LichessClient.h"

#include <memory>
#include <string>

class Bot
{
public:
  Bot(const std::string &token, const std::string &enginePath);
  ~Bot();

  void run();

private:
  std::string token;
  std::string enginePath;
  std::unique_ptr<LichessClient> client;

  // Handles the event stream (challenges, gameStarts)
  bool handleEvent(const std::string &eventJson, const std::string &botId);

  // Handles an individual game stream
  void playGame(const std::string &gameId, const std::string &botId);
};

#endif // BOT_H
