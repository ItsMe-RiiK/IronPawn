#include "Bot.h"

#include <iostream>
#include <thread>

Bot::Bot(std::unique_ptr<IPlatform> platform, const std::string &enginePath, int elo, int thinkTimeMs, int depth) :
    platform(std::move(platform)), enginePath(enginePath), elo(elo), thinkTimeMs(thinkTimeMs), depth(depth)
{
}

Bot::~Bot()
{
  std::lock_guard<std::mutex> lock(gamesMutex);
  for (auto &pair : activeGames)
  {
    pair.second->stop();
  }
}

void Bot::run()
{
  platform->startListening([this](const ChessEvent &evt) { handleEvent(evt); });
}

void Bot::handleEvent(const ChessEvent &evt)
{
  switch (evt.type)
  {
  case ChessEventType::CHALLENGE_RECEIVED:
  {
    std::cout << "Received challenge: " << evt.challengeId << std::endl;
    if (platform->acceptChallenge(evt.challengeId))
    {
      std::cout << "Accepted challenge: " << evt.challengeId << std::endl;
    }
    else
    {
      std::cerr << "Failed to accept challenge: " << evt.challengeId << std::endl;
    }
    break;
  }
  case ChessEventType::GAME_STARTED:
  {
    std::cout << "Game started: " << evt.gameId << std::endl;
    auto engine = std::make_shared<UciEngine>(enginePath);
    if (engine->start())
    {
      int gameElo = (evt.elo >= 0) ? evt.elo : this->elo;
      engine->setElo(gameElo); // Use dynamic ELO if provided, else use base config ELO
      engine->sendCommand("ucinewgame");

      std::shared_ptr<UciEngine> oldEngine;
      {
        std::lock_guard<std::mutex> lock(gamesMutex);
        if (activeGames.find(evt.gameId) != activeGames.end())
        {
          oldEngine = activeGames[evt.gameId];
        }
        activeGames[evt.gameId] = engine;
      }

      if (oldEngine)
      {
        oldEngine->stop(); // Force abort the old engine's thinking so it doesn't pollute the new game
      }
      std::cout << "Engine successfully assigned to game " << evt.gameId << " with ELO " << gameElo << std::endl;

      // If human chose Black, the bot must play White and make the first move immediately!
      if (!evt.isWhite)
      {
        std::thread t(&Bot::processMoveAsync, this, evt.gameId, "");
        t.detach();
      }
    }
    else
    {
      std::cerr << "Failed to start engine for game " << evt.gameId << std::endl;
    }
    break;
  }
  case ChessEventType::MOVE_RECEIVED:
  {
    if (evt.isBotTurn)
    {
      // Offload heavy thinking to a detached thread to prevent blocking event loop!
      // This fulfills the "no spike / crash mitigation" requirement.
      std::thread t(&Bot::processMoveAsync, this, evt.gameId, evt.moves);
      t.detach();
    }
    break;
  }
  case ChessEventType::GAME_ENDED:
  {
    std::cout << "Game " << evt.gameId << " ended. Status: " << evt.status << std::endl;
    std::lock_guard<std::mutex> lock(gamesMutex);
    auto it = activeGames.find(evt.gameId);
    if (it != activeGames.end())
    {
      it->second->stop();
      activeGames.erase(it);
    }
    break;
  }
  }
}

void Bot::processMoveAsync(std::string gameId, std::string moves)
{
  std::shared_ptr<UciEngine> engine;
  {
    std::lock_guard<std::mutex> lock(gamesMutex);
    auto it = activeGames.find(gameId);
    if (it != activeGames.end())
    {
      engine = it->second;
    }
  }

  if (!engine)
  {
    std::cerr << "Error: Engine not found for game " << gameId << std::endl;
    return;
  }

  std::cout << "Bot's turn. Calculating move..." << std::endl;
  std::string positionCmd = moves.empty() ? "startpos" : "startpos moves " + moves;

  std::string bestMove = engine->getBestMove(positionCmd, thinkTimeMs, depth);
  if (!bestMove.empty())
  {
    std::cout << "Sending move " << bestMove << " for game " << gameId << std::endl;
    platform->makeMove(gameId, bestMove);
  }
  else
  {
    std::cerr << "Engine returned no bestmove!" << std::endl;
  }
}
