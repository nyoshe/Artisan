#include "Engine.h"



namespace
{
	auto calc_lmr_base() {
		std::array<std::array<int, 256>, MAX_PLY> lmr_base;
		for (int depth = 0; depth < MAX_PLY; depth++) {
			for (int move = 0; move < 256; move++) {
				lmr_base[depth][move] = 2.0 + std::log(depth) * std::log(move) / 2.5;
			}
		}
		return lmr_base;
	}

	auto lmr_base = calc_lmr_base();
}

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
	if (!uci_options.uci && !do_bench) {
		b.printBoard();
		std::cout << "    depth   score      time      nodes          nps     hash   pv\n"
			         "-----------------------------------------------------------------\n";
	}

	search_stack->clear();
	b.genPseudoLegalMoves(search_stack->moves);
	b.filterToLegal(search_stack->moves);
	max_depth = 1;

	if (search_stack->moves.size() == 1) {
		printPV(alphaBeta(-100000, 100000, max_depth, false, search_stack));
		search_stack->clear();
		b.genPseudoLegalMoves(search_stack->moves);
		b.filterToLegal(search_stack->moves);

		return search_stack->moves[0];
	}

	if (depth == -1) calcTime();
	else max_time = -1;

	
	int score = alphaBeta(-100000, 100000, max_depth, false, search_stack);

	Move best_move = pv_table[0][0];

	bool add_time = false;

	for (max_depth = 1; max_depth < ((depth == -1) ? MAX_PLY : depth); max_depth++) {

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
				int total_time = b.us ? tc.btime : tc.wtime;
				int inc = b.us ? tc.binc : tc.winc;
				if (max_time * 3 < total_time - inc - 10) {
					max_time = max_time * 3;
					add_time = true;
					continue;
				}
			}
			break;
			//return best_move;
		} else if ((std::clock() - start_time) > max_time / 2 && max_time != -1) {
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

	if (b.isLegal(best_move)) {
		return best_move;
	} else {
		//this should really never happen 
		search_stack->moves.clear();
		b.genPseudoLegalMoves(search_stack->moves);
		b.filterToLegal(search_stack->moves);
		return search_stack->moves[0];
	}
}

void Engine::bench() {
	auto start_bench_time = std::clock();
	int total_nodes = 0;
	do_bench = true;
	for (auto position : bench_fens) {
		tc.movetime = INT32_MAX;
		b = Board();
		std::istringstream iss(position);
		setBoardFEN(iss);
		search(12);
		total_nodes += nodes;
	}
	int nps = static_cast<float>(total_nodes) / (static_cast<float>(std::clock() - start_bench_time) / CLOCKS_PER_SEC);

	std::cout << total_nodes << " nodes " << nps << " nps" << std::endl;
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

	if (search_ply >= MAX_PLY - 1) return b.getEval();
	if (b.isRepetition(is_root ? 2 : 1) || b.half_move >= 100) {
		return 0;
	}

	if (checkTime(false)) return b.getEval();

	ss->in_check = b.isCheck();
	
	if (depth_left <= 0 && ss->in_check) { depth_left = 1; }
	
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
	if (is_root) b.filterToLegal(ss->moves);
	//b.filterToLegal(ss->moves);

	// Check for #M
	

	int moves_searched = 0;
	MovePick move_gen;
	bool raised_alpha = false;
	bool only_noisy = false;
	while (const Move move = move_gen.getNext(*this, b, ss, 0)) {
		//if (!b.isLegal(move)) continue;
		if (checkTime(false)) return best;

		moves_searched++;
		int score = 0;

		bool is_quiet = !(move.captured() || move.promotion());
		if (only_noisy && is_quiet) continue;

		if (is_quiet) {
			ss->seen_quiets.emplace_back(move);
		} else {
			ss->seen_noisies.emplace_back(move);
		}
		int hist = is_quiet ? history_table[b.us][move.from()][move.to()] : capture_history[b.us][move.piece()][move.captured()][move.to()];

		int see_margin[2];
		see_margin[0] = -20 * depth_left * depth_left;
		see_margin[1] = -64 * depth_left;
		
		if (best > -30000
			&& depth_left <= 10
			&& move_gen.stage > MoveStage::good_captures
			&& !b.staticExchangeEvaluation(move, see_margin[is_quiet] - hist / 512)){

			continue;
		}

		b.doMove(move);
		ss->current_move = move;
		int extension = 0;
		int new_depth = depth_left - 1 + extension;
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

			if (raised_alpha && moves_searched > 4 && depth_left < 4 && hist < -1024 * depth_left) {
				b.undoMove();
				break;
			}
		}
		
		

		//search reductions
		if (moves_searched > 1 + 2 * is_pv + ss->improving  && depth_left > 2 && !is_root) {
			int R = 0;
			
			if (!is_quiet) {
				//R = static_cast<int>(0.5 + log_table[depth_left] * log_table[moves_searched] / 3.5);
				R = 3 - hist / 5000;
			} else {
				R = lmr_base[depth_left][moves_searched];
				//R += move_is_check && move.piece() == eKing;
				//R += move_gen.stage == MoveStage::killer;
				//R -= hist / 10000;
				//R += tt_entry ? (tt_entry.best_move.captured() || tt_entry.best_move.promotion()) : 0;
				R += cut_node;
				R += !ss->improving;
			}
			R -= move_is_check;
			R += !is_pv;
			//R -= history_table[!b.us][move.from()][move.to()] / 8870;
			//R += tt_entry ? (tt_entry.best_move.captured() || tt_entry.best_move.promotion()) : 0;

			int lmr_depth = std::clamp(depth_left - R, 1, depth_left);
			score = -alphaBeta(-alpha - 1, -alpha, lmr_depth, true, ss + 1);
			if (score > alpha) {
				int post_lmr_depth = new_depth;
				//history_table[!b.us][move.from()][move.to()];
				//new_depth += score > best + 50;
				post_lmr_depth -= score < (best + 10 * depth_left);
				post_lmr_depth = std::min(post_lmr_depth, depth_left);
				//research full depth if we fail high
				
				score = -alphaBeta(-alpha - 1, -alpha, post_lmr_depth,  !cut_node, ss + 1);
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
				updateQuietHistory(ss, depth_left);
			}
			updateNoisyHistory(ss, depth_left);

			storeTTEntry(b.getHash(), best, TType::BETA_CUT, depth_left, best_move);
			return best;
		}
		
	}
	if (moves_searched == 0 && ss->in_check) {
		return -99999 + b.ply - start_ply;
	}
	if (moves_searched == 0) {
		return 0;
	}


	if (raised_alpha) {
		storeTTEntry(b.getHash(), best, TType::EXACT, depth_left, best_move);
	}
	else {
		storeTTEntry(b.getHash(), best, TType::FAIL_LOW, depth_left, best_move);
	}
	return best;
}

int Engine::quiesce(int alpha, int beta, bool cut_node, SearchStack* ss) {
	ss->clear();
	nodes++;
	int search_ply = b.ply - start_ply;
	sel_depth = std::max(search_ply, sel_depth);
	if (search_ply >= MAX_PLY - 1) return b.getEval();
	if (b.isRepetition(1) || b.half_move >= 100) return 0;

	ss->static_eval = b.getEval();
	ss->in_check = b.isCheck();
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
	if (!cut_node && entry) {
		if (entry.type == TType::EXACT) return entry.eval;
		if (entry.type == TType::BETA_CUT && entry.eval >= beta) return entry.eval;
		if (entry.type == TType::FAIL_LOW && entry.eval <= alpha) return entry.eval;
	}

	/*
	if (!ss->in_check) {
		b.genPseudoLegalCaptures(ss->moves);
	} else {
		b.genPseudoLegalMoves(ss->moves);
	}
	b.filterToLegal(ss->moves);

	// Check for #M
	if (ss->moves.empty()) {
		if (!ss->in_check) {
			ss->moves.clear();
			b.genPseudoLegalMoves(ss->moves);
			b.filterToLegal(ss->moves);
		}
		
		if (ss->moves.empty() && b.isCheck()) {
			return -99999 + b.ply - start_ply;
		}
		if (ss->moves.empty()) {
			return 0;
		}
		return stand_pat;
	}
	*/

	b.genPseudoLegalCaptures(ss->moves);
	if (ss->moves.empty()) {
		return stand_pat;
	}

	bool raised_alpha = false;
	Move best_move;
	MovePick move_gen;
	int moves_searched = 0;
	while (Move move = move_gen.getNext(*this, b, ss, alpha - stand_pat - 120)) {
		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);
		if (checkTime(false)) return best;
		moves_searched++;
		if (move_gen.stage > MoveStage::good_captures) {
			//count remaining legal moves
			while (move_gen.getNext(*this, b, ss, alpha - stand_pat - 120)) moves_searched++;
			break;
		}

		b.doMove(move);
		int score = -quiesce(-beta, -alpha, cut_node, ss + 1);
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
	}
	if (moves_searched == 0) {
		ss->moves.clear();
		b.genPseudoLegalMoves(ss->moves);
		b.filterToLegal(ss->moves);
		if (ss->in_check) {
			return -99999 + b.ply - start_ply;
		} else {
			if (ss->moves.empty()) return 0;
			//return stand_pat;
		}
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
	if (do_bench) return;
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
	chess::Movelist ml;
	chess::movegen::legalmoves(ml, test_b);
	std::string ssss = chess::uci::moveToUci(ml.front());
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
		if ((std::clock() - start_time) > max_time && max_time != -1) {
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
		max_time = tc.movetime * (CLOCKS_PER_SEC / 1000);
		return;
	}
	int search_ply = b.ply - start_ply;

	double num_moves = static_cast<double>(search_stack->moves.size());
	double factor = num_moves / 1000.0;
	if (!b.state_stack.empty()) {
		if (expected_response != b.state_stack.back().move) {
			factor *= 2.0;
		}
		else if (expected_response.captured()) {
			factor /= 2.0;
		}
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

void Engine::updateQuietHistory(SearchStack* ss, int depth_left) {
	Move move = ss->current_move;
	//int bonus = std::min(280 * depth_left - 432, 2576);
	//int malus = -std::min(343 * depth_left - 161, 1239);
	int bonus = std::min(7 * depth_left * depth_left + 274 * depth_left - 182, 2048);
	int malus = -std::min(5 * depth_left * depth_left + 283 * depth_left + 169, 1024);

	bonus = std::clamp(bonus, -16384, 16384);
	history_table[b.us][move.from()][move.to()] +=
		bonus - history_table[b.us][move.from()][move.to()] * abs(bonus) / 16384;

	for (auto& quiet : ss->seen_quiets) {
		malus = std::clamp(malus, -16384, 16384);
		history_table[b.us][quiet.from()][quiet.to()] +=
			malus - history_table[b.us][quiet.from()][quiet.to()] * abs(malus) / 16384;
	}
}

void Engine::updateNoisyHistory(SearchStack* ss, int depth_left) {
	Move move = ss->current_move;
	bool is_quiet = !(move.captured() || move.promotion());
	//int bonus = std::min(280 * depth_left - 432, 2576);
	//int malus = -std::min(343 * depth_left - 161, 1239);
	int bonus = std::min(7 * depth_left * depth_left + 274 * depth_left - 182, 2048);
	int malus = -std::min(5 * depth_left * depth_left + 283 * depth_left + 169, 1024);

	if (!is_quiet) {
		bonus = std::clamp(bonus, -16384, 16384);
		capture_history[b.us][move.piece()][move.captured()][move.to()] +=
			bonus - capture_history[b.us][move.piece()][move.captured()][move.to()] * abs(bonus) / 16384;
	}

	for (auto& capture : ss->seen_noisies) {
		malus = std::clamp(malus, -16384, 16384);
		capture_history[b.us][capture.piece()][capture.captured()][capture.to()] +=
			malus - capture_history[b.us][capture.piece()][capture.captured()][capture.to()] * abs(malus) / 16384;
	}
}


Engine::Engine(UciOptions options) {
	uci_options = options;
	tt.clear();
	tt.resize((uci_options.hash_size * 1024 * 1024) / sizeof(TTEntry));
	b = Board();
	reset();
}

void Engine::reset() {
	for (auto& i : history_table) {
		for (auto& j : i) {
			std::ranges::fill(j.begin(), j.end(), 0);
		}
	}
	for (auto& i : capture_history) {
		for (auto& j : i) {
			for (auto& k : j) {
				std::ranges::fill(k.begin(), k.end(), 0);
			}
		}
	}
	for (auto& i : pv_table) {
		std::ranges::fill(i.begin(), i.end(), Move());
	}

	std::ranges::fill(pv_length.begin(), pv_length.end(), 0);

	for (int i = 0; i < MAX_PLY; i++) {
		search_stack[i].clear();
		search_stack[i].killers[0] = Move(0, 0);
		search_stack[i].killers[1] = Move(0, 0);
	}
	nodes = 0;
	hash_hits = 0;
	hash_count = 0;
	hash_miss = 0;
	do_bench = false;
	max_depth = 0;
	sel_depth = 0;
	start_ply = 0;
	time_over = false;
	root_best = Move(0, 0);
	expected_response = Move(0, 0);
	perf_values.clear();
	pos_count = 0;
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
	std::string board_part, active_color, castling, ep, halfmove, fullmove;
	fen >> board_part >> active_color >> castling >> ep >> halfmove >> fullmove;
	std::string fen_string = std::string(board_part + " " + active_color + " " + castling + " " + ep + " " + halfmove + " " + fullmove);
	b.loadBoard(chess::Board::fromFen(fen_string));
}

void Engine::setBoardUCI(std::istringstream& uci) {
	std::string token;
	std::vector<std::string> tokens;
	while (uci >> token) {
		tokens.push_back(token);
	}
	if (tokens.empty()) return;

	size_t idx = 0;
	if (idx >= tokens.size()) return;

	// Handle "moves" and apply each move
	if (idx < tokens.size() && tokens[idx] == "moves") {
		++idx;
		for (; idx < tokens.size(); ++idx) {
			Move move = b.moveFromUCI(tokens[idx]);
			if (move.raw() != 0) {
				b.doMove(move);
#ifdef DEBUG
				if (calcHash() != hash) {
					throw std::logic_error("hashing error");
				}
#endif
			}
			else {
				// Invalid move, stop processing further
				break;
			}
		}
	}
}

std::vector<PerfT> Engine::doPerftSearch(std::string position, int depth) {
	b.loadBoard(chess::Board::fromFen(position));
	return doPerftSearch(depth);
}
