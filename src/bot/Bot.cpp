#include "Bot.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

Bot::Bot(const std::string &token, const std::string &enginePath) : token(token), enginePath(enginePath)
{
  client = std::make_unique<LichessClient>(token);
}

Bot::~Bot() {}

void Bot::run()
{
  std::cout << "Starting bot, fetching bot ID..." << std::endl;
  std::string botId = client->getBotId();
  if (botId.empty())
  {
    std::cerr << "Failed to fetch bot ID. Token might be invalid." << std::endl;
    return;
  }
  std::cout << "Authenticated as bot: " << botId << std::endl;
  std::cout << "Listening for events..." << std::endl;

  client->streamEvents([this, botId](const std::string &eventJson) -> bool { return handleEvent(eventJson, botId); });
}

bool Bot::handleEvent(const std::string &eventJson, const std::string &botId)
{
  try
  {
    json e = json::parse(eventJson);
    std::string type = e.value("type", "");

    if (type == "challenge")
    {
      std::string challengeId = e["challenge"]["id"];
      std::cout << "Received challenge: " << challengeId << std::endl;
      if (client->acceptChallenge(challengeId))
      {
        std::cout << "Accepted challenge: " << challengeId << std::endl;
      }
      else
      {
        std::cerr << "Failed to accept challenge: " << challengeId << std::endl;
      }
    }
    else if (type == "gameStart")
    {
      std::string gameId = e["game"]["id"]; // Usually the game ID is here, or in "gameId"
      if (e["game"].contains("gameId"))
      {
        gameId = e["game"]["gameId"];
      }
      std::cout << "Game started: " << gameId << std::endl;

      std::thread gameThread(&Bot::playGame, this, gameId, botId);
      gameThread.detach();
    }
  }
  catch (const json::exception &ex)
  {
    std::cerr << "JSON error: " << ex.what() << " in event: " << eventJson << std::endl;
  }
  return true; // Keep streaming
}

void Bot::playGame(const std::string &gameId, const std::string &botId)
{
  UciEngine engine(enginePath);
  if (!engine.start())
  {
    std::cerr << "Failed to start engine for game " << gameId << std::endl;
    return;
  }
  std::cout << "Engine started for game " << gameId << std::endl;

  // --- CONFIGURE BOT DIFFICULTY HERE ---
  // You can set the Elo rating (1320-3190): using elo is a standard way to set the difficulty of the engine.
  engine.setElo(1500);
  // Or you can set the Skill Level (0-20):
  // engine.setSkillLevel(0);

  engine.sendCommand("ucinewgame");

  LichessClient gameClient(token);

  bool isWhite = false;
  bool colorDetermined = false;

  gameClient.streamGame(gameId,
                        [&engine, &gameClient, &gameId, &botId, &isWhite, &colorDetermined](const std::string &gameEventJson) -> bool
                        {
                          try
                          {
                            json e = json::parse(gameEventJson);
                            std::string type = e.value("type", "");

                            if (type == "gameFull")
                            {
                              // Determine our color
                              std::string whiteId = e["white"].value("id", "");
                              std::string blackId = e["black"].value("id", "");
                              if (whiteId == botId)
                              {
                                isWhite = true;
                                colorDetermined = true;
                              }
                              else if (blackId == botId)
                              {
                                isWhite = false;
                                colorDetermined = true;
                              }
                              else
                              {
                                std::cerr << "Warning: botId " << botId << " not found in white/black players" << std::endl;
                              }
                            }

                            if (type == "gameFull" || type == "gameState")
                            {
                              std::string moves = "";
                              std::string status = "";

                              if (type == "gameFull")
                              {
                                moves = e["state"].value("moves", "");

                                if (e["state"]["status"].is_object())
                                {
                                  status = e["state"]["status"].value("name", "");
                                }
                                else if (e["state"]["status"].is_string())
                                {
                                  status = e["state"].value("status", "");
                                }
                              }
                              else if (type == "gameState")
                              {
                                moves = e.value("moves", "");

                                if (e["status"].is_object())
                                {
                                  status = e["status"].value("name", "");
                                }
                                else if (e["status"].is_string())
                                {
                                  status = e.value("status", "");
                                }
                              }

                              std::cout << "Parsed status: " << status << std::endl;

                              if (status != "started" && status != "created")
                              {
                                std::cout << "Game " << gameId << " ended with status " << status << std::endl;
                                return false; // Abort stream, game over
                              }

                              if (colorDetermined)
                              {
                                // Count spaces to determine move count
                                int moveCount = 0;
                                if (!moves.empty())
                                {
                                  moveCount = 1;
                                  for (char c : moves)
                                  {
                                    if (c == ' ')
                                      moveCount++;
                                  }
                                }

                                bool isWhiteTurn = (moveCount % 2 == 0);
                                if ((isWhite && isWhiteTurn) || (!isWhite && !isWhiteTurn))
                                {
                                  std::cout << "Bot's turn. Calculating move..." << std::endl;
                                  std::string positionCmd = moves.empty() ? "startpos" : "startpos moves " + moves;

                                  // You can adjust the think time (ms) and depth here.
                                  // Set either to 0 to ignore it (e.g., depth=0 means it only uses thinkTimeMs).
                                  //
                                  // Recommendations:
                                  // - Bullet/Blitz: thinkTimeMs = 100 to 500, depth = 10
                                  // - Rapid:        thinkTimeMs = 1000 to 2000, depth = 15
                                  // - Classical:    thinkTimeMs = 5000 to 10000, depth = 20
                                  int thinkTimeMs = 1000;
                                  int depth = 15;

                                  std::string bestMove = engine.getBestMove(positionCmd, thinkTimeMs, depth);
                                  if (!bestMove.empty())
                                  {
                                    std::cout << "Sending move " << bestMove << " for game " << gameId << std::endl;
                                    gameClient.makeMove(gameId, bestMove);
                                  }
                                  else
                                  {
                                    std::cerr << "Engine returned no bestmove!" << std::endl;
                                  }
                                }
                              }
                            }
                          }
                          catch (const json::exception &ex)
                          {
                            std::cerr << "JSON error in game stream: " << ex.what() << std::endl;
                          }
                          return true;
                        });

  engine.stop();
  std::cout << "Game " << gameId << " ended." << std::endl;
}
