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
    "UCI.h" "UCI.cpp"
    "Misc.h"
    "Tuner.h"
    "Tuner.cpp")

set_target_properties(Artisan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(CMAKE_SYSTEM MATCHES Linux)
    target_link_libraries(Artisan PRIVATE Threads::Threads)

endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Artisan PROPERTY CXX_STANDARD 20)
endif()

if(CMAKE_SYSTEM MATCHES Linux)
    # Apply Linux-specific settings to the Artisan target
    target_compile_options(Artisan PRIVATE -stdlib=libstdc++)
    # The key fix: explicitly link against libstdc++ and other required libraries
    target_link_libraries(Artisan PRIVATE 
        -lstdc++ 
        -lm 
        -ldl 
        -lpthread
        Threads::Threads)
endif()

# TODO: Add tests and install targets if needed.
