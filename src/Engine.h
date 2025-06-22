#pragma once

#include "Board.h"
#include <ctime>
#include <algorithm>
#include <unordered_map>

#include "Memory.h"
#include "Misc.h"

enum class TType : u8 {
	INVALID,
	EXACT,
	FAIL_LOW,
	BETA_CUT,
	BEST
};

struct TTEntry {
	u32 hash;
	int eval = 0;
	u8 depth_left = 0;
	u16 ply = 0;
	u8 depth_from_root = 0;
	TType type = TType::INVALID;
	Move best_move;

	[[nodiscard]] explicit constexpr operator bool() const {
		return type != TType::INVALID;
	}
};


struct TimeControl {
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movetime = 0;
};



struct SearchStack {
	Move move;
};

class Engine {
	int hash_miss;
	//Move best_move;
	static constexpr int MAX_PLY = 64;
	static constexpr u64 hash_size = 32e6 / sizeof(TTEntry);
	std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
	std::array<int, MAX_PLY> pv_length;
	std::vector<TTEntry> tt;

	// Engine state variables
	u16 max_depth = 0;
	int nodes = 0;
	Move root_best;
	Move expected_response;

	// Timer variables
	std::clock_t start_time = 0;
	int max_time = 0;
	std::vector<PerfT> perf_values;
	int pos_count = 0;


	void perftSearch(int depth);
	int alphaBeta(int alpha, int beta, int depth_left, bool is_pv);
	int quiesce(int alpha, int beta, bool is_pv);

public:
	std::array<std::array<std::array<int, 64>, 64>, 2> history_table;
	int hash_hits = 0;
	std::array<StaticVector<Move>, 64> seen_quiets;
	std::array<std::array<Move, 2>, MAX_PLY> killer_moves;
	int start_ply = 0;
	Board b;
	TimeControl tc;

	Engine() {
		tt.resize(hash_size);
		for (auto& i : history_table) {
			for (auto& j : i) {
				for (auto& k : j) {
					k = 0;
				}
			}
		}
	}

	std::array<StaticVector<Move>, 64> move_vec;


	std::vector<PerfT> doPerftSearch(int depth);
	std::vector<PerfT> doPerftSearch(std::string position, int depth);

	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);

	Move search(int depth);
	std::vector<Move> getPrincipalVariation() const;

	std::string getPV();
	void printPV(int score);

	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best);

	TTEntry probeTT(u64 hash_key) const {
		u64 index = static_cast<std::uint64_t>((static_cast<unsigned __int128>(hash_key) * static_cast<unsigned __int128>(hash_size)) >> 64);
		if (tt[index].hash == (hash_key & 0xFFFFFFFFull)) {
			return tt[index];
		}
		return TTEntry();
	}

	bool checkTime();
	void calcTime();

	void updatePV(int depth, Move move);
	void updateHistoryBonus(Move move, int depth_left);
	void updateHistoryMalus(Move move, int depth_left);
};

enum class MoveStage {
	ttMove,
	promotion,
	captures,
	killer,
	history
};

class MoveGen {
	MoveStage stage = MoveStage::ttMove;
	int killer_slot = 0;
	bool init = false;

public:
	Move getNext(Engine& e, Board& b, StaticVector<Move>& moves) {

		if (moves.empty()) {
			return Move(0, 0);
		}
		Move out;
		TTEntry entry;
		switch (stage) {
		case MoveStage::ttMove:
			entry = e.probeTT(e.b.getHash());
			if (entry && entry.type != TType::FAIL_LOW) {
				auto pos_best = std::find(moves.begin(), moves.end(), entry.best_move);
				if (pos_best != moves.end()) {
					out = *pos_best;
					e.hash_hits++;
					*pos_best = moves.back();
					moves.pop_back();

					stage = MoveStage::captures;
					return out;
				}
			}
			/*
			stage = MoveStage::promotion;
			[[fallthrough]];
		case MoveStage::promotion:
			if (moves.end() != std::find_if(moves.begin(), moves.end(),
				[](const auto& m) { return m.promotion(); })) {
				auto promo = [](const auto& a, const auto& b) { return piece_vals[a.promotion()]; };
				auto pos_best = std::ranges::max_element(moves.begin(), moves.end(), promo);
				out = *pos_best;
				*pos_best = moves.back();
				moves.pop_back();
				return out;
			}
			*/
			stage = MoveStage::captures;
			[[fallthrough]];
		case MoveStage::captures:
			if (moves.end() != std::find_if(moves.begin(), moves.end(),
				[](const auto& m) { return m.captured() || m.promotion(); })) {
				auto mvv_lva = [](const auto& a, const auto& b) {
					return piece_vals[a.promotion()] * 8 + piece_vals[a.captured()] * 8 - piece_vals[a.piece()] <
						   piece_vals[b.promotion()] * 8 + piece_vals[b.captured()] * 8 - piece_vals[b.piece()];
					};
				auto pos_best = std::ranges::max_element(moves.begin(), moves.end(), mvv_lva);
				out = *pos_best;
				*pos_best = moves.back();
				moves.pop_back();
				return out;
			}

			stage = MoveStage::killer;
			[[fallthrough]];
		case MoveStage::killer:
			if ((b.ply - e.start_ply > 2) && killer_slot < 2) {
				Move killer = e.killer_moves[b.ply - e.start_ply - 2][killer_slot++];
				auto pos_best = std::find(moves.begin(), moves.end(), killer);
				if (killer && pos_best != moves.end()) {
					out = *pos_best;
					*pos_best = moves.back();
					moves.pop_back();
					return out;
				}
			}

			stage = MoveStage::history;
			[[fallthrough]];
		case MoveStage::history:
			int max = -100000;
			int index = 0;

			for (int i = 0; i < moves.size(); i++) {
				int val = e.history_table[b.us][moves[i].from()][moves[i].to()];
				if (val > max) {
					max = val;
					index = i;
				}
			}
			out = moves[index];
			moves[index] = moves.back();
			moves.pop_back();
			return out;
			break;
		}

		return Move();
	}
};
