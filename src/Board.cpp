#include "Board.h"


Board::Board() {
	state_stack.reserve(512);
	reset();
}


void Board::loadBoard(chess::Board new_board) {
	eval = 0;
	ply = 0;
	hash = 0;
	half_move = 0;
	us = new_board.sideToMove();
	castle_flags = 0b1111;
	ep_square = -1;
	state_stack.clear();

	for (int i = 0; i < 64; i++) {
		chess::Piece piece = new_board.at(i);
		u8 p = piece.type() == 6 ? 0 : (piece.type() + 1);
		mailbox[i] = p;
	}
	for (auto side : { 0,1 }) {
		boards[side][ePawn] = new_board.pieces(chess::PieceType::PAWN, side).getBits();
		boards[side][eKnight] = new_board.pieces(chess::PieceType::KNIGHT, side).getBits();
		boards[side][eBishop] = new_board.pieces(chess::PieceType::BISHOP, side).getBits();
		boards[side][eRook] = new_board.pieces(chess::PieceType::ROOK, side).getBits();
		boards[side][eQueen] = new_board.pieces(chess::PieceType::QUEEN, side).getBits();
		boards[side][eKing] = new_board.pieces(chess::PieceType::KING, side).getBits();
	}

	setOccupancy();
	hash = calcHash();
	eval = evalUpdate();
	runSanityChecks();

}

bool Board::operator==(const Board& other) const {
	for (int side = 0; side < 2; ++side) {
		for (int piece = 0; piece < 7; ++piece) {
			if (boards[side][piece] != other.boards[side][piece])
				return false;
		}
	}
	//kinda cursed for quick comparison
	for (int i = 0; i < 8; ++i) {
		if (reinterpret_cast<const u64*>(&mailbox)[i] != reinterpret_cast<const u64*>(&mailbox)[i])
			return false;
	}
	if (ply != other.ply ||
		us != other.us ||
		castle_flags != other.castle_flags ||
		ep_square != other.ep_square ||
		hash != other.hash) {
		return false;
	}

	return true;
}

void Board::setOccupancy() {
	boards[eBlack][0] = 0;

	for (int p = 1; p < 7; p++) {
		boards[eBlack][0] |= boards[eBlack][p];
	}

	boards[eWhite][0] = 0;

	for (int p = 1; p < 7; p++) {
		boards[eWhite][0] |= boards[eWhite][p];
	}
}

void Board::doMove(Move move) {
	state_stack.emplace_back(ep_square, castle_flags, move, eval, hash, half_move);

	//null move
	if (move.from() == move.to()) {
		us = !us;
		//update bare minimum zobrist, clear ep square
		hash ^= z.side;
		if (state_stack.back().ep_square != -1) hash ^= z.ep_file[state_stack.back().ep_square & 0x7];
		if (ep_square != -1) hash ^= z.ep_file[ep_square & 0x7];

		// Increment ply count  
		ply++;
		ep_square = -1;
		half_move++;
		return;
	}

	if (move.piece() == ePawn || move.captured()) {
		half_move = 0;
	}
	else {
		half_move++;
	}
	movePiece(move.from(), move.to());

	u8 p = move.piece();

	// Handle promotion  
	if (move.promotion() != eNone) {
		boards[us][p] &= ~BB::set_bit[move.to()];
		boards[us][move.promotion()] |= BB::set_bit[move.to()];
		mailbox[move.to()] = move.promotion();
	}

	// Handle en passant  
	if (move.isEnPassant()) {
		u8 ep_capture_square = move.to() + (us == eWhite ? -8 : 8);
		boards[!us][ePawn] &= ~BB::set_bit[ep_capture_square];
		boards[!us][0] &= ~BB::set_bit[ep_capture_square];
		mailbox[ep_capture_square] = eNone;
	}

	if (p == eKing) {
		// Move the rook if castling
		if (std::abs((int)move.to() - (int)move.from()) == 2) {
			switch (move.to()) {
			case g1: movePiece(h1, f1); break; // King-side castling for white
			case c1: movePiece(a1, d1); break; // Queen-side castling for white
			case g8: movePiece(h8, f8); break; // King-side castling for black
			case c8: movePiece(a8, d8); break; // Queen-side castling for black
			default: throw std::invalid_argument("Invalid castling move");
			}
		}
		//modify castle flags for king move
		castle_flags &= (us == eWhite) ? ~(wShortCastleFlag | wLongCastleFlag) : ~(bShortCastleFlag | bLongCastleFlag);
	}

	//modify castle flags for rook move
	if (p == eRook) {
		switch (move.from()) {
		case h1: castle_flags &= ~wShortCastleFlag; break; // White king-side rook
		case a1: castle_flags &= ~wLongCastleFlag; break; // White queen-side rook
		case h8: castle_flags &= ~bShortCastleFlag; break; // Black king-side rook
		case a8: castle_flags &= ~bLongCastleFlag; break; // Black queen-side rook
		default: break;
		}
	}

	switch (move.to()) {
	case h1: castle_flags &= ~wShortCastleFlag; break; // White king-side rook
	case a1: castle_flags &= ~wLongCastleFlag; break; // White queen-side rook
	case h8: castle_flags &= ~bShortCastleFlag; break; // Black king-side rook
	case a8: castle_flags &= ~bLongCastleFlag; break; // Black queen-side rook
	default: break;
	}

	// Update en passant square
	if (p == ePawn && std::abs((int)move.to() - (int)move.from()) == 16) {
		ep_square = (move.from() + move.to()) / 2;
	}
	else {
		ep_square = -1;
	}

	us = !us;
	updateZobrist(move);

	//pos_history[hash]++;

	// Increment ply count  
	ply++;


#ifndef NDEBUG
	runSanityChecks();
#endif
}

void Board::undoMove() {
	if (state_stack.empty()) return;
	if (BB::popcnt(boards[eBlack][ePawn]) > 8) {
		printBitBoards();
		throw std::logic_error("too many pawns!");
	}
	// Pop the last move
	Move move = state_stack.back().move;

	us = !us;
	ep_square = state_stack.back().ep_square;
	castle_flags = state_stack.back().castle_flags;
	eval = state_stack.back().eval;
	hash = state_stack.back().hash;
	half_move = state_stack.back().half_move;
	state_stack.pop_back();
	// Switch side to move back

	// Decrement ply count
	ply--;

	//assume null move
	if (move.from() == move.to()) {
		return;
	}

	u8 from = move.from();
	u8 to = move.to();
	u8 piece = move.piece();
	u8 captured = move.captured();
	u8 promotion = move.promotion();
	// Handle castling undo (move rook back)
	if (piece == eKing && std::abs((int)to - (int)from) == 2) {
		if (us == eWhite) {
			if (to == 6) movePiece(5, 7); // King-side
			else if (to == 2) movePiece(3, 0); // Queen-side
		}
		else {
			if (to == 62) movePiece(61, 63); // King-side
			else if (to == 58) movePiece(59, 56); // Queen-side
		}
	}

	// Handle promotion reversal
	if (promotion != eNone) {
		removePiece(to); // Remove promoted piece
		setPiece(from, us, ePawn); // Restore pawn to 'from'
	}
	else {
		removePiece(to);
		setPiece(from, us, piece);
		//movePiece(to, from);
	}

	// Handle en passant undo
	if (move.isEnPassant()) {
		u8 ep_capture_square = to + (us == eWhite ? -8 : 8);
		setPiece(ep_capture_square, !us, ePawn);
	}
	else {
		if (captured != eNone) {
			setPiece(to, !us, captured);
		}
	}

#ifndef NDEBUG
	runSanityChecks();
#endif
}

void Board::printBoard() const {
#if defined(_MSC_VER)
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	bool color = true;

	std::cout << "  a b c d e f g h \n";

	for (int rank = 7; rank >= 0; rank--) {
		std::cout << std::to_string(rank + 1) << " ";
		for (int file = 0; file <= 7; file++) {

			// Set background color for the square  
			std::string bgColor = "\x1b[48;2;" + std::to_string(color ? 252 : 230) + ";" +
				std::to_string(color ? 197 : 151) + ";" +
				std::to_string(color ? 142 : 55) + "m";
			printf(bgColor.c_str());

			u64 mask_pos = (1ULL << (file | (rank << 3)));

			// Check if no piece is present  
			if (!((boards[eWhite][0] & mask_pos) || (boards[eBlack][0] & mask_pos))) {
				std::cout << "  ";
				color = !color;
				continue;
			}

			// Print the piece if found  
			for (int side = 0; side < 2; side++) {
				// Set text color based on side  
				std::string textColor = (side == eBlack)
					? "\x1b[38;2;0;0;0m"
					: "\x1b[38;2;255;255;255m";
				printf(textColor.c_str());

				wchar_t* pieceChar = nullptr;
				if (boards[side][eQueen] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265B ");
				else if (boards[side][eKing] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265A ");
				else if (boards[side][eRook] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265C ");
				else if (boards[side][eBishop] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265D ");
				else if (boards[side][eKnight] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265E ");
				else if (boards[side][ePawn] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265F ");

#if defined(_MSC_VER)
				if (pieceChar) {
					WriteConsoleW(hOut, pieceChar, wcslen(pieceChar), nullptr, nullptr);
					break;
				}
#elif defined(__GNUC__) || defined(__clang__)
				if (pieceChar) {
					std::wcout << pieceChar;
					break;
				}
#endif
			}

			color = !color;
		}

		// Reset background and text color for the row  
		printf("\x1b[48;2;0;0;0m");
		printf("\x1b[38;2;255;255;255m");

		// Print rank number  
		std::cout << " " << static_cast<char>('1' + rank) << " \n";
		color = !color;
	}

	// Print file labels  
	std::cout << "  a b c d e f g h \n\n";
}

void Board::printBitBoards() const {
	const std::string names[] = {
		"occupancy", "pawn", "knight", "bishop",
		"rook", "queen", "king"
	};

	for (int side = eWhite; side <= eBlack; side++) {
		std::cout << std::left;
		for (const auto& name : names) {
			std::cout << std::setw(18) << (side == eWhite ? "white " + name : "black " + name);
		}
		std::cout << "\n";

		for (int rank = 7; rank >= 0; rank--) {
			for (int i = 0; i < 7; i++) {
				std::cout << BB::rank_to_string(boards[side][i], rank) + "  ";
			}
			std::cout << "\n";
		}
		std::cout << "\n";
	}
}

std::string Board::boardString() const {
	std::string out;
	out += "  a b c d e f g h\n";
	for (int rank = 7; rank >= 0; rank--) {
		out += std::to_string(rank + 1);
		for (int file = 0; file <= 7; file++) {
			u64 mask_pos = (1ULL << (file | (rank << 3)));
			if (!(boards[eWhite][0] & mask_pos) && !(boards[eBlack][0] & mask_pos)) {
				out += " .";
			}
			else {


				for (int side = 0; side < 2; side++) {
					if (boards[side][eQueen] & mask_pos) out += " Q";
					else if (boards[side][eKing] & mask_pos) out += " K";
					else if (boards[side][eRook] & mask_pos) out += " R";
					else if (boards[side][eBishop] & mask_pos) out += " B";
					else if (boards[side][eKnight] & mask_pos) out += " N";
					else if (boards[side][ePawn] & mask_pos) out += " P";

				}
				if (getSide(file | (rank << 3)) == eBlack) out[out.size() - 1] += 32;
			}
		}
		out += " " + std::to_string(rank + 1) + "\n";
	}
	out += "  a b c d e f g h\n";
	return out;
}

void Board::movePiece(u8 from, u8 to) {
	//clear and set our piece
	Side color = getSide(from);
#ifdef DEBUG
	if (color == eSideNone) {
		printBitBoards();
		printBoard();
		undoMove();
		printBitBoards();
		printBoard();
		throw std::logic_error("weird");
	}
#endif
	boards[color][mailbox[from]] ^= BB::set_bit[from];
	boards[color][0] ^= BB::set_bit[from];
	boards[color][mailbox[from]] |= BB::set_bit[to];
	boards[color][0] |= BB::set_bit[to];
	//clear enemy bit
	boards[!color][mailbox[to]] &= ~BB::set_bit[to];
	boards[!color][0] &= ~BB::set_bit[to];

	mailbox[to] = mailbox[from];
	mailbox[from] = eNone;
}

void Board::setPiece(u8 square, u8 color, u8 piece) {
	boards[color][piece] |= BB::set_bit[square];
	boards[color][0] |= BB::set_bit[square];
	mailbox[square] = piece;
}

void Board::removePiece(u8 square) {
	Side color = getSide(square);
	if (color != eSideNone) {
		boards[color][mailbox[square]] &= ~BB::set_bit[square];
		boards[color][0] &= ~BB::set_bit[square];
	}

	mailbox[square] = eNone;
}

Side Board::getSide(int square) const {
	return boards[eWhite][0] & BB::set_bit[square]
		? eWhite
		: (boards[eBlack][0] & BB::set_bit[square] ? eBlack : eSideNone);
}

void Board::loadFen(std::istringstream& fen_stream) {
	
	reset();
	// Clear all bitboards and piece_board
	for (int side = 0; side < 2; ++side)
		for (int piece = 0; piece < 7; ++piece)
			boards[side][piece] = 0;
	for (int i = 0; i < 64; ++i)
		mailbox[i] = eNone;

	std::string board_part, active_color, castling, ep, halfmove, fullmove;
	fen_stream >> board_part >> active_color >> castling >> ep >> halfmove >> fullmove;
	start_fen = std::string(board_part + " " + active_color + " " + castling + " " + ep + " " + halfmove + " " + fullmove);
	// Parse board
	int square = 56; // a8
	for (char c : board_part) {
		if (c == '/') {
			square -= 16; // move to next rank
		}
		else if (isdigit(c)) {
			square += c - '0';
		}
		else {
			int color = isupper(c) ? eWhite : eBlack;
			int piece = eNone;
			switch (tolower(c)) {
			case 'p': piece = ePawn; break;
			case 'n': piece = eKnight; break;
			case 'b': piece = eBishop; break;
			case 'r': piece = eRook; break;
			case 'q': piece = eQueen; break;
			case 'k': piece = eKing; break;
			}
			if (piece != eNone) {
				boards[color][piece] |= BB::set_bit[square];
				boards[color][0] |= BB::set_bit[square];
				mailbox[square] = piece;
			}
			++square;
		}
	}

	// Parse active color
	us = (active_color == "w") ? eWhite : eBlack;

	castle_flags = 0;
	// Parse castling rights
	for (char c : castling) {
		switch (c) {
		case 'K': castle_flags |= wShortCastleFlag; break;
		case 'Q': castle_flags |= wLongCastleFlag; break;
		case 'k': castle_flags |= bShortCastleFlag; break;
		case 'q': castle_flags |= bLongCastleFlag; break;
		case '-': break;
		}
	}

	// Parse en passant square
	if (ep != "-" && ep.length() == 2) {
		int file = ep[0] - 'a';
		int rank = ep[1] - '1';
		ep_square = file + (rank << 3);
	}
	else {
		ep_square = -1;
	}

	if (fullmove[0] != 'c') {
		if (!fullmove.empty())
			ply = std::stoi(fullmove);

		if (!halfmove.empty())
			half_move = std::stoi(halfmove);
	}
	

	// Recompute occupancy
	setOccupancy();
	hash = calcHash();
	eval = evalUpdate();
	runSanityChecks();
}

Move Board::moveFromUCI(const std::string& uci) {
	StaticVector<Move> moves;
	genPseudoLegalMoves(moves);
	filterToLegal(moves);

	for (const auto& move : moves) {
		if (move.toUci() == uci) {
			return move;
		}
	}
#ifdef DEBUG
	throw std::logic_error("invalid move!");
#endif
	return Move(0, 0);
}

void Board::loadUci(std::istringstream& iss) {
	std::string token;
	std::vector<std::string> tokens;
	while (iss >> token) {
		tokens.push_back(token);
	}
	if (tokens.empty()) return;

	size_t idx = 0;
	if (idx >= tokens.size()) return;

	// Handle "moves" and apply each move
	if (idx < tokens.size() && tokens[idx] == "moves") {
		++idx;
		for (; idx < tokens.size(); ++idx) {
			Move move = moveFromUCI(tokens[idx]);
			if (move.raw() != 0) {
				doMove(move);
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
	setOccupancy();
	hash = calcHash();
	eval = evalUpdate();
	runSanityChecks();
}



void Board::genPseudoLegalMoves(StaticVector<Move>& moves) {
	const int them = us ^ 1;

	u64 our_occ = boards[us][0];
	u64 their_occ = boards[them][0];
	u64 all_occ = our_occ | their_occ;

	// PAWNS
	u64 pawns = boards[us][ePawn];
	int forward = (us == eWhite) ? 8 : -8;
	int promo_rank = (us == eWhite) ? 6 : 1;

	// Single pushes
	u64 single_push = (us == eWhite) ? (pawns << 8) : (pawns >> 8);
	single_push &= ~all_occ;

	u64 attacks = single_push;

	genPseudoLegalCaptures(moves);

	while (attacks) {
		unsigned long to;
		BB::bitscan_reset(to, attacks);
		int from = to - forward;
		if ((from >> 3) == promo_rank) {
			// Promotions
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back({ u8(from), u8(to), ePawn, eNone, u8(promo) });
		}
		else {
			moves.emplace_back({ u8(from), u8(to), ePawn });
		}
	}

	// Double pushes
	u64 double_push = (us == eWhite ? (single_push << 8) : (single_push >> 8)) & ~all_occ;
	attacks = double_push;
	while (attacks) {
		unsigned long to;
		BB::bitscan_reset(to, attacks);
		int from = to + (us == eWhite ? -16 : 16);
		if ((us == eWhite && (from >> 3 == 1)) || (us == eBlack && (from >> 3) == 6)) {
			moves.emplace_back({ u8(from), u8(to), ePawn });
		}
	}

	serializeMoves(eKnight, moves, true);
	serializeMoves(eBishop, moves, true);
	serializeMoves(eRook, moves, true);
	serializeMoves(eQueen, moves, true);
	serializeMoves(eKing, moves, true);

	// --- Castling logic ---
	// King and rook must be on their original squares, and squares between must be empty
	// e1 = 4, h1 = 7, a1 = 0 (White)
	// e8 = 60, h8 = 63, a8 = 56 (Black)
	if (!isCheck()) {
		if (us == eWhite) {
			if ((castle_flags & wShortCastleFlag) && !(u64(0b01100000) & all_occ)) moves.emplace_back({ e1, g1, eKing });
			if ((castle_flags & wLongCastleFlag) && !(u64(0b00001110) & all_occ)) moves.emplace_back({e1, c1, eKing});
		}
		else {
			if ((castle_flags & bShortCastleFlag) && !((u64(0b01100000) << 56) & all_occ)) moves.emplace_back({
				e8, g8, eKing
				});
			if ((castle_flags & bLongCastleFlag) && !((u64(0b00001110) << 56) & all_occ)) moves.emplace_back({
				e8, c8, eKing
		});
		}
	}
}

void Board::genPseudoLegalCaptures(StaticVector<Move>& moves) {
	const int them = us ^ 1;
	u64 our_occ = boards[us][0];
	u64 their_occ = boards[them][0];
	u64 all_occ = our_occ | their_occ;

	// PAWNS
	u64 pawns = boards[us][ePawn];
	int promo_rank = (us == eWhite) ? 6 : 1;

	// Single pushes
	u64 single_push = (us == eWhite) ? (pawns << 8) : (pawns >> 8);
	single_push &= ~all_occ;

	// Pawn captures

	u64 left_captures = BB::get_pawn_attacks(eWest, Side(us), pawns, their_occ);
	u64 right_captures = BB::get_pawn_attacks(eEast, Side(us), pawns, their_occ);


	while (left_captures) {
		unsigned long to;
		BB::bitscan_reset(to, left_captures);
		int from = to - ((us == eWhite) ? 7 : -9);
		if ((from >> 3) == promo_rank) {
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back({ u8(from), u8(to), ePawn, mailbox[to], u8(promo) });
		}
		else {
			moves.emplace_back({u8(from), u8(to), ePawn, mailbox[to]
		});
		}
	}
	while (right_captures) {
		unsigned long to;
		BB::bitscan_reset(to, right_captures);
		int from = to - ((us == eWhite) ? 9 : -7);
		if ((from >> 3) == promo_rank) {
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back({u8(from), u8(to), ePawn, mailbox[to], u8(promo)
		});
		}
		else {
			moves.emplace_back({ u8(from), u8(to), ePawn, mailbox[to] });
		}
	}


	if (ep_square != -1) {
		int ep_from = ep_square + (us == eWhite ? -8 : 8);
		// Left capture
		if ((ep_square & 7) > 0 && (pawns & BB::set_bit[ep_from - 1])) {
			moves.emplace_back({ u8(ep_from - 1), u8(ep_square), ePawn, ePawn, eNone, true });
		}
		// Right capture
		if ((ep_square & 7) < 7 && (pawns & BB::set_bit[ep_from + 1])) {
			moves.emplace_back({ u8(ep_from + 1), u8(ep_square), ePawn, ePawn, eNone, true });
		}
	}

	serializeMoves(eKnight, moves, false);
	serializeMoves(eBishop, moves, false);
	serializeMoves(eRook, moves, false);
	serializeMoves(eQueen, moves, false);
	serializeMoves(eKing, moves, false);

}

void Board::serializeMoves(Piece piece, StaticVector<Move>& moves, bool quiet) {

	u64 all_occ = boards[eBlack][0] | boards[eWhite][0];
	u64 our_occ = boards[us][0];
	u64 attackers = boards[us][piece];
	u64 mask = quiet ? ~all_occ : all_occ;
	unsigned long from;
	
	while (attackers) {
		BB::bitscan_reset(from, attackers);
		u64 targets = 0;
		switch (piece) {
		case eKnight: targets = BB::knight_attacks[from] & ~our_occ & mask; break;
		case eBishop: targets = BB::get_bishop_attacks(from, all_occ) & ~our_occ & mask; break;
		case eRook: targets = BB::get_rook_attacks(from, all_occ) & ~our_occ & mask; break;
		case eQueen: targets = BB::get_queen_attacks(from, all_occ) & ~our_occ & mask; break;
		case eKing: targets = BB::king_attacks[from] & ~our_occ & mask; break;
		}
		unsigned long to;
		while (targets) {
			BB::bitscan_reset(to, targets);
			moves.emplace_back({ static_cast<u8>(from), static_cast<u8>(to), piece, mailbox[to] });
		}
	}
}

void Board::filterToLegal(StaticVector<Move>& moves) {
	//could be made quicker, perhaps only checking pieces between king and attackers
	int new_i = 0;
	if (isRepetition(2)) {
		moves.resize(0);
		return;
	}
	//bool current_check = isCheck();
	for (unsigned int i = 0; i < moves.size();i++) {
		Move move = moves[i];
		if (move.isCastle()) {
			if (getAttackers((move.to() + move.from()) / 2, us)) {
				//moves.erase(moves.begin() + i);
				continue;
			}
		}
		int p = move.piece();
		doMove(move);
		if (half_move > 100) {
			undoMove();
			continue;
		}
		us ^= 1;
		bool inCheck = isCheck();
		us ^= 1;
		undoMove();

		if (!inCheck) {
			//i++
			moves[new_i] = moves[i];
			new_i++;
			continue;
		}
		else {
			//moves.erase(moves.begin() + i);
		}
	}
	moves.resize(new_i);
}

bool Board::isLegal(Move move) {
	StaticVector<Move> vmove;
	vmove.emplace_back(move);
	if (move.raw() == 0) return false;

	if (mailbox[move.from()] == move.piece() && mailbox[move.to()] == move.captured()) {
		filterToLegal(vmove);
		return !vmove.empty();
	}
	return false;
}

u64 Board::getAttackers(int square) const {
	return getAttackers(square, us);
}

u64 Board::getAttackers(int square, bool side) const {
	u64 attackers = 0;
	u64 square_mask = BB::set_bit[square];
	u64 our_occ = boards[side][0];
	u64 their_occ = boards[!side][0];
	u64 all_occ = our_occ | their_occ;

	u64 west_defenders = BB::get_pawn_attacks(eWest, static_cast<Side>(side), square_mask, boards[!side][ePawn]);
	u64 east_defenders = BB::get_pawn_attacks(eEast, static_cast<Side>(side), square_mask, boards[!side][ePawn]);

	attackers |= east_defenders | west_defenders;

	// Check knights  
	attackers |= BB::knight_attacks[square] & boards[!side][eKnight];
	//check bishops
	attackers |= BB::get_bishop_attacks(square, all_occ) & ~our_occ & (boards[!side][eBishop] | boards[!side][eQueen]);
	// Check rooks
	attackers |= BB::get_rook_attacks(square, all_occ) & ~our_occ & (boards[!side][eRook] | boards[!side][eQueen]);

	// Check king  
	attackers |= BB::king_attacks[square] & boards[!side][eKing];
	return attackers;
}

bool Board::isCheck() const {
	return getAttackers(BB::bitscan(boards[us][eKing]), us);
}

int Board::evalUpdate(Move move)  {
	int out = 0;
	if (true) {
		return evalUpdate();
	}
}


void Board::runSanityChecks() const {
	if (BB::popcnt(boards[eBlack][ePawn]) > 8 || BB::popcnt(boards[eWhite][ePawn]) > 8) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("too many pawns!");
	}

	if (BB::popcnt(boards[eBlack][ePawn]) > 8) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("too many pawns!");
	}
	if (boards[eWhite][0] != (boards[eWhite][ePawn] | boards[eWhite][eKnight] | boards[eWhite][eBishop] | boards[eWhite]
		[eRook] | boards[eWhite][eQueen] | boards[eWhite][eKing])) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("white bitboard mismatch");
	}
	if (boards[eBlack][0] != (boards[eBlack][ePawn] | boards[eBlack][eKnight] | boards[eBlack][eBishop] | boards[eBlack]
		[eRook] | boards[eBlack][eQueen] | boards[eBlack][eKing])) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("black bitboard mismatch");
	}

}

void Board::printMoves() const {
	for (auto state : state_stack) {
		std::cout << state.move.toUci() << " ";
	}
	std::cout << "\n";
}

void Board::reset() {
	// Reset all bitboards and piece_board to the initial chess position
	boards[eWhite][ePawn] = 0x000000000000FF00;
	boards[eBlack][ePawn] = 0x00FF000000000000;

	boards[eWhite][eKnight] = 0b01000010;
	boards[eBlack][eKnight] = (u64(0b01000010) << 56);

	boards[eWhite][eBishop] = 0b00100100;
	boards[eBlack][eBishop] = (u64(0b00100100) << 56);

	boards[eWhite][eRook] = 0b10000001;
	boards[eBlack][eRook] = (u64(0b10000001) << 56);

	boards[eWhite][eQueen] = 0b00001000;
	boards[eBlack][eQueen] = (u64(0b00001000) << 56);

	boards[eWhite][eKing] = 0b00010000;
	boards[eBlack][eKing] = (u64(0b00010000) << 56);

	// Set up piece_board
	static const u8 initial_piece_board[64] = {
		eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook,
		ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
		eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
		eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
		eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
		eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
		ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
		eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook
	};
	for (int i = 0; i < 64; ++i) {
		mailbox[i] = initial_piece_board[i];
	}

	eval = 0;
	ply = 0;
	hash = 0;
	half_move = 0;
	us = eWhite;
	castle_flags = 0b1111;
	ep_square = -1;
	state_stack.clear();

	//legal_moves.reserve(256);
	setOccupancy();
	hash = calcHash();
}

std::vector<Move> Board::getLastMoves(int n_moves) const {
	std::vector<Move> last_moves;
	const size_t available_moves = state_stack.size();
	const size_t moves_to_return = std::min(static_cast<size_t>(n_moves), available_moves);

	last_moves.reserve(moves_to_return);
	for (size_t i = moves_to_return; i; --i) {
		last_moves.push_back(state_stack[available_moves - i].move);
	}

	return last_moves;
}

u64 Board::getHash() const {
	return hash;
}

bool Board::isRepetition(int n) const {
	if (state_stack.empty()) return false;
	int counter = 0;
	for (int i = 1; i <= std::min(static_cast<int>(state_stack.size()), static_cast<int>(half_move + 1)); i++) {
		if (state_stack[state_stack.size() - i].hash == hash) {
			counter++;
		}
	}
	if (counter >= n) {
		return true;
	}
	return false;
}

u64 Board::calcHash() const {
	u64 out_hash = 0;
	for (int sq = 0; sq < 64; sq++) {
		if (mailbox[sq] != eNone) {
			out_hash ^= z.piece_at[sq * 12 + (mailbox[sq] - 1) + (getSide(sq) * 6)];
		}
	}
	out_hash ^= z.castle_rights[castle_flags];
	if (us) out_hash ^= z.side;
	if (ep_square != -1) out_hash ^= z.ep_file[ep_square & 0x7];
	return out_hash;
}

void Board::updateZobrist(Move move) {
	
	u8 p = move.piece();
	hash ^= z.side;
	hash ^= z.piece_at[(move.from() * 12) + (move.piece() - 1) + (!us * 6)]; //invert from square hash

	if (move.promotion() != eNone) {
		hash ^= z.piece_at[(move.to() * 12) + (move.promotion() - 1) + (!us * 6)];
	}
	else {
		hash ^= z.piece_at[(move.to() * 12) + (move.piece() - 1) + (!us * 6)];
	}

	if (move.captured() != eNone) {
		if (move.isEnPassant()) {
			hash ^= z.piece_at[((state_stack.back().ep_square - (!us ? 8 : -8)) * 12) + (ePawn - 1) + (us * 6)]; //invert captured piece
			//hash ^= z.piece_at[(move.to() * 12) + (ePawn - 1) + (!us * 6)]; //invert captured piece
		}
		else {
			hash ^= z.piece_at[(move.to() * 12) + (move.captured() - 1) + (us * 6)]; //invert captured piece
		}
	}

	if (state_stack.back().castle_flags != castle_flags) {
		hash ^= z.castle_rights[state_stack.back().castle_flags];
		hash ^= z.castle_rights[castle_flags];
	}

	if (state_stack.back().ep_square != -1) hash ^= z.ep_file[state_stack.back().ep_square & 0x7];
	if (ep_square != -1) hash ^= z.ep_file[ep_square & 0x7];

	if (p == eKing) {
		// Move the rook if castling
		if (std::abs((int)move.to() - (int)move.from()) == 2) {
			switch (move.to()) {
			case g1:
				hash ^= z.piece_at[(h1 * 12) + (eRook - 1) + (!us * 6)];
				hash ^= z.piece_at[(f1 * 12) + (eRook - 1) + (!us * 6)];
				break; // King-side castling for white
			case c1:
				hash ^= z.piece_at[(a1 * 12) + (eRook - 1) + (!us * 6)];
				hash ^= z.piece_at[(d1 * 12) + (eRook - 1) + (!us * 6)];
				break; // Queen-side castling for white
			case g8:
				hash ^= z.piece_at[(h8 * 12) + (eRook - 1) + (!us * 6)];
				hash ^= z.piece_at[(f8 * 12) + (eRook - 1) + (!us * 6)];
				break;// King-side castling for black
			case c8:
				hash ^= z.piece_at[(a8 * 12) + (eRook - 1) + (!us * 6)];
				hash ^= z.piece_at[(d8 * 12) + (eRook - 1) + (!us * 6)];
				break; // Queen-side castling for black
			default: throw std::invalid_argument("Invalid castling move");
			}
		}
	}
}

int Board::getMobility(bool side)  {
	int mobility = 0;
	u64 w_pawn_defenders =
		BB::get_pawn_attacks(eEast, eWhite, boards[eWhite][ePawn], 0xFFFFFFFFFFFFFFFF) |
		BB::get_pawn_attacks(eWest, eWhite, boards[eWhite][ePawn], 0xFFFFFFFFFFFFFFFF);
	u64 b_pawn_defenders =
		BB::get_pawn_attacks(eEast, eBlack, boards[eBlack][ePawn], 0xFFFFFFFFFFFFFFFF) |
		BB::get_pawn_attacks(eWest, eBlack, boards[eBlack][ePawn], 0xFFFFFFFFFFFFFFFF);
	for (u8 p = 2; p <= eKing; p++) {
		u64 attackers = boards[side][p];
		u64 all_occ = boards[eBlack][0] | boards[eWhite][0];
		u64 our_occ = boards[side][0];
		unsigned long from;
		while (attackers) {
			BB::bitscan_reset(from, attackers);
			u64 targets = 0;
			switch (p) {
				case eKnight: targets = BB::knight_attacks[from]; break;
				case eBishop: targets = BB::get_bishop_attacks(from, all_occ); break;
				case eRook: targets = BB::get_rook_attacks(from, all_occ); break;
				case eQueen: targets = BB::get_queen_attacks(from, all_occ); break;
				case eKing: targets = BB::king_attacks[from]; break;
				default: break;
			}
			//mobility += BB::popcnt(targets & ~our_occ);
			//extra points for captures
			eval_c.mobility[p - 2] += side == eWhite ? BB::popcnt(targets & ~all_occ & ~b_pawn_defenders) : -BB::popcnt(targets & ~all_occ & ~w_pawn_defenders);
			eval_c.captures[p - 2] += side == eWhite ? BB::popcnt(targets & boards[!side][0] & ~b_pawn_defenders) : -BB::popcnt(targets & boards[!side][0] & ~w_pawn_defenders);
			mobility += BB::popcnt(targets & boards[!side][0] & (side == eWhite ? ~b_pawn_defenders : ~w_pawn_defenders));
		}
	}
	return mobility;
}

int Board::evalUpdate()  {
	int out = 0;
	eval_c = EvalCounts();
	//tempo
	eval_c.tempo = us ? -1 : 1;

	int16_t game_phase = getPhase();

	int mg_val = 0;
	int eg_val = 0;

	auto eval_pst = [&](int color, int sign) {
		u64 pieces = boards[color][0];
		unsigned long sq;
		while (pieces) {
			BB::bitscan_reset(sq, pieces);
			u8 p = mailbox[sq];
			u8 idx = (color == eWhite) ? (sq ^ 56) : sq; 
			out += sign * S(mg_table[p][idx], eg_table[p][idx]);
		}
	};
	eval_pst(eWhite, +1);
	eval_pst(eBlack, -1);

	u64 w_front_spans = 0;
	u64 b_front_spans = 0;

	for (int file = 0; file < 8; file++) {

		u64 white_pawns = boards[eWhite][ePawn] & BB::files[file];
		u64 black_pawns = boards[eBlack][ePawn] & BB::files[file];

		u64 white_neighbors = 0, black_neighbors = 0;

		white_neighbors |= boards[eWhite][ePawn] & BB::neighbor_files[file];
		black_neighbors |= boards[eBlack][ePawn] & BB::neighbor_files[file];
		
		// Count isolated pawns for this file
		eval_c.isolated_pawns += BB::popcnt(white_pawns & ~white_neighbors);
		eval_c.isolated_pawns -= BB::popcnt(black_pawns & ~black_neighbors);

		//count doubled 
		eval_c.doubled_pawns +=
		(int(BB::popcnt(white_pawns) >= 2) -
			int(BB::popcnt(white_pawns) >= 2));
		//count passed
		
		while (white_pawns) {
			unsigned long at = 0;
			BB::bitscan_reset(at, white_pawns);
			w_front_spans |= BB::front_spans[eWhite][at];
		}
		while (black_pawns) {
			unsigned long at = 0;
			BB::bitscan_reset(at, black_pawns);
			b_front_spans |= BB::front_spans[eBlack][at];
		}
	}
	eval_c.passed_pawns += BB::popcnt(boards[eWhite][ePawn] & ~b_front_spans);
	eval_c.passed_pawns -= BB::popcnt(boards[eBlack][ePawn] & ~w_front_spans);

	

	u64 occ = boards[eBlack][0] | boards[eWhite][0];
	//count defenders
	u64 w_east_defenders = BB::get_pawn_attacks(eEast, eWhite, boards[eWhite][ePawn], boards[eWhite][0]);
	u64 w_west_defenders = BB::get_pawn_attacks(eWest, eWhite, boards[eWhite][ePawn], boards[eWhite][0]);
	u64 b_east_defenders = BB::get_pawn_attacks(eEast, eBlack, boards[eBlack][ePawn], boards[eBlack][0]);
	u64 b_west_defenders = BB::get_pawn_attacks(eWest, eBlack, boards[eBlack][ePawn], boards[eBlack][0]);
        
	//single defenders
	eval_c.defender_pawns = (BB::popcnt(w_east_defenders | w_west_defenders) - BB::popcnt(b_east_defenders | b_west_defenders));
	//double defenders
	eval_c.double_defender_pawns = (BB::popcnt(w_east_defenders & w_west_defenders) - BB::popcnt(b_east_defenders & b_west_defenders));


	//bishop pair
	eval_c.bishop_pair += (BB::popcnt(boards[eWhite][eBishop]) == 2);
	eval_c.bishop_pair -= (BB::popcnt(boards[eBlack][eBishop]) == 2);

	int white_king_sq = BB::bitscan(boards[eWhite][eKing]);
	int black_king_sq = BB::bitscan(boards[eBlack][eKing]);

	

	
	auto king_safety = [&](int sq, bool side) {
		u64 king_acc = BB::king_attacks[sq] & ~boards[side][0];
		king_acc |= ~getOccupancy() & ((king_acc | BB::set_bit[sq]) << (side ? -8 : 8));
		//std::cout << BB::to_string(king_acc) << std::endl;
		king_acc |= ~getOccupancy() & ((king_acc | BB::set_bit[sq]) << (side ? -8 : 8));
		//std::cout << BB::to_string(king_acc) << std::endl;
		//king_acc |= ~getOccupancy() & ((king_acc | BB::set_bit[sq]) << (side ? -8 : 8));
		//std::cout << BB::to_string(king_acc) << std::endl;
		return (king_acc) & ~boards[side][0];
	};
	static const int SafetyTable[100] = {
		0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
		18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
		68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
		140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
		260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
		377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
		494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500, 500, 500
	};

	auto king_attack_val = [&](bool side) {
		int king_sq = BB::bitscan(boards[side][eKing]);
		u64 occ = getOccupancy();
		u64 knights = boards[!side][eKnight];
		u64 bishops = boards[!side][eBishop];
		u64 rooks = boards[!side][eRook];
		u64 queens = boards[!side][eQueen];
		u64 king_zone = king_safety(king_sq, side);
		unsigned long at = 0;
		int attack_val = 0;

		if (side == eBlack) {
			attack_val += 2 * BB::popcnt(boards[eWhite][ePawn] & king_zone);
		} else {
			attack_val += 2 * BB::popcnt(boards[eBlack][ePawn] & king_zone);
		}
		while (knights) {
			BB::bitscan_reset(at, knights);
			attack_val += 2 * BB::popcnt(BB::knight_attacks[at] & king_zone);
		}
		at = 0;
		while (bishops) {
			BB::bitscan_reset(at, bishops);
			attack_val += 3 * BB::popcnt(BB::get_bishop_attacks(at, occ) & king_zone);
		}
		at = 0;
		while (rooks) {
			BB::bitscan_reset(at, rooks);
			attack_val += 4 * BB::popcnt(BB::get_rook_attacks(at, occ) & king_zone);
		}
		at = 0;
		while (queens) {
			BB::bitscan_reset(at, queens);
			attack_val += 5 * BB::popcnt(BB::get_queen_attacks(at, occ) & king_zone);
		}

		int danger = (SafetyTable[std::min(attack_val, 100)]);
		return danger;
	};
	//printBoard();

	//std::cout << BB::to_string(king_safety(BB::bitscan(boards[eWhite][eKing]), eWhite));
	//get attacks, ignoring our pieces
	int w_attacks = king_attack_val(eWhite);
	int b_attacks = king_attack_val(eBlack);
	out -= S(w_attacks,0);
	out += S(b_attacks,0);
	


		
	getMobility(eWhite);
	getMobility(eBlack);
	//double defenders
	//out += var * (BB::popcnt(w_east_defenders & w_west_defenders) - BB::popcnt(b_east_defenders & b_west_defenders));
	//out = us == eWhite ? out : -out;
	//int i = 0;

	for (int i = 0; i < sizeof(EvalCounts) / 4; i++) {
		out += S(reinterpret_cast<const i32*>(&eval_c)[i] * MG_SCORE(reinterpret_cast<const i32*>(&params)[i]),
				reinterpret_cast<const i32*>(&eval_c)[i] * EG_SCORE(reinterpret_cast<const i32*>(&params)[i]));
	}

	out = ((24 - game_phase) * MG_SCORE(out)) / 24  + (game_phase * EG_SCORE(out)) / 24;
	return out;
}
/*
int Board::staticExchangeEvaluation(Move move, int threshold) {

	int from, to, type, colour, balance, nextVictim;
	uint64_t bishops, rooks, occupied, attackers, myAttackers;

	// Unpack move information
	from = move.from();
	to = move.to();
	type = move.piece();

	// Next victim is moved piece or promotion type
	nextVictim = !move.promotion()
		? mailbox[from]
		: move.promotion();

	// Balance is the value of the move minus threshold. Function
	// call takes care for Enpass, Promotion and Castling moves.
	balance = moveEstimatedValue(board, move) - threshold;

	// Best case still fails to beat the threshold
	if (balance < 0) return 0;

	// Worst case is losing the moved piece
	balance -= see_piece_vals[nextVictim];

	// If the balance is positive even if losing the moved piece,
	// the exchange is guaranteed to beat the threshold.
	if (balance >= 0) return 1;

	// Grab sliders for updating revealed attackers
	bishops = boards[us][eBishop] | boards[us][eQueen];
	rooks = boards[us][eRook] | boards[us][eBishop];

	// Let occupied suppose that the move was actually made
	occupied = (board->colours[WHITE] | board->colours[BLACK]);
	occupied = (occupied ^ (1ull << from)) | (1ull << to);
	if (type == ENPASS_MOVE) occupied ^= (1ull << board->epSquare);

	// Get all pieces which attack the target square. And with occupied
	// so that we do not let the same piece attack twice
	attackers = getAttackers(to, occupied);

	// Now our opponents turn to recapture
	colour = !us;

	while (1) {

		// If we have no more attackers left we lose
		myAttackers = attackers & board->colours[colour];
		if (myAttackers == 0ull) break;

		// Find our weakest piece to attack with
		for (nextVictim = PAWN; nextVictim <= QUEEN; nextVictim++)
			if (myAttackers & board->pieces[nextVictim])
				break;

		// Remove this attacker from the occupied
		occupied ^= (1ull << getlsb(myAttackers & board->pieces[nextVictim]));

		// A diagonal move may reveal bishop or queen attackers
		if (nextVictim == PAWN || nextVictim == BISHOP || nextVictim == QUEEN)
			attackers |= bishopAttacks(to, occupied) & bishops;

		// A vertical or horizontal move may reveal rook or queen attackers
		if (nextVictim == ROOK || nextVictim == QUEEN)
			attackers |= rookAttacks(to, occupied) & rooks;

		// Make sure we did not add any already used attacks
		attackers &= occupied;

		// Swap the turn
		colour = !colour;

		// Negamax the balance and add the value of the next victim
		balance = -balance - 1 - SEEPieceValues[nextVictim];

		// If the balance is non negative after giving away our piece then we win
		if (balance >= 0) {

			// As a slide speed up for move legality checking, if our last attacking
			// piece is a king, and our opponent still has attackers, then we've
			// lost as the move we followed would be illegal
			if (nextVictim == KING && (attackers & board->colours[colour]))
				colour = !colour;

			break;
		}
	}

	// Side to move after the loop loses
	return board->turn != colour;
}
*/