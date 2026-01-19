#pragma once

#include "Engine.h"
#include <algorithm>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <vector>

#include <mutex>

// #include "../nchess/imgui/imgui.h"

struct EngineData {
  std::string pv;
};
class UCI {
public:
  UCI() : engine_(Engine(UciOptions())) { instance = this; }

  void setupBoard(std::istringstream &iss);
  int loop();
  static UCI *getInstance();
  EngineData data;
  std::mutex data_out;

protected:
  static UCI *instance;

private:
  Engine engine_;
  UciOptions options;

  static void sendId() {
    std::cout << "id name Artisan" << std::endl;
    std::cout << "id author Nia W." << std::endl;
  }

  static void sendOptions() {
    std::cout << "option name Hash type spin default 16 min 1 max 65536"
              << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1"
              << std::endl;
  }

  void handleGo(std::istringstream &iss);
};
