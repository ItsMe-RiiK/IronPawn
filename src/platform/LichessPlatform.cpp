#include "LichessPlatform.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

LichessPlatform::LichessPlatform(const std::string &token)
{
  client = std::make_unique<LichessClient>(token);
}

LichessPlatform::~LichessPlatform() {}

void LichessPlatform::startListening(std::function<void(const ChessEvent &)> callback)
{
  eventCallback = callback;

  std::cout << "Starting Lichess Platform, fetching bot ID..." << std::endl;
  botId = client->getBotId();
  if (botId.empty())
  {
    std::cerr << "Failed to fetch bot ID. Token might be invalid." << std::endl;
    return;
  }
  std::cout << "Authenticated as bot: " << botId << std::endl;
  std::cout << "Listening for events..." << std::endl;

  client->streamEvents(
      [this](const std::string &eventJson) -> bool
      {
        try
        {
          json e = json::parse(eventJson);
          std::string type = e.value("type", "");

          if (type == "challenge")
          {
            ChessEvent evt;
            evt.type = ChessEventType::CHALLENGE_RECEIVED;
            evt.challengeId = e["challenge"]["id"];
            if (eventCallback)
              eventCallback(evt);
          }
          else if (type == "gameStart")
          {
            std::string gameId = e["game"]["id"];
            if (e["game"].contains("gameId"))
            {
              gameId = e["game"]["gameId"];
            }

            ChessEvent evt;
            evt.type = ChessEventType::GAME_STARTED;
            evt.gameId = gameId;
            if (eventCallback)
              eventCallback(evt);

            // Spawn a new thread to listen to this specific game's stream
            std::thread t(&LichessPlatform::streamGame, this, gameId);
            t.detach();
          }
        }
        catch (const json::exception &ex)
        {
          std::cerr << "JSON error: " << ex.what() << " in event: " << eventJson << std::endl;
        }
        return true;
      });
}

bool LichessPlatform::acceptChallenge(const std::string &challengeId)
{
  return client->acceptChallenge(challengeId);
}

void LichessPlatform::makeMove(const std::string &gameId, const std::string &move)
{
  client->makeMove(gameId, move);
}

void LichessPlatform::streamGame(const std::string &gameId)
{
  bool isWhite = false;
  bool colorDetermined = false;

  client->streamGame(gameId,
                     [this, gameId, &isWhite, &colorDetermined](const std::string &gameEventJson) -> bool
                     {
                       try
                       {
                         json e = json::parse(gameEventJson);
                         std::string type = e.value("type", "");

                         if (type == "gameFull")
                         {
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
                         }

                         if (type == "gameFull" || type == "gameState")
                         {
                           std::string moves = "";
                           std::string status = "";

                           if (type == "gameFull")
                           {
                             moves = e["state"].value("moves", "");
                             if (e["state"]["status"].is_object())
                               status = e["state"]["status"].value("name", "");
                             else if (e["state"]["status"].is_string())
                               status = e["state"].value("status", "");
                           }
                           else if (type == "gameState")
                           {
                             moves = e.value("moves", "");
                             if (e["status"].is_object())
                               status = e["status"].value("name", "");
                             else if (e["status"].is_string())
                               status = e.value("status", "");
                           }

                           if (status != "started" && status != "created")
                           {
                             ChessEvent evt;
                             evt.type = ChessEventType::GAME_ENDED;
                             evt.gameId = gameId;
                             evt.status = status;
                             if (eventCallback)
                               eventCallback(evt);
                             return false; // Abort stream
                           }

                           if (colorDetermined)
                           {
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
                             bool isBotTurn = ((isWhite && isWhiteTurn) || (!isWhite && !isWhiteTurn));

                             ChessEvent evt;
                             evt.type = ChessEventType::MOVE_RECEIVED;
                             evt.gameId = gameId;
                             evt.moves = moves;
                             evt.isBotTurn = isBotTurn;
                             if (eventCallback)
                               eventCallback(evt);
                           }
                         }
                       }
                       catch (const json::exception &ex)
                       {
                         std::cerr << "JSON error in game stream: " << ex.what() << std::endl;
                       }
                       return true;
                     });
}
