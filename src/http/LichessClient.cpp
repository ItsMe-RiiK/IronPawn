#include "LichessClient.h"

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct StreamData
{
  LichessClient::StreamCallback callback;
  std::string buffer;
};

LichessClient::LichessClient(const std::string &token) : token(token)
{
  curl_global_init(CURL_GLOBAL_ALL);
}

LichessClient::~LichessClient()
{
  curl_global_cleanup();
}

size_t LichessClient::writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  size_t realsize = size * nmemb;
  StreamData *data = static_cast<StreamData *>(userdata);

  // Append new data to the buffer
  data->buffer.append(ptr, realsize);

  // Process line by line (NDJSON)
  size_t pos = 0;
  while ((pos = data->buffer.find('\n')) != std::string::npos)
  {
    std::string line = data->buffer.substr(0, pos);
    data->buffer.erase(0, pos + 1);

    if (!line.empty())
    {
      // Call the callback. If it returns false, we abort the stream.
      if (!data->callback(line))
      {
        return 0; // Abort curl transfer
      }
    }
  }

  return realsize;
}

size_t LichessClient::stringWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  size_t realsize = size * nmemb;
  std::string *str = static_cast<std::string *>(userdata);
  str->append(ptr, realsize);
  return realsize;
}

std::string LichessClient::getBotId()
{
  CURL *curl = curl_easy_init();
  std::string response = "";
  if (curl)
  {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://lichess.org/api/account");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
      try
      {
        json e = json::parse(response);
        if (e.contains("id"))
        {
          std::string id = e["id"];
          curl_slist_free_all(headers);
          curl_easy_cleanup(curl);
          return id;
        }
      }
      catch (...)
      {
      }
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  return "";
}

void LichessClient::doStream(const std::string &url, StreamCallback callback)
{
  CURL *curl = curl_easy_init();
  if (curl)
  {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());

    StreamData data;
    data.callback = callback;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

    // Required to keep the stream alive
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }
    else
    {
      long response_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      if (response_code >= 400)
      {
        std::cerr << "Stream to " << url << " failed with HTTP " << response_code << ". Buffer left: " << data.buffer << std::endl;
      }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
}

void LichessClient::streamEvents(StreamCallback callback)
{
  doStream("https://lichess.org/api/stream/event", callback);
}

void LichessClient::streamGame(const std::string &gameId, StreamCallback callback)
{
  doStream("https://lichess.org/api/bot/game/stream/" + gameId, callback);
}

bool LichessClient::postRequest(const std::string &url)
{
  CURL *curl = curl_easy_init();
  bool success = false;
  if (curl)
  {
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());

    // POST without body requires Content-Length: 0 usually, or just empty POST
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
      long response_code;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      if (response_code >= 200 && response_code < 300)
      {
        success = true;
      }
      else
      {
        std::cerr << "POST Request to " << url << " failed with code " << response_code << std::endl;
      }
    }
    else
    {
      std::cerr << "POST Request failed: " << curl_easy_strerror(res) << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  return success;
}

bool LichessClient::acceptChallenge(const std::string &challengeId)
{
  return postRequest("https://lichess.org/api/challenge/" + challengeId + "/accept");
}

bool LichessClient::makeMove(const std::string &gameId, const std::string &move)
{
  return postRequest("https://lichess.org/api/bot/game/" + gameId + "/move/" + move);
}
