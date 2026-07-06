#include "UciEngine.h"

#include <iostream>
#include <string.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

UciEngine::UciEngine(const std::string &path) : enginePath(path)
{
#ifdef _WIN32
  hChildStd_IN_Rd = NULL;
  hChildStd_IN_Wr = NULL;
  hChildStd_OUT_Rd = NULL;
  hChildStd_OUT_Wr = NULL;
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
#else
  inPipe[0] = -1;
  inPipe[1] = -1;
  outPipe[0] = -1;
  outPipe[1] = -1;
#endif
}

UciEngine::~UciEngine()
{
  stop();
}

bool UciEngine::start()
{
#ifdef _WIN32
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
  {
    std::cerr << "Failed to create out pipe" << std::endl;
    return false;
  }
  if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
  {
    return false;
  }

  if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0))
  {
    std::cerr << "Failed to create in pipe" << std::endl;
    return false;
  }
  if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
  {
    return false;
  }

  STARTUPINFO siStartInfo;
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = hChildStd_OUT_Wr;
  siStartInfo.hStdOutput = hChildStd_OUT_Wr;
  siStartInfo.hStdInput = hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  bool bSuccess = CreateProcess(NULL,
                                const_cast<char *>(enginePath.c_str()), // command line
                                NULL, // process security attributes
                                NULL, // primary thread security attributes
                                TRUE, // handles are inherited
                                0, // creation flags
                                NULL, // use parent's environment
                                NULL, // use parent's current directory
                                &siStartInfo, // STARTUPINFO pointer
                                &piProcInfo); // receives PROCESS_INFORMATION

  if (!bSuccess)
  {
    std::cerr << "Failed to create process." << std::endl;
    return false;
  }

  sendCommand("uci");
  while (true)
  {
    std::string line = readLine();
    if (line == "uciok")
      break;
  }

  sendCommand("isready");
  while (true)
  {
    std::string line = readLine();
    if (line == "readyok")
      break;
  }

  return true;

#else
  if (pipe(inPipe) == -1)
  {
    std::cerr << "Failed to create inPipe." << std::endl;
    return false;
  }
  if (pipe(outPipe) == -1)
  {
    std::cerr << "Failed to create outPipe." << std::endl;
    close(inPipe[0]);
    close(inPipe[1]);
    return false;
  }

  childPid = fork();
  if (childPid == -1)
  {
    std::cerr << "Failed to fork." << std::endl;
    close(inPipe[0]);
    close(inPipe[1]);
    close(outPipe[0]);
    close(outPipe[1]);
    return false;
  }

  if (childPid == 0)
  {
    dup2(inPipe[0], STDIN_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    close(inPipe[1]);
    close(outPipe[0]);

    char *args[] = { const_cast<char *>(enginePath.c_str()), nullptr };
    execvp(enginePath.c_str(), args);
    std::cerr << "Failed to execute engine: " << enginePath << std::endl;
    exit(1);
  }
  else
  {
    close(inPipe[0]);
    close(outPipe[1]);

    sendCommand("uci");
    while (true)
    {
      std::string line = readLine();
      if (line == "uciok")
        break;
    }

    sendCommand("isready");
    while (true)
    {
      std::string line = readLine();
      if (line == "readyok")
        break;
    }
  }
  return true;
#endif
}

void UciEngine::stop()
{
#ifdef _WIN32
  if (piProcInfo.hProcess != NULL)
  {
    sendCommand("quit");
    WaitForSingleObject(piProcInfo.hProcess, 3000); // Wait 3s
    TerminateProcess(piProcInfo.hProcess, 0); // Force kill if still alive

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStd_IN_Wr);
    CloseHandle(hChildStd_OUT_Rd);
    CloseHandle(hChildStd_IN_Rd);
    CloseHandle(hChildStd_OUT_Wr);

    piProcInfo.hProcess = NULL;
    piProcInfo.hThread = NULL;
  }
#else
  if (childPid > 0)
  {
    sendCommand("quit");
    if (inPipe[1] != -1)
      close(inPipe[1]);
    if (outPipe[0] != -1)
      close(outPipe[0]);

    waitpid(childPid, nullptr, 0);
    childPid = -1;
    inPipe[0] = inPipe[1] = outPipe[0] = outPipe[1] = -1;
  }
#endif
}

void UciEngine::sendCommand(const std::string &cmd)
{
  std::string commandWithNewline = cmd + "\n";
#ifdef _WIN32
  if (hChildStd_IN_Wr != NULL)
  {
    DWORD dwWritten;
    WriteFile(hChildStd_IN_Wr, commandWithNewline.c_str(), commandWithNewline.length(), &dwWritten, NULL);
  }
#else
  if (inPipe[1] != -1)
  {
    write(inPipe[1], commandWithNewline.c_str(), commandWithNewline.length());
  }
#endif
}

std::string UciEngine::readLine()
{
  std::string result = "";
  char c;
#ifdef _WIN32
  if (hChildStd_OUT_Rd == NULL)
    return result;
  DWORD dwRead;
  while (ReadFile(hChildStd_OUT_Rd, &c, 1, &dwRead, NULL) && dwRead > 0)
  {
    if (c == '\n')
      break;
    if (c != '\r')
      result += c;
  }
#else
  if (outPipe[0] == -1)
    return result;
  while (read(outPipe[0], &c, 1) > 0)
  {
    if (c == '\n')
      break;
    if (c != '\r')
      result += c;
  }
#endif
  return result;
}

void UciEngine::setElo(int elo)
{
  sendCommand("setoption name UCI_LimitStrength value true");
  sendCommand("setoption name UCI_Elo value " + std::to_string(elo));
}

void UciEngine::setSkillLevel(int level)
{
  sendCommand("setoption name Skill Level value " + std::to_string(level));
}

std::string UciEngine::getBestMove(const std::string &position, int movetimeMs, int depth)
{
  sendCommand("position " + position);

  std::string goCommand = "go";
  if (depth > 0)
  {
    goCommand += " depth " + std::to_string(depth);
  }
  if (movetimeMs > 0)
  {
    goCommand += " movetime " + std::to_string(movetimeMs);
  }
  sendCommand(goCommand);

  int emptyCount = 0;

  while (true)
  {
    std::string line = readLine();
    if (line.empty())
    {
      emptyCount++;
      if (emptyCount > 1000)
      {
        std::cerr << "Engine Stopped responding" << std::endl;
        break;
      }
      continue;
    }

    emptyCount = 0;

    if (line.find("bestmove") == 0)
    {
      size_t firstSpace = line.find(' ');
      size_t secondSpace = line.find(' ', firstSpace + 1);
      if (firstSpace != std::string::npos)
      {
        if (secondSpace != std::string::npos)
        {
          return line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        }
        else
        {
          return line.substr(firstSpace + 1);
        }
      }
      break;
    }
  }
  return "";
}
