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
        int count = 0;
        while (std::getline(file, line)) {
            chess::Board board = chess::Board(line);

            std::istringstream fen(line);
            std::string result;
            while (result[0] != '\"') {
                fen >> result;
            }
            float f_result = 0;
            if (result == "\"1-0\";") f_result = 1;
            if (result == "\"1/2-1/2\";") f_result = 0.5;
            if (result == "\"0-1\";") f_result = 0;

        	pos_vec.push_back({f_result, chess::Board::Compact::encode(board)});
            count++;
            if (count % 100000 == 0) {
                std::cout << "loaded " << count << " entries" << std::endl;
            }
        }
        gradient_sum.resize(sizeof(BoardParams) / 4, std::vector<double>(2));
        gradient.resize(sizeof(BoardParams) / 4, std::vector<double>(2));
        lr.resize(sizeof(BoardParams) / 4, std::vector<double>(2));
        params.resize(sizeof(BoardParams) / 4, std::vector<double>(2));

        //init our params
        for (int i = 0; i < params.size(); i++) {
            float mg_val = MG_SCORE(reinterpret_cast<i32*>(&b.params)[i]);
            float eg_val = EG_SCORE(reinterpret_cast<i32*>(&b.params)[i]);
            params[i][0] = mg_val;
            params[i][1] = eg_val;
        }

        double K = 0.0056;
        
        /*
        for (K = 0.005; K < 0.1; K += 0.0001) {
            double MSE = computeMSE(K);
            std::cout << "MSE: " << MSE << " K: " << K << std::endl;
        }
		*/


        for (int i = 0 ; i < 500; i++) {
            float MSE = computeMSE(K);
            computeGradientSums(0.9);
            computeLR(0.1);
            updateValues();
            std::cout << "error: " << MSE << " ";
            for (auto& param : params) {
                std::cout << "{" << int(param[0]) << ", " << int(param[1]) << "} ";
            }
            std::cout << std::endl;
        }
        double MSE = computeMSE(K);
    }

    double computeMSE(double K) {
        double MSE = 0;
        for (auto& val : gradient) {
            val[0] = 0;
            val[1] = 0;
        }
        for (auto& [result, position] : pos_vec ) {
            b.loadBoard(chess::Board::Compact::decode(position));

            double eval = b.us ? -b.getEval() : b.getEval();

            double b_result = 1.0 / (1.0 + std::exp(-K * eval));
            MSE += std::pow(result- b_result  , 2.0);

            updateGradient(K, result, eval, b.eval_c);
        }


        return MSE / pos_vec.size();
    }

    void updateGradient(double K, double result, double eval, const EvalCounts& counts) {
        int N = pos_vec.size();
        double S = 1.0 / (1.0 + std::exp(-K * eval));
        double X = (result - S) * S * (1 - S);
        int i = 0;

        for (auto& val : gradient) {
            
            val[0] += X * reinterpret_cast<const i32*>(&counts)[i]  * (24 - b.getPhase());
            val[1] += X * reinterpret_cast<const i32*>(&counts)[i]  * b.getPhase();
            i++;
        }
    }

    void computeGradientSums(double decay) {
        //incredibly cursed, should be templated or something
        int i = 0;
        for (auto& val : gradient_sum) {
            val[0] = decay * val[0] + (1 - decay) * std::pow(gradient[i][0], 2.0);
            val[1] = decay * val[1] + (1 - decay) * std::pow(gradient[i][1], 2.0);
            i++;
        }
    }

    void computeLR(double global_lr) {
        int i = 0;
        for (int i = 0; i < gradient_sum.size(); i++) {
            lr[i][0] = global_lr / (std::sqrt(gradient_sum[i][0] + 1e-8));
            lr[i][1] = global_lr / (std::sqrt(gradient_sum[i][1] + 1e-8));
        }
    }

    void updateValues() {
        for (int i = 0; i < params.size(); i++) {
            params[i][0] += lr[i][0] * gradient[i][0];
            params[i][1] += lr[i][1] * gradient[i][1];
        }

        for (int i = 0; i < gradient.size(); i++) {
            double mg_val = MG_SCORE(reinterpret_cast<i32*>(&b.params)[i]);
            double eg_val = EG_SCORE(reinterpret_cast<i32*>(&b.params)[i]);
            reinterpret_cast<i32*>(&b.params)[i] = S( params[i][0], params[i][1] );
        }
    }

private:
    Board b;
    std::vector < std::pair<double, chess::PackedBoard> > pos_vec;
    std::vector < std::vector<double> > gradient;
    std::vector < std::vector<double> > gradient_sum;
    std::vector < std::vector<double> > lr;
    std::vector < std::vector<double> > params;
    std::vector < double > errors;
};
