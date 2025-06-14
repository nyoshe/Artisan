#pragma once
#include <fstream>
#include <sstream>
#include "Board.h"
#include "include/chess.hpp"

// Struct for tunable parameters
struct TunableParam
{
    std::string name;
    int value;
    int defaultValue;
    int min;
    int max;
    int step;
};

std::list<TunableParam>& tunables();
TunableParam& addTunableParam(std::string name, int value, int min, int max, int step);
void printWeatherFactoryConfig();

#define TUNABLE_PARAM(name, val, min, max, step) \
    inline TunableParam& name##Param = addTunableParam(#name, val, min, max, step); \
    inline int name() { return name##Param.value; }

TUNABLE_PARAM(PAWN_CORR_WEIGHT, 186, 64, 2048, 32)

class Tuner
{
public:
    Tuner(std::string filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            chess::Board board = chess::Board(line);

            std::istringstream fen(line);
            std::string result;
            while (result[0] != '\"') {
                fen >> result;
            }
            float f_result = 0;
            if (result == "\"1-0\"") f_result = 1;
            if (result == "\"1/2-1/2\"") f_result = 0.5;
            if (result == "\"0-1\"") f_result = 0;

        	pos_vec.push_back({f_result, chess::Board::Compact::encode(board)});
        }
    }

    void computeEvals() {
        Board board;
        for (auto& [result, position] : pos_vec) {
            board.loadBoard(chess::Board::Compact::decode(position));
            result = board.getEval();
        }
    }

    void computeMSE() {
        Board board;
        for (auto& [result, position] : pos_vec) {
            board.loadBoard(chess::Board::Compact::decode(position));
            evals.push_back(board.getEval());
        }
    }

private:
    Board b;
    std::vector < std::pair<float, chess::PackedBoard> > pos_vec;
    std::vector < float > evals;
    std::vector < float > mse;
};
