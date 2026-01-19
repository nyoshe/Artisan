// Artisan.cpp : Defines the entry point for the application.
//

#include "Artisan.h"
#include "Tuner.h"
#include "UCI.h"
using namespace std;

int main(int argc, char *argv[]) {
  BB::init();
  // Tuner tuner("quiet-labeled.epd");
  if (argc > 1 && std::string(argv[1]) == "bench") {
    Engine engine = Engine(UciOptions());
    engine.bench();
    return 0;
  }

  while (UCI::getInstance()->loop()) {
  }
  return 0;
}
