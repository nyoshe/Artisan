# CMakeList.txt : CMake project for Artisan, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Artisan "Artisan.cpp" "Artisan.h"
    "Board.h" "Board.cpp"
    "BitBoard.h"
    "Memory.h"
    "Engine.h" "Engine.cpp"
    
    "Move.h" "Move.cpp"
    "MoveGen.h" "MoveGen.cpp"
    "include/chess.hpp"
    "Parser.h"
    "UCI.h"
 "Misc.h"   "Tuner.h" "Tuner.cpp")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Artisan PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
