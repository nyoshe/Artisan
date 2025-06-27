#pragma once
#define NOMINMAX
#include "Misc.h"
#include "BitBoard.h"
#include "Move.h"
#include "Tables.h"
#include <iostream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <cassert>
#include <sstream>
#include <ranges>
#include <limits>
#include "Memory.h"
#include "include/chess.hpp"
#undef min
#undef max

static int32_t constexpr S(const int mg, const int eg) { return static_cast<int32_t>(static_cast<uint32_t>(eg) << 16) + (mg); };
static int32_t constexpr MG_SCORE(const int s) {
    return static_cast<int16_t>(static_cast<uint16_t>(static_cast<unsigned>((s))));
}
static int32_t constexpr EG_SCORE(const int s) {
	return static_cast<int16_t>(static_cast<uint16_t>(static_cast<unsigned>((s) + 0x8000) >> 16));
}

inline u64 rnd64()
{
    static u64 i = 69;
    return (i = (164603309694725029ull * i) % 14738995463583502973ull);
}

struct BoardParams {
    int32_t tempo = S(22, 45);
    int32_t doubled_pawns = S(1, 48);
    int32_t passed_pawns = S(-30, 71);
    int32_t defender_pawns = S(12, 22);
    int32_t double_defender_pawns = S(-15, 10);
    int32_t bishop_pair = S(39, 79);
    int32_t mobility[5] = { S(7,-1), S(6, 0), S(8, 2), S(2, 4), S(-15, 14) };
    int32_t captures[5] = { S(-5,31), S(3, 18), S(1, 23), S(-10, 17), S(-87, 84) };
    int32_t isolated_pawns = S(25, -47);

    //int32_t piece[6] = {S(1,1), S(10 ,20), S(10,20), S(10,20), S(10,20) , S(10,20)};
    //int32_t phase_values = S(100,100);
};

struct EvalCounts {
    int32_t tempo = 0;
    int32_t doubled_pawns = 0;
    int32_t passed_pawns = 0;
    int32_t defender_pawns = 0;
    int32_t double_defender_pawns = 0;
    int32_t bishop_pair = 0;
    int32_t mobility[5];
    int32_t captures[5];
    int32_t isolated_pawns = 0;
    //int32_t piece[6];
    //int32_t phase_values = S(100,100);
};

struct BoardState {
    u64 hash = 0;
    i8 ep_square = -1;
    u8 castle_flags = 0b1111; // 0bKQkq
    Move move;
    int eval = 0;
    u16 half_move;

    BoardState(int ep_square, u8 castle_flags, Move move, int eval, u64 hash, u16 half_move)
        : ep_square(ep_square), castle_flags(castle_flags), move(move), eval(eval), hash(hash), half_move(half_move) {
    };

    auto operator<=>(const BoardState&) const = default;
};

struct Zobrist {
    std::array<u64, 12 * 64> piece_at;
    u64 side;
    std::array<u64, 16> castle_rights;
    std::array<u64, 8> ep_file;
};

class Board
{
private:
    // boards[side][0] = occupancy
    //256 seems big enough, right?
    //std::vector<Move> legal_moves;
    
    std::array < std::array<u64, 7>, 2 > boards;

    static Zobrist initZobristValues() {
        Zobrist z;

        for (auto& val : z.piece_at) val = rnd64();
        z.side = rnd64();
        for (auto& val : z.castle_rights) val = rnd64();
        for (auto& val : z.ep_file) val = rnd64();

        return z;
    }

    Zobrist z = initZobristValues();
    std::array<u8, 64> mailbox;
    int eval = 0;
    u64 hash = 0;
    u8 castle_flags = 0b1111;
    int ep_square = -1; // -1 means no en passant square, ep square represents piece taken

public:
    EvalCounts eval_c;
    BoardParams params;
    std::vector<BoardState> state_stack;
    std::string start_fen;
    bool us = eWhite;
    int ply = 0;
    u16 half_move = 0;

    Board();
    // Copy constructor
    Board(const Board& other) = default;

    // Equality operator
    bool operator==(const Board& other) const;

    void setOccupancy();

    //fancy print
    void printBoard() const;
    void printBitBoards() const;
    [[nodiscard]] std::string boardString() const; //outputs board string in standard ascii

    void doMove(Move move);
    void undoMove();
    void movePiece(u8 from, u8 to);
    void setPiece(u8 square, u8 color, u8 piece);
    void removePiece(u8 square);

    [[nodiscard]] Side getSide(int square) const;
    void loadFen(std::istringstream& fen_stream);
    void loadBoard(chess::Board new_board);
    // Returns a Move object corresponding to the given UCI string (e.g. "e2e4", "e7e8q").
    [[nodiscard]] Move moveFromUCI(const std::string& uci);
    void loadUci(std::istringstream& uci);
    void genPseudoLegalCaptures(StaticVector<Move>& moves);
    void serializeMoves(Piece piece, StaticVector<Move>& moves, bool quiet);

    void genPseudoLegalMoves(StaticVector<Move>& moves);
    void filterToLegal(StaticVector<Move>& pseudo_moves);
    [[nodiscard]] bool isLegal(Move move);
    [[nodiscard]] int staticExchangeEvaluation(Move move, int threshold);

    //get index of all attackers of a square
    [[nodiscard]] u64 getAttackers(int square) const;
    [[nodiscard]] u64 getAttackers(int square, bool side) const;
    [[nodiscard]] bool isCheck() const;

    [[nodiscard]] u64 getOccupancy() const {
        return boards[eWhite][0] | boards[eBlack][0];
    };

    [[nodiscard]] u64 getPieceBoard(Piece piece) const {
        return boards[eWhite][piece] | boards[eBlack][piece];
    }

    //incrementally update eval
    [[nodiscard]] int evalUpdate(Move move) ;

    [[nodiscard]] int getEval()  {
        eval = evalUpdate();
	    return us == eWhite ? eval : -eval;
    };

    [[nodiscard]] int getPhase() const {
        return 24 -
            BB::popcnt(boards[eWhite][eKnight]) -
            BB::popcnt(boards[eWhite][eBishop]) -
            BB::popcnt(boards[eWhite][eRook]) * 2 -
            BB::popcnt(boards[eWhite][eQueen]) * 4 -
            BB::popcnt(boards[eBlack][eKnight]) -
            BB::popcnt(boards[eBlack][eBishop]) -
            BB::popcnt(boards[eBlack][eRook]) * 2 -
            BB::popcnt(boards[eBlack][eQueen]) * 4
            ;
    }

    void runSanityChecks() const;
    void printMoves() const;
    void reset();

    [[nodiscard]] std::vector<Move> getLastMoves(int n_moves) const;

    [[nodiscard]] u64 getHash() const;
    bool isRepetition(int n) const;

    [[nodiscard]] u64 calcHash() const;

    void updateZobrist(Move move);

    int getMobility(bool side) ;

    int evalUpdate() ;
};

