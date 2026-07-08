#include "DiscordPlatform.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <algorithm>

static bool isValidUciMove(const std::string& move) {
    if (move.empty() || move.length() < 4 || move.length() > 5) return false;
    for (char c : move) {
        if (!std::isalnum(c)) return false;
    }
    return true;
}

DiscordPlatform::DiscordPlatform(const std::string &token)
{
  // The dpp::i_default_intents gives us access to message content
  bot = std::make_unique<dpp::cluster>(token, dpp::i_default_intents | dpp::i_message_content);
}

DiscordPlatform::~DiscordPlatform()
{
  svr.stop();
  if (webThread.joinable())
    webThread.join();
}

void DiscordPlatform::startListening(std::function<void(const ChessEvent &)> callback)
{
  eventCallback = callback;

  svr.set_mount_point("/", "./public");

  svr.Post("/api/start",
           [this](const httplib::Request &req, httplib::Response &res)
           {
             auto jsonReq = nlohmann::json::parse(req.body);
             std::string gameId = jsonReq.value("gameId", "local_web");
             int elo = jsonReq.value("elo", 1500);
             if (elo < 0) elo = 0;
             if (elo > 4000) elo = 4000;
             bool isWhite = jsonReq.value("isWhite", true);

             {
               std::lock_guard<std::mutex> lock(gamesMutex);
               if (activeGames.find(gameId) == activeGames.end() && activeGames.size() >= 100) {
                 res.status = 429;
                 res.set_content(R"({"status":"error", "message":"Server capacity reached. Try again later."})", "application/json");
                 return;
               }
               activeGames[gameId] = "";
               latestBotMoves[gameId] = "";
             }

             ChessEvent evt;
             evt.type = ChessEventType::GAME_STARTED;
             evt.gameId = gameId;
             evt.elo = elo;
             evt.isWhite = isWhite;
             if (eventCallback)
               eventCallback(evt);

             res.set_content(R"({"status":"ok"})", "application/json");
           });

  svr.Post("/api/move",
           [this](const httplib::Request &req, httplib::Response &res)
           {
             nlohmann::json body;
             try
             {
               body = nlohmann::json::parse(req.body);
               std::string move = body.value("move", "");
               std::string gameId = body.value("gameId", "local_web");

               if (!isValidUciMove(move)) {
                 res.status = 400;
                 res.set_content(R"({"status":"error", "message":"Invalid move format."})", "application/json");
                 return;
               }

               if (!move.empty())
               {
                 std::string currentMoves;
                 {
                   std::lock_guard<std::mutex> lock(gamesMutex);
                   if (!activeGames[gameId].empty())
                     activeGames[gameId] += " ";
                   activeGames[gameId] += move;
                   currentMoves = activeGames[gameId];
                   latestBotMoves[gameId] = "";
                 }

                 ChessEvent evt;
                 evt.type = ChessEventType::MOVE_RECEIVED;
                 evt.gameId = gameId;
                 evt.moves = currentMoves;
                 evt.isBotTurn = true;
                 if (eventCallback)
                   eventCallback(evt);
               }
             }
             catch (...)
             {
             }

             res.set_content(R"({"status":"ok"})", "application/json");
           });

  svr.Post("/api/end",
           [this](const httplib::Request &req, httplib::Response &res)
           {
             try
             {
               auto body = nlohmann::json::parse(req.body);
               std::string gameId = body.value("gameId", "local_web");
               {
                 std::lock_guard<std::mutex> lock(gamesMutex);
                 activeGames.erase(gameId);
                 latestBotMoves.erase(gameId);
               }
               ChessEvent evt;
               evt.type = ChessEventType::GAME_ENDED;
               evt.gameId = gameId;
               evt.status = "abandoned";
               if (eventCallback)
                 eventCallback(evt);
             }
             catch (...)
             {
             }
             res.set_content(R"({"status":"ok"})", "application/json");
           });

  svr.Get("/api/status",
          [this](const httplib::Request &req, httplib::Response &res)
          {
            std::string gameId = req.has_param("gameId") ? req.get_param_value("gameId") : "local_web";
            std::lock_guard<std::mutex> lock(gamesMutex);
            nlohmann::json response;
            response["latestBotMove"] = latestBotMoves[gameId];
            response["currentMoves"] = activeGames[gameId];
            res.set_content(response.dump(), "application/json");
          });

  svr.Get("/api/config",
          [this](const httplib::Request &, httplib::Response &res)
          {
            nlohmann::json response;
            response["clientId"] = std::to_string(bot->me.id);
            res.set_content(response.dump(), "application/json");
          });

  webThread = std::thread([this]() { svr.listen("0.0.0.0", 8080); });

  bot->on_log(dpp::utility::cout_logger());

  bot->on_message_create(
      [this](const dpp::message_create_t &event)
      {
        if (event.msg.author.is_bot())
          return;

        std::string content = event.msg.content;
        std::string channelId = std::to_string(event.msg.channel_id);

        if (content == "!play")
        {
          {
            std::lock_guard<std::mutex> lock(gamesMutex);
            if (activeGames.find(channelId) == activeGames.end() && activeGames.size() >= 100) {
              dpp::message msg(event.msg.channel_id, "Server is currently at maximum capacity. Please try again later.");
              event.reply(msg);
              return;
            }
            activeGames[channelId] = ""; // empty moves
          }
          std::string activityLink = "https://discord.com/activities/" + std::to_string(bot->me.id);
          dpp::message msg(
              event.msg.channel_id,
              "Game started! Click the link below to launch the Interactive GUI (This message will self-destruct in 15 seconds):\n" +
                  activityLink);
          event.reply(msg, false,
                      [this, event](const dpp::confirmation_callback_t &callback)
                      {
                        if (!callback.is_error())
                        {
                          dpp::message m = std::get<dpp::message>(callback.value);
                          std::thread(
                              [this, m, event]()
                              {
                                std::this_thread::sleep_for(std::chrono::seconds(15));
                                bot->message_delete(m.id, m.channel_id);
                                bot->message_delete(event.msg.id, event.msg.channel_id);
                              })
                              .detach();
                        }
                      });

          ChessEvent evt;
          evt.type = ChessEventType::GAME_STARTED;
          evt.gameId = channelId;
          if (eventCallback)
            eventCallback(evt);
        }
        else if (content.find("!move ") == 0)
        {
          std::string move = content.substr(6); // Extract the move
          if (!isValidUciMove(move)) {
            dpp::message msg(event.msg.channel_id, "Invalid move format. Please use UCI format (e.g., e2e4).");
            event.reply(msg);
            return;
          }

          std::string currentMoves;
          {
            std::lock_guard<std::mutex> lock(gamesMutex);
            if (activeGames.find(channelId) == activeGames.end())
            {
              dpp::message msg(event.msg.channel_id, "No active game in this channel. Type `!play` to start.");
              event.reply(msg);
              return;
            }
            currentMoves = activeGames[channelId];
            if (!currentMoves.empty())
              currentMoves += " ";
            currentMoves += move;
            activeGames[channelId] = currentMoves;
          }
          dpp::message msg(event.msg.channel_id, "You played **" + move + "**. Stockfish is thinking...");
          event.reply(msg);

          // User just played, so now it is the bot's turn.
          ChessEvent evt;
          evt.type = ChessEventType::MOVE_RECEIVED;
          evt.gameId = channelId;
          evt.moves = currentMoves;
          evt.isBotTurn = true; // Bot needs to reply
          if (eventCallback)
            eventCallback(evt);
        }
        else if (content == "!resign")
        {
          std::lock_guard<std::mutex> lock(gamesMutex);
          if (activeGames.erase(channelId))
          {
            dpp::message msg(event.msg.channel_id, "You resigned! Game over.");
            event.reply(msg);
            ChessEvent evt;
            evt.type = ChessEventType::GAME_ENDED;
            evt.gameId = channelId;
            evt.status = "resign";
            if (eventCallback)
              eventCallback(evt);
          }
        }
      });

  bot->on_ready([this](const dpp::ready_t &) { std::cout << "Discord Bot connected as " << bot->me.username << std::endl; });

  std::cout << "Starting Discord connection..." << std::endl;
  bot->start(dpp::st_wait); // blocks forever
}

bool DiscordPlatform::acceptChallenge(const std::string &)
{
  return true; // We auto-accept by starting when user types !play
}

void DiscordPlatform::makeMove(const std::string &gameId, const std::string &move)
{
  std::string currentMoves;
  {
    std::lock_guard<std::mutex> lock(gamesMutex);
    if (activeGames.find(gameId) != activeGames.end())
    {
      if (!activeGames[gameId].empty())
        activeGames[gameId] += " ";
      activeGames[gameId] += move;
      currentMoves = activeGames[gameId];
      latestBotMoves[gameId] = move;
    }
  }

  uint64_t channel_id = 0;
  try
  {
    channel_id = std::stoull(gameId);
  }
  catch (...)
  {
  }
  if (channel_id > 0)
  {
    // Muted: Do not spam Discord chat with move notations
    // dpp::message msg(channel_id, "Stockfish played: **" + move + "**\nYour turn!");
    // bot->message_create(msg);
  }
}
