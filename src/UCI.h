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
                static std::vector<std::string> fens = { "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
                              "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
                              "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
                              "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
                              "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
                              "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
                              "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
                              "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
                              "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
                              "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
                              "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
                              "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
                              "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
                              "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
                              "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
                              "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
                              "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
                              "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
                              "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
                              "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
                              "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
                              "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
                              "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
                              "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
                              "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
                              "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
                              "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
                              "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
                              "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
                              "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
                              "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
                              "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
                              "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
                              "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
                              "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/3R4 w - - 6 54",
                              "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
                              "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
                              "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
                              "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
                              "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
                              "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
                              "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
                              "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
                              "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
                              "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
                              "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
                              "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
                              "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
                              "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
                              "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93" };
                //std::istringstream test("fen r2qkb1r/p1pp1ppp/1p2pn2/8/1nPP4/2N1PP2/PPQ2P1P/R1B1KB1R w KQkq - 1 8 moves c2d2 c7c5 d4d5 f8d6 f1g2 e6d5 a2a3 b4a6 c4d5 e8g8 e1g1 f8e8 f3f4 a6c7 e3e4 f6h5 e4e5 d6f8 d5d6 c7e6 g2a8 d8a8 d2d5 h5f4 d5a8 e8a8 f1e1 f4d3 e1e3 c5c4 c3b5 e6c5 c1d2 d3b2 d2b4 c5d3 b5c7 a8c8 b4c3 b2a4 c3d4 h7h5 e3e4 c8d8 a1a2 f7f5 e5f6 f8d6 e4e8 d8e8 c7e8 d6c5 d4c5 a4c5 f6g7 d3f4 f2f3 c4c3 a2c2 h5h4 g1h1 f4d5 e8d6 h4h3 d6f5 g8h7 c2c1 c5d3 c1c2 b6b5 c2e2 d5c7 e2c2 c7d5 c2e2");
                chess::Board testb("r1bqk1nr/ppp2ppp/2nb4/3pp3/8/5P1P/PPPPP1PK/R1BQ1BNR b kq - 2 5 ");
                std::istringstream test("startpos moves e2e4 e7e6 b1c3 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 c1e3 a7a6 f1d3 f8e7 e1g1 e8g8 a2a4 b7b6 d1f3 c8b7 f3g3 b8d7 a4a5 b6b5 e3h6 f6e8");
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
                    for (auto& position : fens) {
                        engine_ = Engine(options);
                        std::istringstream ss(position);
                        setupBoard(ss);
                        std::istringstream go_ss("go movetime 1000");
                        handleGo(go_ss);
                    }
                }
            }
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