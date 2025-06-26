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
	u32 hash = 0;
	int eval = 0;
	u8 depth_left = 0;
	u16 ply = 0;
	u8 depth_from_root = 0;
	TType type = TType::INVALID;
	Move best_move = Move();

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
struct UciOptions {
	u64 hash_size = 64;
	bool debug = false;
	bool uci = false;
};


struct SearchStack {
	int static_eval = 0;
	int improving_rate = 0;
	bool improving = false;
	bool in_check = false;
	Move current_move = Move(0, 0);
	StaticVector<Move> moves;
	StaticVector<Move> seen_quiets;
	StaticVector<Move> seen_noisies;
	std::array<Move, 2> killers;
	void clear() {
		moves.clear();
		seen_quiets.clear();
		seen_noisies.clear();
		killers[0] = Move();
		killers[1] = Move();
		static_eval = 0;
		improving_rate = 0;
		improving = false;
		in_check = false;
		current_move = Move(0, 0);
	}
};

class Engine {
	static constexpr int MAX_PLY = 128;
	SearchStack search_stack[MAX_PLY];
	UciOptions uci_options;

	int hash_count = 0;
	int hash_miss = 0;
	//Move best_move;
	
	std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
	std::array<int, MAX_PLY> pv_length;
	std::vector<TTEntry> tt;

	// Engine state variables
	int max_depth = 0;
	int sel_depth = 0;
	int nodes = 0;
	Move root_best;
	Move expected_response;

	// Timer variables
	std::clock_t start_time = 0;
	int max_time = 0;
	std::vector<PerfT> perf_values;
	int pos_count = 0;


	void perftSearch(int depth);
	int alphaBeta(int alpha, int beta, int depth_left, bool is_pv, SearchStack* ss);
	int quiesce(int alpha, int beta, bool is_pv, SearchStack* ss);

public:
	
	std::array<std::array<std::array<int, 64>, 64>, 2> history_table;
	std::array<std::array < std::array<std::array<int, 64>, 7>, 7>, 2> capture_history;

	int hash_hits = 0;
	int start_ply = 0;
	Board b;
	TimeControl tc;

	Engine(UciOptions options) {
		uci_options = options;
		tt.resize((uci_options.hash_size * 1024 * 1024) / sizeof(TTEntry));
		
		for (auto& i : history_table) {
			for (auto& j : i) {
				for (auto& k : j) {
					k = 0;
				}
			}
		}
		for (auto& i : capture_history) {
			for (auto& j : i) {
				for (auto& k : j) {
					for (auto& l : k) {
						l = 0;
					}
				}
			}
		}
		for (auto& i : pv_table) {
			for (auto& j : i) {
				j = Move();
			}
		}
	}

	std::vector<PerfT> doPerftSearch(int depth);
	std::vector<PerfT> doPerftSearch(std::string position, int depth);

	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);

	void initSearch();
	Move search(int depth);
	std::vector<Move> getPrincipalVariation() const;

	void printPV(int score);

	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best);
	TTEntry probeTT(u64 hash_key) const;

	bool time_over = false;
	bool checkTime(bool strict);
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

public:
	Move getNext(Engine& e, Board& b, SearchStack* ss) {

		if (ss->moves.empty()) {
			return Move(0, 0);
		}
		Move out;
		TTEntry entry;
		switch (stage) {
		case MoveStage::ttMove:
			entry = e.probeTT(e.b.getHash());
			if (entry && entry.type != TType::FAIL_LOW) {
				auto pos_best = std::find(ss->moves.begin(), ss->moves.end(), entry.best_move);
				if (pos_best != ss->moves.end()) {
					out = *pos_best;
					e.hash_hits++;
					*pos_best = ss->moves.back();
					ss->moves.pop_back();

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
		case MoveStage::captures: {
			int max = -100000;
			int index = 0;
			
			for (int i = 0; i < ss->moves.size(); i++) {
				if (ss->moves[i].captured() || ss->moves[i].promotion()) {
					//piece_vals[moves[i].promotion()] * 256 +
					Move m = ss->moves[i];
					int val = piece_vals[m.promotion()] + (m.captured() ? piece_vals[m.captured()] * 8 +
						e.capture_history[b.us][m.piece()][m.captured()][m.to()] : 0);
					if (val > max) {
						max = val;
						index = i;
					}
				}

			}
			if (max != -100000) {
				out = ss->moves[index];
				ss->moves[index] = ss->moves.back();
				ss->moves.pop_back();
				return out;

			}
		}

			stage = MoveStage::killer;
			[[fallthrough]];
		case MoveStage::killer:
			if ((b.ply - e.start_ply > 2) && killer_slot < 2) {
				Move killer = (ss - 2)->killers[killer_slot++];
				auto pos_best = std::find(ss->moves.begin(), ss->moves.end(), killer);
				if (killer && pos_best != ss->moves.end()) {
					out = *pos_best;
					*pos_best = ss->moves.back();
					ss->moves.pop_back();
					return out;
				}
			}

			stage = MoveStage::history;
			[[fallthrough]];
		case MoveStage::history: {
			int max = -100000;
			int index = 0;

			for (int i = 0; i < ss->moves.size(); i++) {
				int val = e.history_table[b.us][ss->moves[i].from()][ss->moves[i].to()];
				if (val > max) {
					max = val;
					index = i;
				}
			}
			out = ss->moves[index];
			ss->moves[index] = ss->moves.back();
			ss->moves.pop_back();
			return out;
			break;
			}
		}
		return Move();
	}
};
