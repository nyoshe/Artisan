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
#include "robin_hood.h"
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
    std::array<u8, 64> piece_board;
    int eval = 0;
    u64 hash = 0;
    u8 castle_flags = 0b1111;
    int ep_square = -1; // -1 means no en passant square, ep square represents piece taken
public:
    int tunable = 0;
    std::vector<BoardState> state_stack;
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

    // Returns a Move object corresponding to the given UCI string (e.g. "e2e4", "e7e8q").
    Move moveFromSan(const std::string& san);
    std::string sanFromMove(Move move);
    Move moveFromUCI(const std::string& uci);
    void loadUci(std::istringstream& uci);
    void genPseudoLegalCaptures(StaticVector<Move>& moves);
    void serializeMoves(Piece piece, StaticVector<Move>& moves, bool quiet);

    void genPseudoLegalMoves(StaticVector<Move>& moves);
    void filterToLegal(StaticVector<Move>& pseudo_moves);
    bool isLegal(Move move);


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
        eval = evalFullUpdate();
	    return us == eWhite ? eval : -eval;
    };


    void runSanityChecks() const;
    void printMoves() const;
    void reset();

    std::vector<Move> getLastMoves(int n_moves) const;

    u64 getHash() const;
    bool is3fold();

    [[nodiscard]] u64 calcHash() const;

    void updateZobrist(Move move);

    int getMobility(bool side) const;

    int evalFullUpdate() {
        int out = 0;

        //tempo
        out += 18;

        int16_t game_phase = 24 -
            BB::popcnt(boards[eWhite][eKnight]) -
            BB::popcnt(boards[eWhite][eBishop]) -
            BB::popcnt(boards[eWhite][eRook]) * 2 -
            BB::popcnt(boards[eWhite][eQueen]) * 4 -
            BB::popcnt(boards[eBlack][eKnight]) -
            BB::popcnt(boards[eBlack][eBishop]) -
            BB::popcnt(boards[eBlack][eRook]) * 2 -
            BB::popcnt(boards[eBlack][eQueen]) * 4
            ;

        int mg_val = 0;
        int eg_val = 0;
        u64 white_squares = boards[eWhite][0];
        unsigned long at;
        while (white_squares) {
            BB::bitscan_reset(at, white_squares);
            u8 p = piece_board[at];
            u8 sq = at ^ 56;
            out += S(mg_table[p][sq], eg_table[p][sq]);
        }

        u64 black_squares = boards[eBlack][0];
        at = 0;
        while (black_squares) {
            BB::bitscan_reset(at, black_squares);
            u8 p = piece_board[at];
            u8 sq = at;
            out -= S(mg_table[p][sq], eg_table[p][sq]);
        }

        //count doubled pawns
        //count isolated and doubled
        for (int file = 0; file < 8; file++) {
            /*
            if (boards[eWhite][ePawn] & BB::files[file])
                out -= (boards[eWhite][ePawn] & BB::neighbor_files[file]) ? 0 : 10;
            if (boards[eBlack][ePawn] & BB::files[file])
                out += (boards[eBlack][ePawn] & BB::neighbor_files[file]) ? 0 : 10;
			*/
            out += S(-11, -48) *
                ((BB::popcnt(boards[eWhite][ePawn] & BB::files[file]) >= 2) -
            	 (BB::popcnt(boards[eBlack][ePawn] & BB::files[file]) >= 2));
        }

        u64 occ = boards[eBlack][0] | boards[eWhite][0];
        //count defenders
        u64 w_east_defenders = BB::get_pawn_attacks(eEast, eWhite, boards[eWhite][ePawn], boards[eWhite][ePawn]);
        u64 w_west_defenders = BB::get_pawn_attacks(eWest, eWhite, boards[eWhite][ePawn], boards[eWhite][ePawn]);
        u64 b_east_defenders = BB::get_pawn_attacks(eEast, eBlack, boards[eBlack][ePawn], boards[eBlack][ePawn]);
        u64 b_west_defenders = BB::get_pawn_attacks(eWest, eBlack, boards[eBlack][ePawn], boards[eBlack][ePawn]);
        
        //single defenders
        out += S(22,17) * (BB::popcnt(w_east_defenders | w_west_defenders) - BB::popcnt(b_east_defenders | b_west_defenders));

        //bishop pair
        out += S(33, 110) * ((BB::popcnt(boards[eWhite][eBishop]) == 2) - (BB::popcnt(boards[eBlack][eBishop]) == 2));

        int white_king_sq = BB::bitscan(boards[eWhite][eKing]);
        int black_king_sq = BB::bitscan(boards[eBlack][eKing]);

        
        auto king_safety = [&](int sq, bool side) {
            u64 king_acc = BB::king_attacks[sq];
            king_acc = (~boards[side][0] & (king_acc | BB::set_bit(sq))) << (side ? -8 : 8);
            king_acc = (~boards[side][0] & (king_acc | BB::set_bit(sq))) << (side ? -8 : 8);
            king_acc = (~boards[side][0] & (king_acc | BB::set_bit(sq))) << (side ? -8 : 8);
            return (king_acc | BB::king_attacks[sq]) & ~BB::set_bit(sq);
            };
        static const int SafetyTable[100] = {
    0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
  18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
  68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
 140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
 260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
 377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
 494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
        };

        auto king_attack_val = [&](bool side) {
            int king_sq = BB::bitscan(boards[side][eKing]);
            u64 occ = getOccupancy();
            u64 knights = boards[!side][eKnight];
            u64 bishops = boards[!side][eBishop];
            u64 rooks = boards[!side][eRook];
            u64 queens = boards[!side][eQueen];
            u64 king_zone = king_safety(king_sq, side);
            unsigned long at = 0;
            int attack_val = 0;
            int num_attackers = 0;
            static const int CountModifier[8] = { 0, 0, 63, 126, 96, 124, 124, 128 };
            while (knights) {
                BB::bitscan_reset(at, knights);
                attack_val += 36 * BB::popcnt(BB::knight_attacks[at] & ~boards[side][0] & king_zone);
                num_attackers++;
            }
            at = 0;
            while (bishops) {
                BB::bitscan_reset(at, bishops);
                attack_val += 22 * BB::popcnt(BB::get_bishop_attacks(at, occ) & ~boards[side][0] & king_zone);
                num_attackers++;
            }
            at = 0;
            while (rooks) {
                BB::bitscan_reset(at, rooks);
                attack_val += 23 * BB::popcnt(BB::get_rook_attacks(at, occ) & ~boards[side][0] & king_zone);
                num_attackers++;
            }
            at = 0;
            while (queens) {
                BB::bitscan_reset(at, queens);
                attack_val += 78 * BB::popcnt(BB::get_queen_attacks(at, occ) & ~boards[side][0] & king_zone);
                num_attackers++;
            }

            int danger = (CountModifier[std::min(num_attackers, 7)] * attack_val) / 128;
            return danger;
            };
        
        //get attacks, ignoring our pieces
        out -= S(king_attack_val(eWhite), 0);
        out += S(king_attack_val(eBlack), 0);

        
        /*
        static u64 pawn_shield_white_king =
            BB::set_bit(f2) | BB::set_bit(g2) | BB::set_bit(h2) |
            BB::set_bit(f3) | BB::set_bit(g3) | BB::set_bit(h3);
        static u64 pawn_shield_black_king =
            BB::set_bit(f7) | BB::set_bit(g7) | BB::set_bit(h7) |
            BB::set_bit(f6) | BB::set_bit(g6) | BB::set_bit(h6);
        static u64 pawn_shield_white_queen =
            BB::set_bit(a2) | BB::set_bit(b2) | BB::set_bit(c2) |
            BB::set_bit(a3) | BB::set_bit(b3) | BB::set_bit(c3);
        static u64 pawn_shield_black_queen =
            BB::set_bit(a7) | BB::set_bit(b7) | BB::set_bit(c7) |
            BB::set_bit(a6) | BB::set_bit(b6) | BB::set_bit(b6);

        out += S(31, -12) * (white_king_sq == g1 && (BB::popcnt(pawn_shield_white_king & boards[eWhite][ePawn]) >= 3));
        out -= S(31, -12) * (black_king_sq == g8 && (BB::popcnt(pawn_shield_black_king & boards[eBlack][ePawn]) >= 3));

        out += S(31, -12) * (white_king_sq == b1 && (BB::popcnt(pawn_shield_white_queen & boards[eWhite][ePawn]) >= 3));
        out -= S(31, -12) * (black_king_sq == b8 && (BB::popcnt(pawn_shield_black_queen & boards[eBlack][ePawn]) >= 3));

		*/
        /*
        auto mobility = [&](bool side) {
            int mob = 0;
            u64 occ = getOccupancy();
            u64 knights = boards[side][eKnight];
            u64 bishops = boards[side][eBishop];
            u64 rooks = boards[side][eRook];
            u64 queens = boards[side][eQueen];
            while (knights) {
                BB::bitscan_reset(at, knights);
                mob += BB::popcnt(BB::knight_attacks[at] & ~boards[side][0]);
            }
            while (bishops) {
                BB::bitscan_reset(at, bishops);
                mob += BB::popcnt(BB::get_bishop_attacks(at, occ) & ~boards[side][0]);
            }
            while (rooks) {
                BB::bitscan_reset(at, rooks);
                mob += BB::popcnt(BB::get_rook_attacks(at, occ) & ~boards[side][0]);
            }
            while (queens) {
                BB::bitscan_reset(at, queens);
                mob += BB::popcnt(BB::get_queen_attacks(at, occ) & ~boards[side][0]);
            }
            return mob;
            };
        out += 2 * (mobility(eWhite) - mobility(eBlack));
		*/
    	//out += 4 * (getMobility(eWhite) - getMobility(eBlack));
        //double defenders
        //out += var * (BB::popcnt(w_east_defenders & w_west_defenders) - BB::popcnt(b_east_defenders & b_west_defenders));
        //out = us == eWhite ? out : -out;
        out = ((24 - game_phase) * MG_SCORE(out)) / 24  + (game_phase * EG_SCORE(out)) / 24;
        return out;
    }
};

