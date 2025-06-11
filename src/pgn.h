#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <fstream>
#include <algorithm>
#include <array>

//I'm gonna be honest, this was almost entirely AI generated, I can't be bothered with parsers

namespace pgn
{
    struct PgnMove
    {
        std::string san;
        int ply;
        int eval;
        bool isCheck;
        bool isCheckmate;
        bool isStalemate;
    };



    inline std::vector<PgnMove> parse_pgn_moves(const std::string& pgnText)
    {
        std::vector<PgnMove> moves;
        std::istringstream iss(pgnText);
        std::string token;
        int ply = 1;

        // PGN results to skip - using array instead of vector for efficiency
        const std::array<std::string_view, 4> results = { "1-0", "0-1", "1/2-1/2", "*" };

        while (iss >> token)
        {
            // Handle eval comments: { [%eval 0.23] }
            if (token == "{")
            {
                std::string comment;
                std::string word;
                // Read until closing '}'
                while (iss >> word && word != "}")
                    comment += word + " ";

                // Process evaluation if present and we have moves
                auto evalPos = comment.find("[%eval");
                if (evalPos != std::string::npos && !moves.empty())
                {
                    std::string evalStr = comment.substr(evalPos + 6);
                    // Clean up the evaluation string
                    evalStr.erase(0, evalStr.find_first_not_of(" ["));
                    evalStr.erase(evalStr.find_last_not_of(" ]") + 1);

                    try {
                        if (!evalStr.empty() && evalStr[0] == '#') {
                            // Handle mate score: #5 or #-5
                            int mateDistance = std::stoi(evalStr.substr(evalStr[1] == '-' ? 2 : 1));
                            moves.back().eval = evalStr[1] == '-' ?
                                -100000 + mateDistance : 100000 - mateDistance;
                        }
                        else {
                            // Handle regular eval score
                            moves.back().eval = static_cast<int>(std::stod(evalStr) * 100); // convert to centipawns
                        }
                    }
                    catch (...) {
                        moves.back().eval = 0; // Default on parsing error
                    }
                }
                continue;
            }

            // Skip tokens that aren't moves
            if (token.empty() ||
                (std::isdigit(token[0]) && token.find('.') != std::string::npos) ||
                std::find(results.begin(), results.end(), token) != results.end() ||
                token[0] == '(')
                continue;

            // Remove trailing annotations
            if (size_t end = token.find_first_of("!?#$"); end != std::string::npos)
                token = token.substr(0, end);

            // Extract check/checkmate indicators
            bool isCheck = false, isCheckmate = false;

            // Process ending characters for check/checkmate
            while (!token.empty() && (token.back() == '+' || token.back() == '#')) {
                isCheck = token.back() == '+' ? true : isCheck;
                isCheckmate = token.back() == '#' ? true : isCheckmate;
                token.pop_back();
            }

            if (!token.empty()) {
                moves.push_back({
                    token,       // san
                    ply++,       // ply
                    0,           // eval (will be set if a comment follows)
                    isCheck,     // isCheck
                    isCheckmate, // isCheckmate
                    false        // isStalemate 
                    });
            }
        }
        return moves;
    }
    inline std::vector<std::vector<PgnMove>> read_pgn_file(std::ifstream& file)
    {
        std::vector<std::vector<PgnMove>> gamesMoves;
        std::ostringstream currentGame;
        bool inGame = false;
        std::string line;

        // Extract moves section from game text (header-free content)
        auto extract_moves_section = [](const std::string& gameText) {
            std::istringstream iss(gameText);
            std::string line, movesSection;
            bool movesStarted = false;

            while (std::getline(iss, line)) {
                if (!movesStarted && (line.empty() || line[0] == '['))
                    continue;

                movesStarted = true;
                if (!line.empty())
                    movesSection += line + ' ';
            }
            return movesSection;
            };

        // Process game when complete
        auto process_game = [&]() {
            if (inGame && !currentGame.str().empty()) {
                std::string movesText = extract_moves_section(currentGame.str());
                gamesMoves.push_back(parse_pgn_moves(movesText));
                currentGame.str("");
                currentGame.clear();
            }
            };

        while (std::getline(file, line)) {
            // Detect the start of a new game
            if (line.rfind("[Event", 0) == 0) {
                process_game(); // Process previous game if exists
                inGame = true;
            }

            if (inGame)
                currentGame << line << '\n';
        }

        // Process the last game if present
        process_game();

        return gamesMoves;
    }
}