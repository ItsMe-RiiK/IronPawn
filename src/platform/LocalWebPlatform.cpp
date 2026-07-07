#include "LocalWebPlatform.h"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LocalWebPlatform::LocalWebPlatform() {}

LocalWebPlatform::~LocalWebPlatform()
{
  svr.stop();
}

void LocalWebPlatform::startListening(std::function<void(const ChessEvent &)> callback)
{
  eventCallback = callback;

  // Serve static files from the 'public' directory
  svr.set_mount_point("/", "./public");

  svr.Post("/api/start",
           [this](const httplib::Request &, httplib::Response &res)
           {
             {
               std::lock_guard<std::mutex> lock(gameMutex);
               currentMoves = "";
               latestBotMove = "";
             }

             ChessEvent evt;
             evt.type = ChessEventType::GAME_STARTED;
             evt.gameId = "local_web";
             if (eventCallback)
               eventCallback(evt);

             res.set_content(R"({"status":"ok"})", "application/json");
           });

  // Endpoint to receive human moves
  svr.Post("/api/move",
           [this](const httplib::Request &req, httplib::Response &res)
           {
             (void)req;
             json body;
             try
             {
               body = json::parse(req.body);
               std::string move = body.value("move", "");

               if (!move.empty())
               {
                 std::lock_guard<std::mutex> lock(gameMutex);
                 if (!currentMoves.empty())
                   currentMoves += " ";
                 currentMoves += move;
                 latestBotMove = ""; // Clear bot move until it replies

                 ChessEvent evt;
                 evt.type = ChessEventType::MOVE_RECEIVED;
                 evt.gameId = "local_web";
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

  // Endpoint for frontend to poll bot's move
  svr.Get("/api/status",
          [this](const httplib::Request &, httplib::Response &res)
          {
            std::lock_guard<std::mutex> lock(gameMutex);
            json response;
            response["latestBotMove"] = latestBotMove;
            response["currentMoves"] = currentMoves;
            res.set_content(response.dump(), "application/json");
          });

  std::cout << "Starting Local Web GUI Server on http://localhost:8080" << std::endl;
  std::cout << "Open your browser to http://localhost:8080 to play!" << std::endl;
  svr.listen("0.0.0.0", 8080);
}

bool LocalWebPlatform::acceptChallenge(const std::string &)
{
  return true; // Auto-accept web games
}

void LocalWebPlatform::makeMove(const std::string &, const std::string &move)
{
  std::lock_guard<std::mutex> lock(gameMutex);
  if (!currentMoves.empty())
    currentMoves += " ";
  currentMoves += move;
  latestBotMove = move; // Store the bot move so the frontend can poll it
}
