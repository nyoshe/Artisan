#pragma once
#include <cstdint>
#include <string>
#include <cstddef>
#include <array>

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;
using usize = std::size_t;

static constexpr i16 EVAL_MAX = INT16_MAX;
static constexpr i16 EVAL_MIN = INT16_MIN;

enum Piece : u8 {
	eNone, ePawn, eKnight, eBishop, eRook, eQueen, eKing
};

enum Side : u8 {
	eWhite, eBlack, eSideNone
};

enum SpecialMove : u8 {
	wShortCastle, wLongCastle, bShortCastle, bLongCastle, enPassant
};

enum CastleMask : u8 {
	wShortCastleFlag = 0b0001,
	wLongCastleFlag = 0b0010,
	bShortCastleFlag = 0b0100,
	bLongCastleFlag = 0b1000
};

enum Direction : u8 {
	eNorth, eSouth, eEast, eWest, eNorthEast, eNorthWest, eSouthEast, eSouthWest
};

enum Square : u8 {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

inline int piece_vals[7] = { 0, 100, 320, 330, 500, 900 , 99999 };

struct Pos {
	i8 f; // 0-7
	i8 r; // 0-7
	Pos(const i8 f, const i8 r) : f(f), r(r) {};
	Pos(const u8 square) : f(square & 0x7), r(square >> 3) {};
	Pos() : f(0), r(0) {};
	bool operator==(const Pos& other) const {
		return f == other.f && r == other.r;
	}
	Pos operator+(const Pos& other) const {
		return Pos(f + other.f, r + other.r);
	}

	Pos& operator+=(const Pos& other) {
		f += other.f;
		r += other.r;
		return *this;
	}

	u8 toSquare() const {
		return (r << 3) | f; // rank * 8 + file
	}
};


struct PerfT {
	int depth = 0;           // Search depth
	u64 nodes = 0;           // Total nodes
	u64 captures = 0;        // Number of captures
	u64 en_passant = 0;      // En passant moves
	u64 castles = 0;         // Castling moves
	u64 promotions = 0;      // Promotions
	u64 checks = 0;          // Checks
	u64 discovery_checks = 0;// Discovery checks
	u64 double_checks = 0;   // Double checks
	u64 checkmates = 0;      // Checkmates 
	bool operator==(const PerfT&) const = default;

	std::string to_string() {
		return
			"depth: " + std::to_string(depth) +
			", nodes: " + std::to_string(nodes) +
			", captures: " + std::to_string(captures) +
			", en_passant: " + std::to_string(en_passant) +
			", castles: " + std::to_string(castles) +
			", promotions: " + std::to_string(promotions) +
			", checks: " + std::to_string(checks) +
			", discovery_checks: " + std::to_string(discovery_checks) +
			", double_checks: " + std::to_string(double_checks) +
			", checkmates: " + std::to_string(checkmates);
	};
};

inline std::array<float, 64> log_table = { 0, 0, 0.693147, 1.09861, 1.38629, 1.60944, 1.79176, 1.94591, 2.07944,
	2.19722, 2.30259, 2.3979, 2.48491, 2.56495, 2.63906, 2.70805, 2.77259, 2.83321, 2.89037,
	2.94444, 2.99573, 3.04452, 3.09104, 3.13549, 3.17805, 3.21888, 3.2581, 3.29584, 3.3322,
	3.3673, 3.4012, 3.43399, 3.46574, 3.49651, 3.52636, 3.55535, 3.58352, 3.61092, 3.63759,
	3.66356, 3.68888, 3.71357, 3.73767, 3.7612, 3.78419, 3.80666, 3.82864, 3.85015, 3.8712,
	3.89182, 3.91202, 3.93183, 3.95124, 3.97029, 3.98898, 4.00733, 4.02535, 4.04305, 4.06044,
	4.07754, 4.09434, 4.11087, 4.12713, 4.14313 };


