#ifndef LICHESS_CLIENT_H
#define LICHESS_CLIENT_H

#include <curl/curl.h>
#include <functional>
#include <string>

class LichessClient
{
public:
  LichessClient(const std::string &token);
  ~LichessClient();

  // Callback type for NDJSON streams. Returns false to abort stream.
  using StreamCallback = std::function<bool(const std::string &)>;

  // Get the bot's username/ID
  std::string getBotId();

  // Start streaming events. Blocking call.
  void streamEvents(StreamCallback callback);

  // Stream a specific game's events. Blocking call.
  void streamGame(const std::string &gameId, StreamCallback callback);

  // Accept a challenge
  bool acceptChallenge(const std::string &challengeId);

  // Make a move
  bool makeMove(const std::string &gameId, const std::string &move);

private:
  std::string token;

  // Helper to perform a POST request
  bool postRequest(const std::string &url);

  // Helper for streaming
  void doStream(const std::string &url, StreamCallback callback);

  // libcurl write callback for streams
  static size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

  // libcurl write callback for standard strings
  static size_t stringWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata);
};

#endif // LICHESS_CLIENT_H
