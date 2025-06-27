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

inline int see_piece_vals[7] = { 0, 100, 320, 330, 500, 900 , 99999 };

struct Pos {
	i8 f = 0; // 0-7
	i8 r = 0; // 0-7
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

inline std::array<float, 256> log_table = {
	0, 0, 0.693147, 1.09861, 1.38629, 1.60944, 1.79176, 1.94591, 2.07944, 2.19722, 2.30259, 2.3979, 2.48491, 2.56495,
	2.63906, 2.70805, 2.77259, 2.83321, 2.89037, 2.94444, 2.99573, 3.04452, 3.09104, 3.13549, 3.17805, 3.21888, 3.2581,
	3.29584, 3.3322, 3.3673, 3.4012, 3.43399, 3.46574, 3.49651, 3.52636, 3.55535, 3.58352, 3.61092, 3.63759, 3.66356,
	3.68888, 3.71357, 3.73767, 3.7612, 3.78419, 3.80666, 3.82864, 3.85015, 3.8712, 3.89182, 3.91202, 3.93183, 3.95124,
	3.97029, 3.98898, 4.00733, 4.02535, 4.04305, 4.06044, 4.07754, 4.09434, 4.11087, 4.12713, 4.14313, 4.15888, 4.17439,
	4.18965, 4.20469, 4.21951, 4.23411, 4.2485, 4.26268, 4.27667, 4.29046, 4.30407, 4.31749, 4.33073, 4.34381, 4.35671,
	4.36945, 4.38203, 4.39445, 4.40672, 4.41884, 4.43082, 4.44265, 4.45435, 4.46591, 4.47734, 4.48864, 4.49981, 4.51086,
	4.52179, 4.5326, 4.54329, 4.55388, 4.56435, 4.57471, 4.58497, 4.59512, 4.60517, 4.61512, 4.62497, 4.63473, 4.64439,
	4.65396, 4.66344, 4.67283, 4.68213, 4.69135, 4.70048, 4.70953, 4.7185, 4.72739, 4.7362, 4.74493, 4.75359, 4.76217,
	4.77068, 4.77912, 4.78749, 4.79579, 4.80402, 4.81218, 4.82028, 4.82831, 4.83628, 4.84419, 4.85203, 4.85981, 4.86753,
	4.8752, 4.8828, 4.89035, 4.89784, 4.90527, 4.91265, 4.91998, 4.92725, 4.93447, 4.94164, 4.94876, 4.95583, 4.96284,
	4.96981, 4.97673, 4.98361, 4.99043, 4.99721, 5.00395, 5.01064, 5.01728, 5.02388, 5.03044, 5.03695, 5.04343, 5.04986,
	5.05625, 5.0626, 5.0689, 5.07517, 5.0814, 5.0876, 5.09375, 5.09987, 5.10595, 5.11199, 5.11799, 5.12396, 5.1299, 5.1358,
	5.14166, 5.14749, 5.15329, 5.15906, 5.16479, 5.17048, 5.17615, 5.18178, 5.18739, 5.19296, 5.1985, 5.20401, 5.20949,
	5.21494, 5.22036, 5.22575, 5.23111, 5.23644, 5.24175, 5.24702, 5.25227, 5.2575, 5.26269, 5.26786, 5.273, 5.27811, 5.2832,
	5.28827, 5.2933, 5.29832, 5.3033, 5.30827, 5.31321, 5.31812, 5.32301, 5.32788, 5.33272, 5.33754, 5.34233, 5.34711, 5.35186,
	5.35659, 5.36129, 5.36598, 5.37064, 5.37528, 5.3799, 5.3845, 5.38907, 5.39363, 5.39816, 5.40268, 5.40717, 5.41165, 5.4161,
	5.42053, 5.42495, 5.42935, 5.43372, 5.43808, 5.44242, 5.44674, 5.45104, 5.45532, 5.45959, 5.46383, 5.46806, 5.47227, 5.47646,
	5.48064, 5.4848, 5.48894, 5.49306, 5.49717, 5.50126, 5.50533, 5.50939, 5.51343, 5.51745, 5.52146, 5.52545, 5.52943, 5.53339, 5.53733, 5.54126 };


