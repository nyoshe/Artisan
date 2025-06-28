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

void Engine::initSearch() {
	for (auto& i : pv_table) {
		for (auto& j : i) {
			j = Move();
		}
	}

	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}

	for (auto& i : search_stack) {
		i = SearchStack();
	}

	sel_depth = 0;
	nodes = 0;
	start_time = std::clock();
	start_ply = b.ply;
	hash_count = 0;
}

Move Engine::search(int depth) {
	
	initSearch();
	if (!uci_options.uci) {
		b.printBoard();
		std::cout << "    depth   score      time      nodes          nps     hash   pv\n"
			         "-----------------------------------------------------------------\n";
	}
	b.genPseudoLegalMoves(search_stack->moves);
	b.filterToLegal(search_stack->moves);
	
	calcTime();

	max_depth = 2;
	int score = alphaBeta(-100000, 100000, max_depth, false, search_stack);

	if (search_stack->moves.size() == 1) {
		printPV(score);
		return search_stack->moves[0];
	}

	Move best_move = pv_table[0][0];
	if (!best_move) best_move = search_stack->moves.front();
	bool add_time = false;

	for (max_depth = 2; max_depth < MAX_PLY; max_depth++) {
		// Keep searching until we get a score within our window
		search_stack->clear();
		pv_length[0] = 0;
		int delta = 9 + score * score / 16384;
		int alpha = score - delta;
		int beta = score + delta;

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, true, search_stack);
			if (checkTime(true)) break;
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

		if (checkTime(true)) {
			//allocate extra time if we just had a best move change
			
			if (best_move != pv_table[0][0] && !add_time) {
				float total_time = b.us ? tc.btime : tc.wtime;
				float inc = b.us ? tc.binc : tc.winc;
				if (max_time * 3.0 < total_time - inc - 10) {
					max_time = max_time * 3.0;
					add_time = true;
					continue;
				}
			} 
			return best_move;
		} else if ((std::clock() - start_time) > max_time / 2.0) {
			if (pv_table[0][0]) {
				best_move = pv_table[0][0];
				expected_response = pv_table[0][1];
				printPV(score);
			}
			break;
		}

		
		if (pv_table[0][0]) {
			best_move = pv_table[0][0];
			expected_response = pv_table[0][1];
		}

		printPV(score);
	}

	return best_move;
}


int Engine::alphaBeta(int alpha, int beta, int depth_left, bool cut_node, SearchStack* ss) {
	ss->clear();
	const int search_ply = b.ply - start_ply;

	sel_depth = std::max(search_ply, sel_depth);
	const bool is_pv = alpha != beta - 1;
	if (is_pv) pv_length[search_ply] = 0;
	bool is_root = search_ply == 0;
	(ss + 1)->killers[0] = Move(0, 0);
	(ss + 1)->killers[1] = Move(0, 0);

	nodes++;
	if (b.isRepetition(is_root ? 2 : 1) || b.half_move >= 100) {
		return 0;
	}

	
	if (checkTime(false)) return b.getEval();

	ss->in_check = b.isCheck();
	
	if (depth_left <= 0 && ss->in_check) { depth_left = 1; }
	if (search_ply >= MAX_PLY - 1) return b.getEval();
	if (depth_left <= 0) return quiesce(alpha, beta, is_pv, ss + 1);

	bool futility_prune = false;
	

	TTEntry tt_entry = probeTT(b.getHash());

	ss->static_eval = b.getEval();

	if (!ss->in_check) {
		SearchStack* past_stack = nullptr;
		if (search_ply > 2 && (ss - 2)->static_eval != 0 && !(ss - 2)->in_check) {
			past_stack = (ss - 2);
		}
		else if (search_ply > 4 && (ss - 4)->static_eval != 0 && !(ss - 4)->in_check ) {
			past_stack = (ss - 4);
		}
		if (past_stack != nullptr) {
			ss->improving_rate = std::clamp(past_stack->improving_rate + (ss->static_eval - past_stack->static_eval) / 50, -100, 100);
			ss->improving = (ss->static_eval - past_stack->static_eval) > 0;
		}
	}

	//pruning
	if (!is_pv && !ss->in_check && !is_root) {

		if (tt_entry && tt_entry.depth_left >= depth_left) {
			if (tt_entry.type == TType::EXACT) return tt_entry.eval;
			if (tt_entry.type == TType::BETA_CUT && tt_entry.eval >= beta) return tt_entry.eval;
			if (tt_entry.type == TType::FAIL_LOW && tt_entry.eval <= alpha) return tt_entry.eval;
		}
		
		
		//null move pruning, do not NMP in late game
		if (depth_left >= 2 && ss->static_eval > beta && (ss - 1)->current_move != Move(0,0)) {
			b.doMove(Move(0, 0));
			const int R = 4 + depth_left / 4 + std::min(3, (ss->static_eval - beta) / 200);
			int null_score = -alphaBeta(-beta, -beta + 1, depth_left - R, !cut_node, ss + 1);
			b.undoMove();
			//don't return wins
			if (null_score >= beta && null_score < 30000 && null_score > -30000) return null_score;
		}
		
		//reverse futility pruning
		if (depth_left <= 6 && ss->static_eval >= beta + 80 * depth_left - depth_left * ss->improving_rate) {
			return ss->static_eval;
		}

		// Futility margins increasing by depth
		static constexpr int futility_margins[4] = { 0, 100, 300, 500 };
		if (depth_left <= 3) {
			int futility_margin = ss->static_eval + futility_margins[depth_left];
			futility_prune = (futility_margin <= alpha);
		}
	}

	int best = -100000;
	Move best_move;

	b.genPseudoLegalMoves(ss->moves);
	b.filterToLegal(ss->moves);

	// Check for #M
	if (ss->moves.empty() && ss->in_check) {
		return -99999 + b.ply - start_ply;
	}
	if (ss->moves.empty()) {
		return 0;
	}

	int moves_searched = 0;
	MoveGen move_gen;
	bool raised_alpha = false;
	while (const Move move = move_gen.getNext(*this, b, ss)) {
		if (checkTime(false)) return best;
		moves_searched++;
		int score = 0;

		bool is_quiet = !(move.captured() || move.promotion());
		if (is_quiet) {
			ss->seen_quiets.emplace_back(move);
		} else {
			ss->seen_noisies.emplace_back(move);
		}
		

		b.doMove(move);
		ss->current_move = move;

		bool move_is_check = b.isCheck();

		if (is_quiet && !move_is_check && !is_pv && !is_root) {
			//futility pruning, LMP
			if (futility_prune) {
				b.undoMove();
				continue;
			}
			
			if (ss->seen_quiets.size() > (1.0 + (depth_left * depth_left)) && depth_left <= 4) {
				b.undoMove();
				continue;
			}
			
			//history pruning
			
			if (raised_alpha && moves_searched > 4 && depth_left < 4 && history_table[!b.us][move.from()][move.to()] < -1024 * depth_left) {
				b.undoMove();
				break;
			}
		}
		

		//search reductions
		if (moves_searched > 2 + 2 * is_pv + ss->improving  && depth_left > 2 && !is_root) {
			int R = 0;
			
			if (!is_quiet) {
				//R = static_cast<int>(0.5 + log_table[depth_left] * log_table[moves_searched] / 3.5);
				R = 3 - capture_history[b.us][move.piece()][move.captured()][move.to()] / 5000;
				R -= move_is_check;
				//R += !is_pv;
			} else {
				R = static_cast<int>(2.0 + log_table[depth_left] * log_table[moves_searched] / 2.5);
				
				//R += move_is_check && move.piece() == eKing;
				//R += move_gen.stage == MoveStage::killer;
				//R -= history_table[!b.us][move.from()][move.to()] / 10000;
				
				R += cut_node;
				R += !ss->improving;
			}
			
			R += !is_pv;
			//R -= move_is_check;
			//R -= history_table[!b.us][move.from()][move.to()] / 8870;

			//R += tt_entry ? (tt_entry.best_move.captured() || tt_entry.best_move.promotion()) : 0;


			int lmr_depth = std::clamp(depth_left - R, 1, depth_left);
			score = -alphaBeta(-alpha - 1, -alpha, lmr_depth, true, ss + 1);
			if (score > alpha) {
				
				int new_depth = depth_left - 1;
				//history_table[!b.us][move.from()][move.to()];
				//new_depth += score > best + 50;
				//new_depth -= score < (best + depth_left);
				//new_depth = std::min(new_depth, depth_left);
				//research full depth if we fail high
				
				score = -alphaBeta(-alpha - 1, -alpha, new_depth,  !cut_node, ss + 1);
			}
		}

		else if (!is_pv || moves_searched > 1) {
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1, !cut_node, ss + 1);
		}

		if (is_pv && (moves_searched == 1 || score > alpha)) {
			score = -alphaBeta(-beta, -alpha, depth_left - 1, false, ss + 1);
		}
		
		b.undoMove();


		if (score > best) {
			best = score;
			best_move = move;
		}

		if (score > alpha) {
			alpha = score;
			if (is_pv) {
				updatePV(b.ply - start_ply, best_move);
			}
			raised_alpha = true;
		}

		if (score >= beta) {
			if (is_quiet) {
				// Store as killer move
				if (move != ss->killers[1]) {
					ss->killers[1] = ss->killers[0];
					ss->killers[0] = move;
				}
				updateHistoryBonus(move, depth_left);
				for (auto& quiet : ss->seen_quiets) {
					updateHistoryMalus(quiet, depth_left);
				}

			} else {
				//int bonus = std::min(7 * depth_left * depth_left + 274 * depth_left - 182, 2048);
				int bonus = std::min(280 * depth_left - 432, 2576);
				bonus = std::clamp(bonus, -16384, 16384);
				capture_history[b.us][move.piece()][move.captured()][move.to()] +=
					bonus - capture_history[b.us][move.piece()][move.captured()][move.to()] * abs(bonus) / 16384;
			}
			//int malus = -std::min(5 * depth_left * depth_left + 283 * depth_left + 169, 1024);
			int malus = -std::min(343 * depth_left - 161, 1239);
			for (auto& capture : ss->seen_noisies) {
				malus = std::clamp(malus, -16384, 16384);
				capture_history[b.us][capture.piece()][capture.captured()][capture.to()] +=
					malus - capture_history[b.us][capture.piece()][capture.captured()][capture.to()] * abs(malus) / 16384;
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

int Engine::quiesce(int alpha, int beta, bool is_pv, SearchStack* ss) {
	ss->clear();
	nodes++;
	int search_ply = b.ply - start_ply;
	sel_depth = std::max(search_ply, sel_depth);
	if (search_ply >= MAX_PLY - 1) return b.getEval();
	if (b.isRepetition(1) || b.half_move >= 100) return 0;

	ss->static_eval = b.getEval();
	int stand_pat = ss->static_eval;
	int best = ss->static_eval;

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

	b.genPseudoLegalCaptures(ss->moves);
	b.filterToLegal(ss->moves);

	// Check for #M
	if (ss->moves.empty()) {
		ss->moves.clear();
		b.genPseudoLegalMoves(ss->moves);
		b.filterToLegal(ss->moves);
		if (ss->moves.empty() && b.isCheck()) {
			return -99999 + b.ply - start_ply;
		}
		if (ss->moves.empty()) {
			return 0;
		}
		return stand_pat;
	}

	bool raised_alpha = false;
	Move best_move;
	MoveGen move_gen;
	Move move = move_gen.getNext(*this, b, ss);
	while (move.raw()) {
		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);
		if (checkTime(false)) return best;

		b.doMove(move);
		int score = -quiesce(-beta, -alpha, is_pv, ss + 1);
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
		move = move_gen.getNext(*this, b, ss);
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

		
	//output UCI string
	if (uci_options.uci) {
		std::cout
			<< "info depth " << max_depth
			<< " seldepth " << sel_depth
			<< " score cp " << score
			<< " time " << (std::clock() - start_time)
			<< " nodes " << nodes
			<< " nps " << static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC))
			<< " hashfull " << ((1000 * hash_count) / tt.size())
			<< " pv ";
	} else {
		
		std::cout
			<< "\x1b[0m"
			<< std::setw(6)
			<< max_depth << "/" << 
			std::left << std::setw(4) << sel_depth
			<< (score == 0 ? "\x1b[38;5;226m" : (score > 0 ? "\x1b[38;5;40m" : "\x1b[38;5;160m"))

			<< std::setw(6) << std::right << std::setprecision(2) << std::fixed << static_cast<float>(score) / 100.0 << "\x1b[0m"
			<< std::setw(9) << std::right << std::setprecision(3) << static_cast<float>(std::clock() - start_time) / CLOCKS_PER_SEC << "s"
			<< std::setw(10) << std::setprecision(3) <<  nodes / 1e6 << "m"
			<< std::setw(9) << std::setprecision(2) << (static_cast<float>(nodes) / ((1000000.0 / CLOCKS_PER_SEC) * static_cast<float>(std::clock() - start_time))) << "mn/s"
			<< std::setw(8) << std::setprecision(2) << static_cast<float>(hash_count * 100.0 / static_cast<float>(tt.size())) << "%"
			<< "   ";
	}
	chess::Board test_b;
	if (!b.start_fen.empty()) {
		test_b.setFen(b.start_fen);
	}
	else {

	}

	for (auto m : b.state_stack) {
		test_b.makeMove(chess::uci::uciToMove(test_b, m.move.toUci()));
	}

	int i = 0;
	for (auto& move : pv) {
		i++;
		b.doMove(move);
		test_b.makeMove(chess::uci::uciToMove(test_b, move.toUci()));
		if (test_b.isRepetition(2)) {
			break;
		}

		if (b.isRepetition(2) || b.half_move >= 100) {
			break;
		}

		std::cout << move.toUci() << " ";

	}
	for (int j = 0; j < i; j++) {
		b.undoMove();
	}


	std::cout << std::endl;
}

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best) {
	u64 index = static_cast<std::uint64_t>((static_cast<unsigned __int128>(hash_key) *
				static_cast<unsigned __int128>(tt.size())) >> 64);
	if (!tt[index]) {
		hash_count++;
	}
	if (tt[index].ply <= start_ply || tt[index].depth_left <= depth_left) {
		tt[index] = TTEntry{
			static_cast<u32>(hash_key & 0xFFFFFFFFull), score, depth_left, static_cast<u16>(start_ply),
			static_cast<u8>(max_depth), type, best
		};
		
	}
}

TTEntry Engine::probeTT(u64 hash_key) const {
	u64 index = static_cast<std::uint64_t>((static_cast<unsigned __int128>(hash_key) *
				static_cast<unsigned __int128>(tt.size())) >> 64);
	if (tt[index].hash == (hash_key & 0xFFFFFFFFull)) {
		return tt[index];
	}
	return TTEntry();
}


bool Engine::checkTime(bool strict) {
	static u32 counter = 0;
	if (time_over) return true;
	counter++;
	if (strict || (counter & 0x80)) {
		if ((std::clock() - start_time) > max_time) {
			time_over = true;
			return true;
		}
		counter = 0;
	}
	return false;
}

void Engine::calcTime() {
	time_over = false;
	if (tc.movetime) {
		max_time = tc.movetime * CLOCKS_PER_SEC / 1000;
		return;
	}
	int search_ply = b.ply - start_ply;

	double num_moves = static_cast<double>(search_stack->moves.size());
	double factor = num_moves / 1000.0;
	if (expected_response != b.state_stack.back().move) {
		factor *= 2.0;
	} else if (expected_response.captured()) {
		factor /= 2.0;
	}

	double total_time = b.us ? tc.btime : tc.wtime;
	double inc = b.us ? tc.binc : tc.winc;

	if (total_time < (total_time * factor) + inc * 0.80) {
		max_time = std::min(inc * 0.80, inc * factor);
	} else {
		max_time = (total_time * factor) + inc * 0.80;
	}

	max_time = max_time * CLOCKS_PER_SEC / 1000;
}

void Engine::updatePV(int depth, Move move) {
	pv_table[depth][0] = move;

	for (int i = 0; i < pv_length[depth + 1]; i++) {
		pv_table[depth][i + 1] = pv_table[depth + 1][i];
	}
	pv_length[depth] = pv_length[depth + 1] + 1;
}

void Engine::updateHistoryBonus(Move move, int depth_left) {
	int bonus = std::min(280 * depth_left - 432, 2576);
	bonus = std::clamp(bonus, -16384, 16384);
	history_table[b.us][move.from()][move.to()] +=
		bonus - history_table[b.us][move.from()][move.to()] * abs(bonus) / 16384;
}

void Engine::updateHistoryMalus(Move move, int depth_left) {
	int malus = -std::min(343 * depth_left - 161, 1239);
	malus = std::clamp(malus, -16384, 16384);
	history_table[b.us][move.from()][move.to()] +=
		malus - history_table[b.us][move.from()][move.to()] * abs(malus) / 16384;
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
