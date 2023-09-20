#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)
#include "gamelog.h"
gamelog &gamelog::get() {
  static gamelog glog{};
  return glog;
}

gamelog::gamelog() {
  AllocConsole();
  FILE *fdummy;
  auto err = freopen_s(&fdummy, "CONOUT$", "w", stdout);
  err = freopen_s(&fdummy, "CONOUT$", "w", stderr);
  std::cout.clear();
  std::clog.clear();
  const HANDLE hConOut =
      CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, NULL);
  SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
  SetStdHandle(STD_ERROR_HANDLE, hConOut);
  std::wcout.clear();
  std::wclog.clear();
}

gamelog::~gamelog() { FreeConsole(); }
