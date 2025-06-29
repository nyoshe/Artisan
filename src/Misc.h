#pragma once
#include <cstdint>
#include <string>
#include <cstddef>
#include <array>
#include <vector>

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



static std::vector<std::string> bench_fens = { "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
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
