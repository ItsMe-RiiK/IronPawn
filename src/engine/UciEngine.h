#ifndef UCI_ENGINE_H
#define UCI_ENGINE_H

#include <mutex>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/types.h>
#endif

class UciEngine
{
public:
  UciEngine(const std::string &path);
  ~UciEngine();

  bool start();
  void stop();

  void sendCommand(const std::string &cmd);
  std::string readLine();

  // Configure difficulty
  void setElo(int elo);
  void setSkillLevel(int level);

  // Blocking call that waits for "bestmove"
  std::string getBestMove(const std::string &position, int movetimeMs = 1000, int depth = 0);

private:
  std::string enginePath;
  std::mutex engineMutex;

#ifdef _WIN32
  HANDLE hChildStd_IN_Rd = NULL;
  HANDLE hChildStd_IN_Wr = NULL;
  HANDLE hChildStd_OUT_Rd = NULL;
  HANDLE hChildStd_OUT_Wr = NULL;
  PROCESS_INFORMATION piProcInfo;
#else
  pid_t childPid = -1;
  int inPipe[2]; // Parent writes to inPipe[1], Child reads from inPipe[0]
  int outPipe[2]; // Child writes to outPipe[1], Parent reads from outPipe[0]
#endif
};

#endif // UCI_ENGINE_H
