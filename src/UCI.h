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
    UCI(): engine_(Engine(UciOptions())) {
	    instance = this;
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
    int loop()
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
                options.uci = true;
                std::cout << "uciok" << std::endl;
            }
            else if (token == "isready")
            {
                std::cout << "readyok" << std::endl;
            }
            else if (token == "setoption")
            {
                //eat "name" token
                iss >> token;
                iss >> token;
                std::ranges::transform(token.begin(), token.end(), token.begin(), ::tolower);
                if (token == "hash") {
                    //eat "value"
                    iss >> token;
                    iss >> token;
                    options.hash_size = std::stoi(token);
                }
            }
            else if (token == "ucinewgame")
            {
                engine_ = Engine(options);
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
                return 0;
            }
            else if (token == "debug")
            {
                std::string mode;
                iss >> mode;
                options.debug = (mode == "on");
            }
            else if (token == "test")
            {
                /*
                 *
                 * Warning; Illegal pv move c6b4 from fixed_king_safety

Info; info score cp 70 depth 10 time 60 nodes 222058 nps 3700966 pv d3b3 b4c6 b3d3 c6b4 d3b3 b4c6 b3d3 c6b4 d3b3 b4c6
Position; fen r2qk1nr/pp1bbppp/2np4/1B1Q4/4PB2/5N2/PPP2PPP/RN2K2R b KQkq - 5 8
Moves; g8f6 d5d3 e8g8 b5c4 c6b4
            	*/
                static std::vector<std::string> pos_list = {
                    "fen r1bqk1r1/1p1p1n2/p1n2pN1/2p1b2Q/2P1Pp2/1PN5/PB4PP/R4RK1 w q - 0 0",
					"fen r1n2N1k/2n2K1p/3pp3/5Pp1/b5R1/8/1PPP4/8 w - - 0 0",
					"fen r1b1r1k1/1pqn1pbp/p2pp1p1/P7/1n1NPP1Q/2NBBR2/1PP3PP/R6K w - - 0 0",
					"fen 5b2/p2k1p2/P3pP1p/n2pP1p1/1p1P2P1/1P1KBN2/7P/8 w - - 0 0",
					"fen r3kbnr/1b3ppp/pqn5/1pp1P3/3p4/1BN2N2/PP2QPPP/R1BR2K1 w kq - 0 0"

                };

                //std::istringstream test("fen r2qkb1r/p1pp1ppp/1p2pn2/8/1nPP4/2N1PP2/PPQ2P1P/R1B1KB1R w KQkq - 1 8 moves c2d2 c7c5 d4d5 f8d6 f1g2 e6d5 a2a3 b4a6 c4d5 e8g8 e1g1 f8e8 f3f4 a6c7 e3e4 f6h5 e4e5 d6f8 d5d6 c7e6 g2a8 d8a8 d2d5 h5f4 d5a8 e8a8 f1e1 f4d3 e1e3 c5c4 c3b5 e6c5 c1d2 d3b2 d2b4 c5d3 b5c7 a8c8 b4c3 b2a4 c3d4 h7h5 e3e4 c8d8 a1a2 f7f5 e5f6 f8d6 e4e8 d8e8 c7e8 d6c5 d4c5 a4c5 f6g7 d3f4 f2f3 c4c3 a2c2 h5h4 g1h1 f4d5 e8d6 h4h3 d6f5 g8h7 c2c1 c5d3 c1c2 b6b5 c2e2 d5c7 e2c2 c7d5 c2e2");
                chess::Board testb("r1bqk1nr/ppp2ppp/2nb4/3pp3/8/5P1P/PPPPP1PK/R1BQ1BNR b kq - 2 5 ");
                std::istringstream test("fen 1r6/3b3p/p2k3P/Pr1p3R/2p1p3/2K3P1/2P1BP2/R7 w - - 5 37");
                /*
                std::istringstream moves("c5d4 d1d4 g8f6 f1g2 b8c6 d4d2 c8f5 b1c3 e7e6 g1f3 f8b4 f3d4 c6d4 e3d4 e8g8 e1g1 a8c8 a2a3 b4c3 d4c3 h7h6 f2f3 a7a6 a1d1 f5g6 f1f2 g8h7 e2e4 d5e4 d2c1 d8c7 c3f6 e4f3 g2f3 g7f6 d1d4 c7b6 d4b4 b6f2 g1f2 c8c2 c1c2 g6c2 b4b7 h7g6 b7a7 c2d3 f2e3 f8d8 f3e2 d3b5 e2b5 a6b5 a7b7 d8d5 e3e2 f6f5 h2h4 d5e5 e2f3 e5d5 f3e2 d5e5 e2f3 e5c5 f3f4 g6f6 f4e3 c5e5 e3f3 e5c5 f3f4 c5d5 f4e3 d5e5 e3f3 e5c5 f3e3 h6h5 e3d4 c5d5 d4e3 d5e5 e3f3 e5d5 f3e3 f6g7 e3e2 d5e5 e2f3 g7g6 b7b8 e5d5 f3e3 d5e5 e3f3 e5c5 f3f4 c5d5 f4e3 g6g7 e3f4 g7f6 f4e3 d5c5 b8b7 c5d5 b2b4 f6g7 e3e2 g7g6 e2e3 g6f6 e3e2 f6g6 e2e3 g6g7 b7b6 g7f6 b6b7 f6g6 b7b8 d5e5 e3f3 g6g7 f3f2 e5d5 f2e3 d5e5 e3f3 g7g6 b8b7 g6f6 f3f2 e5d5 f2e2 f6g7 e2e3 d5e5 e3f2 g7f6 f2f3 f6g6 b7b8 g6g7 b8b7 e5d5 f3e3 g7g6 b7b8 g6f6");
                std::string s;
                while (moves >> s) {
                    testb.makeMove(chess::uci::uciToMove(testb, s));
                }
            	
                testb.makeMove(chess::uci::uciToMove(testb, "b8b7"));
                int rep = testb.isRepetition(2);
				*/
                //std::istringstream test("fen rn1qkb1r/ppp1ppp1/7p/3p4/6bB/2P2N2/PPP1QPPP/R3KB1R b KQkq - 1 7 moves b8c6 e1c1 a8c8 c1b1 g7g5 h4g3 f8g7 h2h3 g4h5 h3h4 e7e5 h4g5 h6g5 h1h5 h8h5 f3e5 g7e5 e2h5 e5g3 f2g3 d8f6 f1b5 f6e5 d1f1 e5e7 h5h1 e8d7 h1h3 d7d8 h3f5 e7e6 f5f7 e6f7 f1f7 a7a6 b5c6 b7c6 f7g7 d8e8 a2a3 e8f8 g7g5 c8e8 g5f5 f8g7 f5g5 g7f6 g5g4 e8e1 b1a2 a6a5 g4f4 f6g7 f4g4 g7h7 g4f4 h7g7 c3c4 e1e2 c4d5 c6d5 c2c4 c7c6 c4d5 c6d5 f4f5 e2d2 a3a4 d2d3 g3g4 d3d4 a2b1 d4a4 f5d5 a4g4 d5a5 g4g2 a5a7 g7g6 a7d7 g2e2 b1c1 e2e4 c1b1 g6f5 d7f7 f5g4 f7g7 g4f3 g7f7 f3g4 b1c2 e4e2 c2c3 g4g3 b2b4 e2e1 b4b5 e1b1 c3c4 b1b2 c4c5 b2c2 c5b6 c2b2 f7d7 b2b1 d7g7 g3f3 g7f7 f3g3 f7g7 g3f3 b6c6 b1b2 b5b6 b2c2 c6d5 c2d2 d5c4 d2c2 c4d3 c2a2 b6b7 a2b2 g7f7 f3g3 d3c3 b2b5 f7g7 g3f4 g7c7 f4f3 ");
                engine_.tc.winc = 100000000;
                engine_.tc.binc = 100000000;
                setupBoard(test);

                int eval_1 = engine_.b.getEval();
                EvalCounts ec1 = engine_.b.eval_c;

                std::istringstream go_stream("go movetime 500000");
                handleGo(go_stream);

                while (true) {
                    for (auto& position : pos_list) {
                        engine_ = Engine(options);
                        std::istringstream ss(position);
                        setupBoard(ss);
                        std::istringstream go_ss("go movetime 1000");
                        handleGo(go_ss);
                    }
                }
                
                

                
            }
            // Add more UCI commands as needed
        }
        return 1;
    }

    void getEngineUpdate() {
        
        while (true) {
            data_out.lock();

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
    UciOptions options;

    void sendId()
    {
        std::cout << "id name Artisan" << std::endl;
        std::cout << "id author Nia W." << std::endl;
    }

    void sendOptions()
    {
        std::cout << "option name Hash type spin default 16 min 1 max 65536" << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
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