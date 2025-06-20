#include "Engine.h"



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
		sorted_moves.push_back({-100000, move});
	}

	max_depth = 1;
	int score = alphaBeta(-100000, 100000, max_depth, true);

	for (max_depth = 2; max_depth < MAX_PLY; max_depth++) {
		// Keep searching until we get a score within our window
		pv_length[0] = 0;
		int delta = 9 + score * score / 16384;
		int alpha = score - delta;
		int beta = score + delta;

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, true);
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

		if (checkTime()) break;
		if (pv_table[0][0]) {
			best_move = pv_table[0][0];
		}
		printPV(score);
	}
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = pv_table[0][0];
	}
	return best_move;
}


int Engine::alphaBeta(int alpha, int beta, int depth_left, bool is_pv) {
	const int search_ply = b.ply - start_ply;
	nodes++;
	if (b.isRepetition(search_ply ? 1 : 2) || b.half_move >= 100) {
		return 0;
	}

	pv_length[search_ply] = 0;
	if (checkTime()) return b.getEval();
	
	bool in_check = b.isCheck();
	if (depth_left <= 0 && in_check) { depth_left = 1; }
	if (search_ply >= MAX_PLY - 1) return b.getEval();
	if (depth_left <= 0) return quiesce(alpha, beta, is_pv);

	bool futility_prune = false;

	TTEntry tt_entry = probeTT(b.getHash());

	int eval;
	//pruning
	if (!is_pv && !in_check) {

		if (tt_entry && tt_entry.depth_left >= depth_left) {
			if (tt_entry.type == TType::EXACT) return tt_entry.eval;
			if (tt_entry.type == TType::BETA_CUT && tt_entry.eval >= beta) return tt_entry.eval;
			if (tt_entry.type == TType::FAIL_LOW && tt_entry.eval <= alpha) return tt_entry.eval;
		}

		eval = b.getEval();

		//null move pruning, do not NMP in late game
		if (depth_left >= 3 && (eval) > beta && b.getPhase() <= 16) {
			b.doMove(Move(0, 0));
			const int R = 4 + depth_left / 4 + std::min(3, (eval - beta) / 200);
			int null_score = -alphaBeta(-beta, -beta + 1, depth_left - R, false);
			b.undoMove();
			//don't return wins
			if (null_score >= beta && null_score < 30000 && null_score > -30000) return null_score;
		}

		//reverse futility pruning
		if (depth_left <= 6 && eval >= beta + 80 * depth_left) {
			return eval;
		}

		// Futility margins increasing by depth
		static constexpr int futility_margins[4] = { 0, 100, 300, 500 };
		if (depth_left <= 3) {
			int futility_margin = eval + futility_margins[depth_left];
			futility_prune = (futility_margin <= alpha);
		}
	}

	int best = -100000;
	Move best_move;

	move_vec[search_ply].clear();
	b.genPseudoLegalMoves(move_vec[search_ply]);
	b.filterToLegal(move_vec[search_ply]);

	// Check for #M
	if (move_vec[search_ply].empty() && in_check) {
		return -99999 + b.ply - start_ply;
	}
	if (move_vec[search_ply].empty()) {
		return 0;
	}

	int moves_searched = 0;
	MoveGen move_gen;
	bool raised_alpha = false;
	seen_quiets[search_ply].clear();
	while (const Move move = move_gen.getNext(*this, b, move_vec[search_ply])) {
		if (checkTime()) return best;
		int score = 0;
		if (!move.captured()) {
			seen_quiets[search_ply].emplace_back(move);
		}

		b.doMove(move);
		
		in_check = b.isCheck();
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

		//search reductions
		if (moves_searched >= (3 + !search_ply) && !in_check && depth_left >= 3) {
			int R = static_cast<int>(1.0 + log_table[depth_left] * log_table[moves_searched] / 2.5);
			//	- history_table[!b.us][move.from()][move.to()] / 9164;
			//int R = 0;
			R += !is_pv;
			R -= is_pv;
			
			//	
			/*
			if (move.captured() || move.promotion()) {
				R = int(log_table[depth_left] * log_table[moves_searched] / 3.76);
			} else {
				R = int(1.01 + log_table[depth_left] * log_table[moves_searched] / 3.0);
			}
			*/
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1 - R, false);
			if (score > alpha) {
				//research full depth if we fail high
				score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1, false);
			}
		}

		else if (!is_pv || moves_searched > 0) {
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1, false);
		}

		if (is_pv && (moves_searched == 0 || score > alpha)) {
			score = -alphaBeta(-beta, -alpha, depth_left - 1, true);

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
				b.doMove(move);
				if (!b.isRepetition(1)) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
			}
			raised_alpha = true;
		}

		if (score >= beta) {
			if (!move.captured()) {
				// Store as killer move
				if (move != killer_moves[search_ply][1]) {
					killer_moves[search_ply][1] = killer_moves[search_ply][0];
					killer_moves[search_ply][0] = move;
				}
				//int bonus = std::min(7 * depth_left * depth_left + 274 * depth_left - 182, 2048);
				//int malus = -std::min(5 * depth_left * depth_left + 283 * depth_left + 169, 1024);
				int bonus = std::min(280 * depth_left - 432, 2576);
				int malus = -std::min(343 * depth_left + 161, 1239);
				bonus = std::clamp(bonus, -16383, 16383);
				malus = std::clamp(malus, -16383, 16383);
				history_table[b.us][move.from()][move.to()] +=
					bonus - history_table[b.us][move.from()][move.to()] * abs(bonus) / 16383;
				for (auto& quiet : seen_quiets[search_ply]) {
					history_table[b.us][quiet.from()][quiet.to()] +=
						malus - history_table[b.us][quiet.from()][quiet.to()] * abs(malus) / 16383;
				}
			}

			storeTTEntry(b.getHash(), best, TType::BETA_CUT, depth_left, best_move);
			return best;
		}
		
	}
	if (raised_alpha) {
		storeTTEntry(b.getHash(), best, TType::EXACT, depth_left, best_move);
	}
	else {
		storeTTEntry(b.getHash(), best, TType::FAIL_LOW, depth_left, best_move);
	}
	return best;
}

int Engine::quiesce(int alpha, int beta, bool is_pv) {
	nodes++;
	int search_ply = b.ply - start_ply;

	if (b.isRepetition(1) || b.half_move >= 100) return 0;

	int stand_pat = b.getEval();
	int best = stand_pat;

	//delta prune
	if (stand_pat < alpha - 950) return stand_pat;

	if (stand_pat >= beta) return stand_pat;

	if (alpha < stand_pat) {
		alpha = stand_pat;
	}

	u64 hash_key = b.getHash();
	TTEntry entry = probeTT(hash_key);

	//always accept TB hits in quiescence
	if (!is_pv && entry) {
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
		}
		if (move_vec[search_ply].empty()) {
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
			storeTTEntry(b.getHash(), score, TType::BETA_CUT, 0, best_move);
			return score;
		}
		move = move_gen.getNext(*this, b, move_vec[search_ply]);
	}
	if (raised_alpha) {
		storeTTEntry(b.getHash(), best, TType::EXACT, 0, best_move);
	}
	else {
		storeTTEntry(b.getHash(), best, TType::FAIL_LOW, 0, best_move);
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
		<< " time " << (std::clock() - start_time)
		<< " nodes " << nodes
		<< " nps " << static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC))
		<< " pv ";
	
	chess::Board test_b(b.start_fen);
	for (auto m : b.state_stack) {
		test_b.makeMove(chess::uci::uciToMove(test_b, m.move.toUci()));
	}
	
	int i = 0;
	for (auto& move : pv) {
		i++;
		b.doMove(move);
		test_b.makeMove(chess::uci::uciToMove(test_b, move.toUci()));

		if (test_b.isRepetition(1) != b.isRepetition(1)) {
			std::cout << "sssss";
		}
		//std::cout << b.getHash() << std::endl;
		//b.printBoard();
		if (b.isRepetition(1) || b.half_move >= 100) {
			//std::cout << "whops";
			break;
		}

		std::cout << move.toUci() << " ";
		
	}
	for (int j = 0; j < i; j++) {
		b.undoMove();
	}
	std::cout << std::endl;
}

std::string Engine::getPV() {
	std::vector<Move> pv = getPrincipalVariation();
	if (!pv.empty()) {
		std::string out = "depth " + std::to_string(max_depth)
			+ " nodes " + std::to_string(nodes)
			+ " nps " + std::to_string(
				static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC)))
			+ " hash hits " + std::to_string(hash_hits) +
			+" pv ";

		for (auto& move : pv) {
			out += move.toUci() + " ";
		}
		return out;
	}
	return "";
}

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best) {
	u64 index = static_cast<std::uint64_t>((static_cast<unsigned __int128>(hash_key) * static_cast<unsigned __int128>(hash_size)) >> 64);
	if (tt[index].ply <= start_ply || tt[index].depth_left <= depth_left) {
		tt[index] = TTEntry{
			static_cast<u32>(hash_key & 0xFFFFFFFFull), score, depth_left, static_cast<u16>(start_ply),
			static_cast<u8>(max_depth), type, best
		};
	}
}


bool Engine::checkTime() {
	static u32 counter = 0;
	counter++;

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
	float factor = num_moves / 1000.0;

	float total_time = b.us ? tc.btime : tc.wtime;
	float inc = b.us ? tc.binc : tc.winc;

	if (total_time < inc * 2) {
		max_time = inc * 0.80;
	}
	else if (total_time < inc) {
		max_time = total_time * 0.80;
	}
	else {
		max_time = (total_time * factor) + inc * 0.80;
	}
}

void Engine::updatePV(int depth, Move move) {
	pv_table[depth][0] = move;

	for (int i = 0; i < pv_length[depth + 1]; i++) {
		pv_table[depth][i + 1] = pv_table[depth + 1][i];
	}
	pv_length[depth] = pv_length[depth + 1] + 1;
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
