#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Engine.h"
#include <signal.h>
#include <conio.h>
#include <mutex>

//#include "../nchess/imgui/imgui.h"

struct EngineData {
    std::string pv;
};
class UCI
{
public:
    UCI()
        : debug_mode_(false)
    {
        instance = this;
        engine_ = Engine();
    }

    void setupBoard(std::istringstream& iss) {
        std::string token;
        engine_.b.reset();
        iss >> token;
        if (token == "fen") {
            engine_.setBoardFEN(iss);
        }
        engine_.setBoardUCI(iss);
    }
    void loop()
    {
        std::string line;

        while (std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "uci")
            {
                sendId();
                sendOptions();
                std::cout << "uciok" << std::endl;
            }
            else if (token == "isready")
            {
                std::cout << "readyok" << std::endl;
            }
            else if (token == "setoption")
            {
                // Option parsing can be implemented here
            }
            else if (token == "ucinewgame")
            {
                engine_ = Engine();
            }
            else if (token == "position")
            {
                setupBoard(iss);
            }
            else if (token == "go")
            {
                handleGo(iss);
            }
            else if (token == "stop")
            {
                // Not implemented: should stop search
            }
            else if (token == "quit")
            {
                //nothing
            }
            else if (token == "debug")
            {
                std::string mode;
                iss >> mode;
                debug_mode_ = (mode == "on");
            }
            else if (token == "test")
            {

                /*Warning; Illegal pv move c4b3 from new
Info; info score cp 716 depth 8 nodes 594293 nps 3301627 pv e6c7 e8d8 c4b3 h8g8 c7a6 c8a6 f7a7 g8e8 a7a6
Position; fen r1bqkb1r/pppp1ppp/2n2n2/4P3/4P3/5N2/PPP2PPP/RNBQKB1R b KQkq - 0 4
Moves; f6e4 d1d5 f7f5 f1c4 d8e7 e1g1 c6b4 d5d1 b4c6 b1c3 c6e5 c3d5 e5f3 d1f3 e7c5 f3f5 e4f6 d5f6 e8d8 f1e1 f8e7 f5c5 e7c5 f6d5 h7h6 c1f4 d7d6 b2b4 c5b6 e1e7 g7g5 f4d2 b6d4 a1e1 c7c6 d5c7 a8b8 c2c3 d4e5 e7f7 b7b5 c7e6 d8e8 e6c7 e8d8 c7e6 d8e8
                */
                static std::vector<std::string> pos_list = {
                    "fen r1bqk1r1/1p1p1n2/p1n2pN1/2p1b2Q/2P1Pp2/1PN5/PB4PP/R4RK1 w q -",
					"fen r1n2N1k/2n2K1p/3pp3/5Pp1/b5R1/8/1PPP4/8 w -",
					"fen r1b1r1k1/1pqn1pbp/p2pp1p1/P7/1n1NPP1Q/2NBBR2/1PP3PP/R6K w -",
					"fen 5b2/p2k1p2/P3pP1p/n2pP1p1/1p1P2P1/1P1KBN2/7P/8 w -",
					"fen r3kbnr/1b3ppp/pqn5/1pp1P3/3p4/1BN2N2/PP2QPPP/R1BR2K1 w kq -"

                };

                std::istringstream test("fen r1bqkb1r/pppp1ppp/2n2n2/4P3/4P3/5N2/PPP2PPP/RNBQKB1R b KQkq - 0 4 moves f6e4 d1d5 f7f5 f1c4 d8e7 e1g1 c6b4 d5d1 b4c6 b1c3 c6e5 c3d5 e5f3");


                //std::istringstream test("fen rn1qkb1r/ppp1ppp1/7p/3p4/6bB/2P2N2/PPP1QPPP/R3KB1R b KQkq - 1 7 moves b8c6 e1c1 a8c8 c1b1 g7g5 h4g3 f8g7 h2h3 g4h5 h3h4 e7e5 h4g5 h6g5 h1h5 h8h5 f3e5 g7e5 e2h5 e5g3 f2g3 d8f6 f1b5 f6e5 d1f1 e5e7 h5h1 e8d7 h1h3 d7d8 h3f5 e7e6 f5f7 e6f7 f1f7 a7a6 b5c6 b7c6 f7g7 d8e8 a2a3 e8f8 g7g5 c8e8 g5f5 f8g7 f5g5 g7f6 g5g4 e8e1 b1a2 a6a5 g4f4 f6g7 f4g4 g7h7 g4f4 h7g7 c3c4 e1e2 c4d5 c6d5 c2c4 c7c6 c4d5 c6d5 f4f5 e2d2 a3a4 d2d3 g3g4 d3d4 a2b1 d4a4 f5d5 a4g4 d5a5 g4g2 a5a7 g7g6 a7d7 g2e2 b1c1 e2e4 c1b1 g6f5 d7f7 f5g4 f7g7 g4f3 g7f7 f3g4 b1c2 e4e2 c2c3 g4g3 b2b4 e2e1 b4b5 e1b1 c3c4 b1b2 c4c5 b2c2 c5b6 c2b2 f7d7 b2b1 d7g7 g3f3 g7f7 f3g3 f7g7 g3f3 b6c6 b1b2 b5b6 b2c2 c6d5 c2d2 d5c4 d2c2 c4d3 c2a2 b6b7 a2b2 g7f7 f3g3 d3c3 b2b5 f7g7 g3f4 g7c7 f4f3 ");
                engine_.tc.winc = 100000000;
                engine_.tc.binc = 100000000;
                setupBoard(test);
                engine_.b.printBoard();
                std::cout << engine_.b.boardString();
                std::istringstream go_stream("go movetime 500000");
                handleGo(go_stream);

                while (true) {
                    for (auto& position : pos_list) {
                        engine_ = Engine();
                        std::istringstream ss(position);
                        setupBoard(ss);
                        std::istringstream go_ss("go movetime 1000");
                        handleGo(go_ss);
                    }
                }
                
                

                
            }
            // Add more UCI commands as needed
        }
    }

    void getEngineUpdate() {
        
        while (true) {
            data_out.lock();
            std::string pv = engine_.getPV();
            if (!pv.empty()) {
                data.pv = pv;
            }
            data_out.unlock();
        }
        
    }

    static void update(int signal) {
        instance->getEngineUpdate();
    }

    static UCI* getInstance() {
        if (instance == nullptr) {
            instance = new UCI();
        }
        return instance;
    }
    EngineData data;
    std::mutex data_out;
protected:
    static UCI* instance;
private:
    
    Engine engine_;
    bool debug_mode_;

    void sendId()
    {
        std::cout << "id name nchess" << std::endl;
        std::cout << "id author nia" << std::endl;
    }

    void sendOptions()
    {
        // Example: std::cout << "option name Hash type spin default 16 min 1 max 128" << std::endl;
    }



    void handleGo(std::istringstream& iss)
    {
        int depth = 0;
        std::string token;
        TimeControl tc;
        while (iss >> token) {
            if (token == "wtime") iss >> tc.wtime;
            if (token == "winc") iss >> tc.winc;
            if (token == "btime") iss >> tc.btime;
            if (token == "binc") iss >> tc.binc;
            if (token == "depth") iss >> depth;
            if (token == "movetime") iss >> tc.movetime;
        }
        engine_.tc = tc;
        Move best_move = engine_.search(depth > 0 ? depth : 7);
        

    	std::cout << "bestmove " << best_move.toUci() << std::endl;

    }
};

UCI* UCI::instance = nullptr;