#pragma once

#include <bit>
#include <string>
#include <cstdint>
#include <iostream>
#include <vector>
#include "Tables.h"

#include "Misc.h"

namespace BB {
    // from Grant Osborne, http://www.open-aurec.com/wbforum/viewtopic.php?f=4&t=51162

	inline u64 magic_db[97264];

    inline struct
    {
        unsigned long long factor;
        int position;
    }
    
    bishop_magics[64] = {
       {0x007bfeffbfeffbffull, 16530}, {0x003effbfeffbfe08ull, 9162}, {0x0000401020200000ull, 9674},  
       {0x0000200810000000ull, 18532}, {0x0000110080000000ull, 19172}, {0x0000080100800000ull, 17700},  
       {0x0007efe0bfff8000ull, 5730}, {0x00000fb0203fff80ull, 19661}, {0x00007dff7fdff7fdull, 17065},  
       {0x0000011fdff7efffull, 12921}, {0x0000004010202000ull, 15683}, {0x0000002008100000ull, 17764},  
       {0x0000001100800000ull, 19684}, {0x0000000801008000ull, 18724}, {0x000007efe0bfff80ull, 4108},  
       {0x000000080f9fffc0ull, 12936}, {0x0000400080808080ull, 15747}, {0x0000200040404040ull, 4066},  
       {0x0000400080808080ull, 14359}, {0x0000200200801000ull, 36039}, {0x0000240080840000ull, 20457},  
       {0x0000080080840080ull, 43291}, {0x0000040010410040ull, 5606}, {0x0000020008208020ull, 9497},  
       {0x0000804000810100ull, 15715}, {0x0000402000408080ull, 13388}, {0x0000804000810100ull, 5986},  
       {0x0000404004010200ull, 11814}, {0x0000404004010040ull, 92656}, {0x0000101000804400ull, 9529},  
       {0x0000080800104100ull, 18118}, {0x0000040400082080ull, 5826}, {0x0000410040008200ull, 4620},  
       {0x0000208020004100ull, 12958}, {0x0000110080040008ull, 55229}, {0x0000020080080080ull, 9892},  
       {0x0000404040040100ull, 33767}, {0x0000202040008040ull, 20023}, {0x0000101010002080ull, 6515},  
       {0x0000080808001040ull, 6483}, {0x0000208200400080ull, 19622}, {0x0000104100200040ull, 6274},  
       {0x0000208200400080ull, 18404}, {0x0000008840200040ull, 14226}, {0x0000020040100100ull, 17990},  
       {0x007fff80c0280050ull, 18920}, {0x0000202020200040ull, 13862}, {0x0000101010100020ull, 19590},  
       {0x0007ffdfc17f8000ull, 5884}, {0x0003ffefe0bfc000ull, 12946}, {0x0000000820806000ull, 5570},  
       {0x00000003ff004000ull, 18740}, {0x0000000100202000ull, 6242}, {0x0000004040802000ull, 12326},  
       {0x007ffeffbfeff820ull, 4156}, {0x003fff7fdff7fc10ull, 12876}, {0x0003ffdfdfc27f80ull, 17047},  
       {0x000003ffefe0bfc0ull, 17780}, {0x0000000008208060ull, 2494}, {0x0000000003ff0040ull, 17716},  
       {0x0000000001002020ull, 17067}, {0x0000000040408020ull, 9465}, {0x00007ffeffbfeff9ull, 16196},  
       {0x007ffdff7fdff7fdull, 6166}  
    },  
    rook_magics[64] = {  
       {0x00a801f7fbfeffffull, 85487}, {0x00180012000bffffull, 43101}, {0x0040080010004004ull, 0},  
       {0x0040040008004002ull, 49085}, {0x0040020004004001ull, 93168}, {0x0020008020010202ull, 78956},  
       {0x0040004000800100ull, 60703}, {0x0810020990202010ull, 64799}, {0x000028020a13fffeull, 30640},  
       {0x003fec008104ffffull, 9256}, {0x00001800043fffe8ull, 28647}, {0x00001800217fffe8ull, 10404},  
       {0x0000200100020020ull, 63775}, {0x0000200080010020ull, 14500}, {0x0000300043ffff40ull, 52819},  
       {0x000038010843fffdull, 2048}, {0x00d00018010bfff8ull, 52037}, {0x0009000c000efffcull, 16435},  
       {0x0004000801020008ull, 29104}, {0x0002002004002002ull, 83439}, {0x0001002002002001ull, 86842},  
       {0x0001001000801040ull, 27623}, {0x0000004040008001ull, 26599}, {0x0000802000200040ull, 89583},  
       {0x0040200010080010ull, 7042}, {0x0000080010040010ull, 84463}, {0x0004010008020008ull, 82415},  
       {0x0000020020040020ull, 95216}, {0x0000010020020020ull, 35015}, {0x0000008020010020ull, 10790},  
       {0x0000008020200040ull, 53279}, {0x0000200020004081ull, 70684}, {0x0040001000200020ull, 38640},  
       {0x0000080400100010ull, 32743}, {0x0004010200080008ull, 68894}, {0x0000200200200400ull, 62751},  
       {0x0000200100200200ull, 41670}, {0x0000200080200100ull, 25575}, {0x0000008000404001ull, 3042},  
       {0x0000802000200040ull, 36591}, {0x00ffffb50c001800ull, 69918}, {0x007fff98ff7fec00ull, 9092},  
       {0x003ffff919400800ull, 17401}, {0x001ffff01fc03000ull, 40688}, {0x0000010002002020ull, 96240},  
       {0x0000008001002020ull, 91632}, {0x0003fff673ffa802ull, 32495}, {0x0001fffe6fff9001ull, 51133},  
       {0x00ffffd800140028ull, 78319}, {0x007fffe87ff7ffecull, 12595}, {0x003fffd800408028ull, 5152},  
       {0x001ffff111018010ull, 32110}, {0x000ffff810280028ull, 13894}, {0x0007fffeb7ff7fd8ull, 2546},  
       {0x0003fffc0c480048ull, 41052}, {0x0001ffffa2280028ull, 77676}, {0x00ffffe4ffdfa3baull, 73580},  
       {0x007ffb7fbfdfeff6ull, 44947}, {0x003fffbfdfeff7faull, 73565}, {0x001fffeff7fbfc22ull, 17682},  
       {0x000ffffbf7fc2ffeull, 56607}, {0x0007fffdfa03ffffull, 56135}, {0x0003ffdeff7fbdecull, 44989},  
       {0x0001ffff99ffab2full, 21479}  
    };

    inline u64 bishop_blocker_mask[64];
    inline u64 rook_blocker_mask[64];

    //coverage for bishop and rook for checking potentially revealed attacks
    inline u64 bishop_coverage[64];
    inline u64 rook_coverage[64];


    inline u64 knight_attacks[64];
	inline u64 king_attacks[64];

    
    inline u64 files[8];
    inline u64 ranks[8];
    inline u64 neighbor_files[8];


    inline Pos dirs[8] = {
        {  0,  1 }, // up
        {  0, -1 }, // down
        {  1,  0 }, // right
		{ -1,  0 }, // left
        {  1,  1 }, // up-right
        { -1,  1 }, // up-left
        {  1, -1 }, // down-right
		{ -1, -1 }  // down-left
	};

    inline std::vector<int> get_set_bits(u64 board) {
        std::vector<int> bits;
#if defined(_MSC_VER)
        unsigned long idx;
        while (board) {
            _BitScanForward64(&idx, board);
            bits.emplace_back(static_cast<int>(idx));
            board &= board - 1;
        }
#elif defined(__GNUC__) || defined(__clang__)
        while (board) {
            int idx = __builtin_ctzll(board);
            bits.push_back(idx);
            board &= board - 1;
        }
#else
        // Portable fallback (not using intrinsics)
        for (int i = 0; i < 64; ++i) {
            if (board & (1ULL << i))
                bits.push_back(i);
        }
#endif
        return bits;
    }

    inline void bitscan_reset(unsigned long &idx, u64 &board) {
#if defined(_MSC_VER)
            _BitScanForward64(&idx, board);
            board &= board - 1;
            return;
#elif defined(__GNUC__) || defined(__clang__)
            idx = __builtin_ctzll(board);
            board &= board - 1;
			return idx
#else
        // Portable fallback (not using intrinsics)
        for (int i = 0; i < 64; ++i) {
            if (board & (1ULL << i)) {
                board &= board - 1;
                idx = i;
                return;
            }
                
        }
#endif
        return;
    }

    inline int bitscan(u64 board) {
#if defined(_MSC_VER)
        unsigned long idx;
        if (_BitScanForward64(&idx, board)) {
            return static_cast<int>(idx);
        }
#elif defined(__GNUC__) || defined(__clang__)
        if (board) {
            return __builtin_ctzll(board);
        }
#else
        for (int i = 0; i < 64; ++i) {
            if (board & (1ULL << i)) {
                return i;
            }
        }
#endif
        return -1; // No bits set
    }

    inline std::string to_string(const u64 board) {
        std::string out = "";
        for (int rank = 7; rank >= 0; rank--) {
            for (int file = 0; file <= 7; file++) {
                if (board & (1ULL << (file | (rank << 3)))) {
                    out += "1 ";
                }
                else {
                    out += ". ";
                }
            }
            out += "\n";
        }
        return out;
    };

    inline std::string rank_to_string(const u64 board, const int rank) {
        std::string out = "";
        for (int file = 0; file <= 7; file++) {
            if (board & (1ULL << (file | (rank << 3)))) {
                out += "1 ";
            }
            else {
                out += ". ";
            }
        }
        return out;
    };

    inline int popcnt(const u64 board) {
        return std::popcount(board);
    }

	inline u64 set_bit(const int square) {
		return 1ULL << square;
	}

    inline void init()
    {


        for (int i = 0; i < 7; i++) {
            for (int square = 0; square < 64; square++) {
                mg_table[i][square] += mg_p_val[i];
                eg_table[i][square] += eg_p_val[i];
            }
        }
        // Rook directions: N, S, E, W
        const Direction rook_dirs[] = { eNorth, eSouth, eEast, eWest };
        // Bishop directions: NE, NW, SE, SW
        const Direction bishop_dirs[] = { eNorthEast, eNorthWest, eSouthEast, eSouthWest };

        // Initialize bishop and rook blocker masks
        for (int i = 0; i < 64; i++) {
            bishop_blocker_mask[i] = 0;
            rook_blocker_mask[i] = 0;
            Pos origin(i);

            // Bishop blocker mask
            for (const auto& dir : bishop_dirs) {
                Pos p = origin + dirs[static_cast<int>(dir)];
                while (p.f >= 1 && p.f < 7 && p.r >= 1 && p.r < 7) {
                    bishop_blocker_mask[i] |= set_bit(p.toSquare());
                    p += dirs[static_cast<int>(dir)];
                }
            }

            // Rook blocker mask
            for (const auto& dir : rook_dirs) {
                Pos p = origin + dirs[static_cast<int>(dir)];
                while (p.f >= (origin.f == 0 ? 0 : 1) && p.f < (origin.f == 7 ? 8 : 7) &&
						p.r >= (origin.r == 0 ? 0 : 1) && p.r < (origin.r == 7 ? 8 : 7)) {
                    rook_blocker_mask[i] |= set_bit(p.toSquare());
                    p += dirs[static_cast<int>(dir)];
                }
            }
        }

        // Rook and bishop coverage
        for (int i = 0; i < 64; ++i) {
            rook_coverage[i] = 0;
            bishop_coverage[i] = 0;
            Pos origin(i);

            // Rook coverage
            // Right
            for (int f = origin.f + 1; f < 8; ++f)
                rook_coverage[i] |= set_bit(Pos(f, origin.r).toSquare());
            // Left
            for (int f = origin.f - 1; f >= 0; --f)
                rook_coverage[i] |= set_bit(Pos(f, origin.r).toSquare());
            // Up
            for (int r = origin.r + 1; r < 8; ++r)
                rook_coverage[i] |= set_bit(Pos(origin.f, r).toSquare());
            // Down
            for (int r = origin.r - 1; r >= 0; --r)
                rook_coverage[i] |= set_bit(Pos(origin.f, r).toSquare());

            // Bishop coverage
            for (const auto& dir : bishop_dirs) {
                Pos p = origin + dirs[static_cast<int>(dir)];
                while (p.f >= 0 && p.f < 8 && p.r >= 0 && p.r < 8) {
                    bishop_coverage[i] |= set_bit(p.toSquare());
                    p += dirs[static_cast<int>(dir)];
                }
            }
        }

        // Knight attacks
        for (int i = 0; i < 64; i++) {
            knight_attacks[i] = 0;
            Pos origin(i);
            // Knight moves
            const int knight_moves[8][2] = {
                {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
            };
            for (auto& move : knight_moves) {
                Pos p(origin.f + move[0], origin.r + move[1]);
                if (p.f >= 0 && p.f < 8 && p.r >= 0 && p.r < 8) {
                    knight_attacks[i] |= set_bit(p.toSquare());
                }
            }
        }

        // King attacks
        for (int i = 0; i < 64; i++) {
            king_attacks[i] = 0;
            Pos origin(i);
            // King moves
            const int king_moves[8][2] = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1},
                {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
            };
            for (auto& move : king_moves) {
                Pos p(origin.f + move[0], origin.r + move[1]);
                if (p.f >= 0 && p.f < 8 && p.r >= 0 && p.r < 8) {
                    king_attacks[i] |= set_bit(p.toSquare());

                }
            }

        }

        //init file masks
        for (int file = 0; file < 8; file++) {
            files[file] = 0;
            files[file] |= u64(0x0101010101010101) << file;
        }

        //init rank masks
        for (int rank = 0; rank < 8; rank++) {
            ranks[rank] = 0;
            ranks[rank] |= u64(0x00000000000000FF) << (rank * 8);
        }

        for (int file = 0; file < 8; file++) {
            neighbor_files[file] = (file < 7 ? BB::files[file + 1] : 0) | (file > 0 ? BB::files[file - 1] : 0);
        }

        std::vector<std::vector<u64>> blocker_combos;
        
        for (int square = 0; square < 64; square++) {
            std::vector<int> relevant = get_set_bits(rook_blocker_mask[square]);
            size_t num = relevant.size();
            std::vector<u64> combinations;
            combinations.reserve(1ULL << num);

            for (u64 mask = 0; mask < (1ULL << num); ++mask) {
                u64 blockers = 0;
                for (size_t i = 0; i < num; ++i) {
                    if (mask & (1ULL << i)) {
                        blockers |= set_bit(relevant[i]);
                    }
                }
                combinations.push_back(blockers);
            }
            blocker_combos.push_back(combinations);
        }

        for (int square = 0; square < 64; ++square) {
            std::vector<int> relevant = get_set_bits(rook_blocker_mask[square]);
            size_t num = relevant.size();
            u64 magic = rook_magics[square].factor;

            for (size_t mask = 0; mask < blocker_combos[square].size(); ++mask) {
                u64 blockers = 0;
                for (size_t i = 0; i < num; ++i) {
                    if (mask & (1ULL << i)) {
                        blockers |= set_bit(relevant[i]);
                    }
                }

                // Generate attack set for this blocker configuration
                u64 attacks = 0;
                int file = square & 7;
                int rank = square >> 3;

                // Right
                for (int f = file + 1; f < 8; ++f) {
                    int sq = f | (rank << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Left
                for (int f = file - 1; f >= 0; --f) {
                    int sq = f | (rank << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Up
                for (int r = rank + 1; r < 8; ++r) {
                    int sq = file | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Down
                for (int r = rank - 1; r >= 0; --r) {
                    int sq = file | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                
                // Compute the magic index for this blocker set
                u64 magic_index = (blockers * magic) >> 52;

                magic_db[rook_magics[square].position + magic_index] = attacks;
                
            }
        }
        // Bishop magic bitboard database initialization
        std::vector<std::vector<u64>> bishop_blocker_combos;
        for (int square = 0; square < 64; square++) {
            std::vector<int> relevant = get_set_bits(bishop_blocker_mask[square]);
            size_t num = relevant.size();
            std::vector<u64> combinations;
            combinations.reserve(1ULL << num);

            for (u64 mask = 0; mask < (1ULL << num); ++mask) {
                u64 blockers = 0;
                for (size_t i = 0; i < num; ++i) {
                    if (mask & (1ULL << i)) {
                        blockers |= set_bit(relevant[i]);
                    }
                }
                combinations.push_back(blockers);
            }
            bishop_blocker_combos.push_back(combinations);
        }

        for (int square = 0; square < 64; ++square) {
            std::vector<int> relevant = get_set_bits(bishop_blocker_mask[square]);
            size_t num = relevant.size();
            u64 magic = bishop_magics[square].factor;

            for (size_t mask = 0; mask < bishop_blocker_combos[square].size(); ++mask) {
                u64 blockers = 0;
                for (size_t i = 0; i < num; ++i) {
                    if (mask & (1ULL << i)) {
                        blockers |= set_bit(relevant[i]);
                    }
                }

                // Generate attack set for this blocker configuration
                u64 attacks = 0;
                int file = square & 7;
                int rank = square >> 3;

                // Top-right
                for (int f = file + 1, r = rank + 1; f < 8 && r < 8; ++f, ++r) {
                    int sq = f | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Top-left
                for (int f = file - 1, r = rank + 1; f >= 0 && r < 8; --f, ++r) {
                    int sq = f | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Bottom-right
                for (int f = file + 1, r = rank - 1; f < 8 && r >= 0; ++f, --r) {
                    int sq = f | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }
                // Bottom-left
                for (int f = file - 1, r = rank - 1; f >= 0 && r >= 0; --f, --r) {
                    int sq = f | (r << 3);
                    attacks |= set_bit(sq);
                    if (blockers & set_bit(sq)) break;
                }

                // Compute the magic index for this blocker set
                u64 magic_index = (blockers * magic) >> 55;

                magic_db[bishop_magics[square].position + magic_index] = attacks;
            }
        }
    }

    // retrieve attacks for rooks as bitboard
    inline u64 get_rook_attacks(const int square, const u64 occupancy) {
        // Compute magic index
        u64 magic_index = ((rook_blocker_mask[square] & occupancy) * rook_magics[square].factor) >> 52;
        // Retrieve from database
        return magic_db[rook_magics[square].position + magic_index];
    }

	// retrieve attacks for bishops as bitboard
    inline u64 get_bishop_attacks(const int square, const u64 occupancy) {
        // Compute magic index
        u64 magic_index = ((bishop_blocker_mask[square] & occupancy) * bishop_magics[square].factor) >> 55;
        // Retrieve from database
        return magic_db[bishop_magics[square].position + magic_index];
    }

    // retrieve attacks for queen as bitboard
    inline u64 get_queen_attacks(const int square, const u64 occupancy) {
        return get_rook_attacks(square, occupancy) | get_bishop_attacks(square, occupancy);
    }

    inline u64 get_pawn_attacks(const Direction dir, const Side side, const u64 pawns, const u64 occupancy) {
	    if (dir == eEast) {
            return ((side == eWhite) ? ((pawns & ~BB::files[7]) << 9) : ((pawns & ~BB::files[7]) >> 7)) & occupancy;
	    }
        else if (dir == eWest) {
            return ((side == eWhite) ? ((pawns & ~BB::files[0]) << 7) : ((pawns & ~BB::files[0]) >> 9)) & occupancy;
        }
    }
}

