#include "Engine.h"

#include <algorithm>

void Engine::perftSearch(int d) {

	if (!d) return;
	StaticVector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	if (legal_moves.size() == 0) {
		perf_values[max_depth - d].checkmates++;
		return;
	}
	for (auto& move : legal_moves) {
		int index = max_depth - d;

		if (max_depth != d) {
			pos_count++;
		}
		perf_values[index].depth = index + 1;
		perf_values[index].nodes++;
		perf_values[index].captures += static_cast<bool>(move.captured());
		perf_values[index].en_passant += move.isEnPassant();
		perf_values[index].castles += move.isCastle();
		perf_values[index].promotions += static_cast<bool>(move.promotion());

		b.doMove(move);

		perf_values[index].checks += b.isCheck();

		perftSearch(d - 1);

		b.undoMove();

		if (max_depth == d) {
			std::cout << move.toUci() << ": " << pos_count << ", ";
			pos_count = 0;
		}
	}
}
/**/
Move Engine::search(int depth) {
	
	for (auto& i : pv_table) {
		for (auto& j : i) {
			j = Move();
		}
	}

	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}

	nodes = 0;
	start_time = std::clock();
	start_ply = b.ply;

	move_vec[0].clear();
	b.genPseudoLegalMoves(move_vec[0]);
	b.filterToLegal(move_vec[0]);
	calcTime();
	Move best_move = move_vec[0].front();
	std::vector<std::pair<int, Move>> sorted_moves;
	for (auto move : move_vec[0]) {
		sorted_moves.push_back({ -100000, move });
	}

	max_depth = 1;
	int score = 0;
	int last_score = -100000;


	for (max_depth = 1; max_depth < 20; max_depth++) {


		// Keep searching until we get a score within our window
		pv_length[0] = 0;
		int delta = 50;
		int alpha = last_score - delta;
		int beta = last_score + delta;
		

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, false);
			if (checkTime()) break;
			if (score > alpha && score < beta) break;
			if (score <= alpha) {
				alpha = std::max(-100000, alpha - delta);
				delta *= 2; // Exponentially increase window size
			}
			else if (score >= beta) {
				beta = std::min(100000, beta + delta);
				delta *= 2; // Exponentially increase window size
			}
			if (alpha <= -100000 && beta >= 100000) break;
		}
		
		
		int index = 0;

		for (auto& move : sorted_moves) {

			b.doMove(move.second);
			if (index == 0) {
				score = -alphaBeta(-beta, -alpha, max_depth, true);
			}
			else {
				score = -alphaBeta(-alpha - 1, -alpha, max_depth, false);
				if (alpha < score && score < beta) {
					score = -alphaBeta(-beta, -alpha, max_depth, true);
				}
			}
			b.undoMove();
			if (checkTime()) break;

			move.first = score;


			if (score > alpha) {
				alpha = score;
				best_move = move.second;
				move.first = score;

				b.doMove(best_move);
				if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}
		
		std::sort(sorted_moves.begin(), sorted_moves.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		last_score = sorted_moves[0].first;
		if (checkTime()) break;
		printPV(alpha);

	}
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = pv_table[0][0];
	}
	return best_move;
}
/*
Move Engine::search(int depth) {
	for (auto& i : history_table) {
		for (auto& j : i) {
			for (auto& k : j) {
				k = 0;
			}
		}
	}
	for (auto& i : pv_table) {
		for (auto& j : i) {
			j = Move();
		}
	}

	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}
	pv_moves.clear();
	nodes = 0;
	start_time = std::clock();
	start_ply = b.ply;



	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);
	sortMoves(legal_moves);
	calcTime();
	Move best_move = legal_moves.front();
	std::vector<std::pair<int, Move>> sorted_moves;
	for (auto move : legal_moves) {
		sorted_moves.push_back({ -100000, move });
	}

	max_depth = 1;
	//int score = alphaBeta(-100000, 100000, max_depth, false);
	int score = -100000;

	int delta = 50;
	for (max_depth = 1 ; max_depth < 32; max_depth++) {
		//pv_length[0] = 0;
		// Start with narrow aspiration window
		int alpha = -100000;
		int beta = 100000;

		if (max_depth >= 3) {
			alpha = std::max(-100000, score - delta);
			beta = std::min(100000, score + delta);
		}

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, true);
			if (checkTime()) break;
			if (score > alpha && score < beta) break;
			if (score <= alpha) {
				alpha = std::max(-100000, alpha - delta);
				delta *= 2; // Exponentially increase window size
			}
			if (score >= beta) {
				beta = std::min(100000, beta + delta);
				delta *= 2; // Exponentially increase window size
			}
			if (alpha <= -100000 && beta >= 100000) break;
		}


		int index = 0;


		//score = alphaBeta(alpha, beta, max_depth, true);

		index++;

		/*
		for (auto& move : sorted_moves) {

			b.doMove(move.second);
			if (index == 0) {
				score = -alphaBeta(-beta, -alpha, max_depth, true);
			}
			else {
				score = -alphaBeta(-alpha - 1, -alpha, max_depth, false);
				if (alpha < score && score < beta) {
					score = -alphaBeta(-beta, -alpha, max_depth, true);
				}
			}
			b.undoMove();
			if (checkTime()) break;

			move.first = score;


			if (score > alpha) {
				alpha = score;
				best_move = move.second;
				move.first = score;

				b.doMove(best_move);
				if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}

		//std::sort(sorted_moves.begin(), sorted_moves.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		if (checkTime()) break;
		printPV(alpha);

	}
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = pv_table[0][0];
	}
	return best_move;
}
*/


int Engine::alphaBeta(int alpha, int beta, int depth_left, bool is_pv) {
	nodes++;
	const int search_ply = b.ply - start_ply;
	pv_length[search_ply] = 0;

	if (b.is3fold() || b.half_move == 100) return 0;
	bool in_check = b.isCheck();
	if (depth_left == 0 && in_check) { depth_left++; }
	if (depth_left <= 0) return quiesce(alpha, beta, is_pv);

	u64 hash_key = b.getHash();
	TTEntry entry = probeTT(hash_key);

	if (!is_pv && entry && entry.depth_from_root >= max_depth && entry.depth_left >= depth_left && entry.hash == b.getHash()) {
		if (entry.type == TType::EXACT) return entry.eval;
		if (entry.type == TType::BETA_CUT && entry.eval >= beta) return entry.eval;
		if (entry.type == TType::FAIL_LOW && entry.eval <= alpha) return entry.eval;
	}

	int best = -100000;
	Move best_move;
	


	if (search_ply >= MAX_PLY - 1) return b.getEval();

	//null move pruning
	if (!is_pv && depth_left >= 3 && !in_check && (b.getEval() + 50) > beta) {
		b.doMove(Move(0, 0));
		const int R = 2 + (depth_left / 6);
		int null_score = -alphaBeta(-beta, -beta + 1, depth_left - 1 - R, false);
		b.undoMove();
		if (null_score >= beta) return beta;
	}

	// Futility margins increasing by depth
	const static int futility_margins[4] = { 0, 100, 300, 500 };
	bool futility_prune = false;

	if (!in_check && depth_left <= 3 && !is_pv) {
		int futility_margin = b.getEval() + futility_margins[depth_left];
		futility_prune = (futility_margin <= alpha);
	}

	move_vec[search_ply].clear();
	b.genPseudoLegalMoves(move_vec[search_ply]);
	b.filterToLegal(move_vec[search_ply]);

	// Check for #M
	if (move_vec[search_ply].empty() && in_check) {
		return -99999 + b.ply - start_ply;
	} else if (move_vec[search_ply].empty()) {
		return 0;
	}


	
	TType tt_flag = TType::FAIL_LOW;
	int moves_searched = 0;
	search_calls++;
	MoveGen move_gen;
	bool raised_alpha = false;
	seen_quiets[search_ply].clear();
	while (const Move move = move_gen.getNext(*this, b, move_vec[search_ply])) {
		moves_inspected++;
		if (checkTime()) return best;
		int score = 0;
		bool can_reduce =
			moves_searched >= 3 &&
			!move.captured() &&
			!move.promotion() &&
			!in_check &&
			!is_pv &&
			depth_left >= 3;

		b.doMove(move);
		//futility pruning

		if (futility_prune &&
			!move.captured() &&
			!move.promotion() &&
			!move.isEnPassant() &&
			!is_pv &&
			!in_check) {
			b.undoMove();

			moves_searched++;
			continue;
		}

		if (can_reduce) {
			int R = int(0.5 + std::log(depth_left) * std::log(moves_searched) / 3.0);
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1 - R, false);
		}

		else if (moves_searched == 0) {
			score = -alphaBeta(-beta, -alpha, depth_left - 1, is_pv);
		}
		else {
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1, false);
			if (alpha < score && score < beta) {
				score = -alphaBeta(-beta, -alpha, depth_left - 1, true);
			}
		}
		
		b.undoMove();

		moves_searched++;
		if (score > best) {
			best = score;
			best_move = move;
		}

		if (score > alpha) {
			alpha = score;
			if (is_pv) {
				b.doMove(best_move);
				if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
			}
			raised_alpha = true;
		}

		if (score >= beta) {
			if (!move.captured()) {
				// Store as killer move
				killer_moves[search_ply][1] = killer_moves[search_ply][0];
				killer_moves[search_ply][0] = move;
				int bonus = std::min(7 * depth_left * depth_left + 274 * depth_left - 182, 2048);
				int malus = -std::min(5 * depth_left * depth_left + 283 * depth_left + 169, 1024);
				bonus = std::clamp((int)bonus, int(-16383), int(16383));
				malus = std::clamp((int)malus, int(-16383), int(16383));
				history_table[b.us][move.from()][move.to()] +=
					bonus - history_table[b.us][move.from()][move.to()] * abs(bonus) / 16383;
				for (auto& quiet : seen_quiets[search_ply]) {
					history_table[b.us][quiet.from()][quiet.to()] += 
						malus - history_table[b.us][quiet.from()][quiet.to()] * abs(malus) / 16383;
				}
			}

			storeTTEntry(b.getHash(), beta, TType::BETA_CUT, depth_left, best_move);
			return beta;
		}
		if (!move.captured()) {
			seen_quiets[search_ply].emplace_back(move);
		}
	}
	if (raised_alpha) {
		storeTTEntry(b.getHash(), best, TType::EXACT, depth_left, best_move);
	} else {
		storeTTEntry(b.getHash(), best, TType::FAIL_LOW, depth_left, best_move);
	}
	return best;
}


std::vector<Move> Engine::getPrincipalVariation() const {
	std::vector<Move> pv;
	for (int i = 0; i < pv_length[0]; i++) {
		pv.push_back(pv_table[0][i]);
	}
	return pv;
}

void Engine::printPV(int score) {
	std::vector<Move> pv = getPrincipalVariation();
	std::cout << "info score cp " << score << " depth " << max_depth
		<< " nodes " << nodes
		<< " nps " << static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC))
	    << " pv ";

	for (auto& move : pv) {
		std::cout << move.toUci() << " ";
	}
	std::cout << std::endl;
}

std::string Engine::getPV() {

	std::vector<Move> pv = getPrincipalVariation();
	if (!pv.empty()) {
		std::string out = "depth " + std::to_string(max_depth)
			+ " nodes " + std::to_string(nodes)
			+ " nps " + std::to_string(static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC)))
			+ " hash hits " + std::to_string(hash_hits) +
			+ " pv ";

		for (auto& move : pv) {
			out += move.toUci() + " ";
		}
		return out;
	}
	return "";
}

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best) {
	u64 index = __mulh(hash_key & 0x7FFFFFFFFFFFFFFF, hash_size);
	if (tt[index].ply <= start_ply || tt[index].depth_left <= depth_left) { //replace
		tt[index] = TTEntry{ hash_key, score, u8(depth_left), u16(start_ply), u8(max_depth), type, best };

	}
}


bool Engine::checkTime() {
	if ((std::clock() - start_time) > max_time) return true;
	return false;
}

void Engine::calcTime() {
	if (tc.movetime) {
		max_time = tc.movetime;
		return;
	}
	int search_ply = b.ply - start_ply;

	float num_moves = move_vec[search_ply].size();
	float factor = num_moves / 500.0;

	float total_time = b.us ? tc.btime : tc.wtime;
	float inc = b.us ? tc.binc : tc.winc;

	if (total_time < inc) {
		max_time = inc * 0.95;
	}
	else {
		max_time = (total_time * factor) + inc * 0.95;
	}
	//max_time = inc;
}

void Engine::updatePV(int depth, Move move) {
	pv_table[depth][0] = move;

	for (int i = 0; i < pv_length[depth + 1]; i++) {
		pv_table[depth][i + 1] = pv_table[depth + 1][i];
	}
	pv_length[depth] = pv_length[depth + 1] + 1;
}

int Engine::quiesce(int alpha, int beta, bool is_pv) {
	nodes++;
	int search_ply = b.ply - start_ply;

	if (b.is3fold()) return 0;

	int stand_pat = b.getEval();
	int best = stand_pat;

	//delta prune
	if (stand_pat < alpha - 950) return alpha;

	if (stand_pat >= beta) return beta;

	if (alpha < stand_pat) {
		alpha = stand_pat;
	}

	u64 hash_key = b.getHash();
	TTEntry entry = probeTT(hash_key);

	if (!is_pv && entry && entry.depth_from_root >= max_depth && entry.hash == b.getHash()) {
		if (entry.type == TType::EXACT) return entry.eval;
		if (entry.type == TType::BETA_CUT && entry.eval >= beta) return entry.eval;
		if (entry.type == TType::FAIL_LOW && entry.eval <= alpha) return entry.eval;
	}


	move_vec[search_ply].clear();
	b.genPseudoLegalCaptures(move_vec[search_ply]);
	b.filterToLegal(move_vec[search_ply]);

	// Check for #M
	if (!move_vec[search_ply].size()) {
		b.genPseudoLegalMoves(move_vec[search_ply]);
		b.filterToLegal(move_vec[search_ply]);
		if (move_vec[search_ply].empty() && b.isCheck()) {
			return -99999 + b.ply - start_ply;
		} else if (move_vec[search_ply].empty()) {
			return 0;
		}
		return stand_pat;
	}
	bool raised_alpha = false;
	Move best_move;
	MoveGen move_gen;
	Move move = move_gen.getNext(*this, b, move_vec[search_ply]);
	while (move.raw()) {
		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);
		if (checkTime()) return best;

		b.doMove(move);
		int score = -quiesce(-beta, -alpha, is_pv);
		b.undoMove();

		if (score > best) {
			best_move = move;
			best = score;
		}
		if (score > alpha) {
			alpha = score;
			raised_alpha = true;
		}
		if (score >= beta) {
			best_move = move;
			storeTTEntry(b.getHash(), beta, TType::BETA_CUT, 0, best_move);
			return beta;
		}
		move = move_gen.getNext(*this, b, move_vec[search_ply]);
	}
	if (raised_alpha) {
		storeTTEntry(b.getHash(), best, TType::EXACT, 0, best_move);
	} else {
		storeTTEntry(b.getHash(), best, TType::FAIL_LOW, 0, best_move);
	}
	return best;
}

std::vector<PerfT> Engine::doPerftSearch(int depth) {
	perf_values.clear();
	perf_values.resize(depth);
	start_time = std::clock();
	max_depth = depth;
	perftSearch(depth);
	// Returns elapsed time in milliseconds
	std::cout << "search time: " << static_cast<int>(1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC) << "ms\n\n";

	return perf_values;
}

void Engine::setBoardFEN(std::istringstream& fen) {
	b.loadFen(fen);
}

void Engine::setBoardUCI(std::istringstream& uci) {
	b.loadUci(uci);
}

std::vector<PerfT> Engine::doPerftSearch(std::string position, int depth) {
	std::istringstream iss(position);
	b.loadFen(iss);
	return doPerftSearch(depth);
}
