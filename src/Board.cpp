#include "Board.h"


Board::Board() {
	state_stack.reserve(512);
	reset();
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
		if (reinterpret_cast<const u64*>(&piece_board)[i] != reinterpret_cast<const u64*>(&piece_board)[i])
			return false;
	}
	if (ply != other.ply ||
		us != other.us ||
		castle_flags != other.castle_flags ||
		ep_square != other.ep_square ||
		state_stack != other.state_stack ||
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
		boards[us][p] &= ~BB::set_bit(move.to());
		boards[us][move.promotion()] |= BB::set_bit(move.to());
		piece_board[move.to()] = move.promotion();
	}

	// Handle en passant  
	if (move.isEnPassant()) {
		u8 ep_capture_square = move.to() + (us == eWhite ? -8 : 8);
		boards[!us][ePawn] &= ~BB::set_bit(ep_capture_square);
		boards[!us][0] &= ~BB::set_bit(ep_capture_square);
		piece_board[ep_capture_square] = eNone;
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
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
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

				if (pieceChar) {
					WriteConsoleW(hOut, pieceChar, wcslen(pieceChar), nullptr, nullptr);
					break;
				}
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
	if (color == eSideNone) {
		printBitBoards();
		printBoard();
		undoMove();
		printBitBoards();
		printBoard();
		throw std::logic_error("weird");
	}

	boards[color][piece_board[from]] ^= BB::set_bit(from);
	boards[color][0] ^= BB::set_bit(from);
	boards[color][piece_board[from]] |= BB::set_bit(to);
	boards[color][0] |= BB::set_bit(to);
	//clear enemy bit
	boards[!color][piece_board[to]] &= ~BB::set_bit(to);
	boards[!color][0] &= ~BB::set_bit(to);

	piece_board[to] = piece_board[from];
	piece_board[from] = eNone;
}

void Board::setPiece(u8 square, u8 color, u8 piece) {
	boards[color][piece] |= BB::set_bit(square);
	boards[color][0] |= BB::set_bit(square);
	piece_board[square] = piece;
}

void Board::removePiece(u8 square) {
	Side color = getSide(square);
	if (color != eSideNone) {
		boards[color][piece_board[square]] &= ~BB::set_bit(square);
		boards[color][0] &= ~BB::set_bit(square);
	}

	piece_board[square] = eNone;
}

Side Board::getSide(int square) const {
	return boards[eWhite][0] & BB::set_bit(square)
		? eWhite
		: (boards[eBlack][0] & BB::set_bit(square) ? eBlack : eSideNone);
}

void Board::loadFen(std::istringstream& fen_stream) {
	reset();
	// Clear all bitboards and piece_board
	for (int side = 0; side < 2; ++side)
		for (int piece = 0; piece < 7; ++piece)
			boards[side][piece] = 0;
	for (int i = 0; i < 64; ++i)
		piece_board[i] = eNone;

	std::string board_part, active_color, castling, ep, halfmove, fullmove;
	fen_stream >> board_part >> active_color >> castling >> ep >> halfmove >> fullmove;

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
				boards[color][piece] |= BB::set_bit(square);
				boards[color][0] |= BB::set_bit(square);
				piece_board[square] = piece;
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

	if (!fullmove.empty())
		ply = std::stoi(fullmove);

	if (!halfmove.empty())
		half_move = std::stoi(halfmove);

	// Recompute occupancy
	setOccupancy();
	hash = calcHash();
	eval = evalFullUpdate();
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
	throw std::logic_error("invalid move!");
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
				if (calcHash() != hash) {
					throw std::logic_error("hashing error");
				}
			}
			else {
				// Invalid move, stop processing further
				break;
			}
		}
	}
	setOccupancy();
	hash = calcHash();
	eval = evalFullUpdate();
	runSanityChecks();
}

Move Board::moveFromSan(const std::string& san) {
	std::string move = san;

	StaticVector<Move> moves;
	genPseudoLegalMoves(moves);
	filterToLegal(moves);
	// Remove check/mate symbols
	if (!move.empty() && (move.back() == '+' || move.back() == '#'))
		move.pop_back();


	// Handle castling
	if (move == "O-O" || move == "0-0") {
		for (auto& m : moves) {
			if (piece_board[m.from()] == eKing && ((int)m.to() - (int)m.from()) == 2)
				return m;
		}
		throw std::invalid_argument("No legal king-side castling move found");
	}
	if (move == "O-O-O" || move == "0-0-0") {
		for (auto& m : moves) {
			if (piece_board[m.from()] == eKing && ((int)m.to() - (int)m.from()) == -2)
				return m;
		}
		throw std::invalid_argument("No legal queen-side castling move found");
	}

	// Parse promotion
	char promotion = '\0';
	size_t eq = move.find('=');
	if (eq != std::string::npos && eq + 1 < move.size()) {
		promotion = move[eq + 1];
		move = move.substr(0, eq);
	}

	// Identify piece
	int piece = ePawn;
	size_t idx = 0;
	if (move[0] >= 'A' && move[0] <= 'Z' && move[0] != 'O') {
		switch (move[0]) {
		case 'N': piece = eKnight;
			break;
		case 'B': piece = eBishop;
			break;
		case 'R': piece = eRook;
			break;
		case 'Q': piece = eQueen;
			break;
		case 'K': piece = eKing;
			break;
		default: throw std::invalid_argument("Unknown piece in SAN");
		}
		idx = 1;
	}

	// Find destination square (always last two characters)
	if (move.size() < idx + 2)
		throw std::invalid_argument("Invalid SAN: missing destination square");
	int to_file = move[move.size() - 2] - 'a';
	int to_rank = move[move.size() - 1] - '1';
	int to_square = to_file | (to_rank << 3);

	// Parse disambiguation and capture between idx and move.size()-2
	char disambig_file = 0, disambig_rank = 0;
	bool is_capture = false;
	for (size_t i = idx; i < move.size() - 2; ++i) {
		if (move[i] == 'x') {
			is_capture = true;
		}
		else if (move[i] >= 'a' && move[i] <= 'h') {
			disambig_file = move[i];
		}
		else if (move[i] >= '1' && move[i] <= '8') {
			disambig_rank = move[i];
		}
	}

	// Find matching legal move
	for (const auto& m : moves) {
		if (m.to() != to_square) continue;
		if (m.piece() != piece) continue;
		if (promotion) {
			int promo_piece = eNone;
			switch (promotion) {
			case 'N': promo_piece = eKnight;
				break;
			case 'B': promo_piece = eBishop;
				break;
			case 'R': promo_piece = eRook;
				break;
			case 'Q': promo_piece = eQueen;
				break;
			default: continue;
			}
			if (m.promotion() != promo_piece) continue;
		}
		if (is_capture && m.captured() == eNone) continue;
		if (!is_capture && m.captured() != eNone) continue;
		if (disambig_file && ((m.from() & 7) != (disambig_file - 'a'))) continue;
		if (disambig_rank && ((m.from() >> 3) != (disambig_rank - '1'))) continue;
		return m;
	}
	return Move();
	printBitBoards();
	std::cout << boardString() << "\n";
	std::cout << ep_square;
	throw std::invalid_argument("No matching legal move found for SAN: " + san);
}

std::string Board::sanFromMove(Move move) {
	std::string san;
	u8 from = move.from();
	u8 to = move.to();
	u8 piece = move.piece();
	u8 captured = move.captured();
	u8 promotion = move.promotion();

	// Handle castling  
	if (piece == eKing && abs((int)to - (int)from) == 2) {
		if (to > from) {
			return "O-O"; // King-side castling  
		}
		else {
			return "O-O-O"; // Queen-side castling  
		}
	}

	// Determine piece type  
	char piece_char = (piece == ePawn) ? '\0' : " PNBRQK"[piece];

	// Handle pawn moves  
	if (piece == ePawn) {
		if (captured != eNone) {
			san += static_cast<char>('a' + (from & 7)); // File of the pawn  
		}
	}
	else {
		san += piece_char;
	}

	// Handle disambiguation
	StaticVector<Move> moves;
	genPseudoLegalMoves(moves);

	filterToLegal(moves);
	bool fileAmbiguity = false, rankAmbiguity = false;
	for (const auto& m : moves) {
		if (m.to() == to && m.piece() == piece && m.from() != from) {
			if ((m.from() & 7) == (from & 7)) fileAmbiguity = true;
			if ((m.from() >> 3) == (from >> 3)) rankAmbiguity = true;
		}
	}
	if (fileAmbiguity && rankAmbiguity) {
		san += static_cast<char>('a' + (from & 7));
		san += static_cast<char>('1' + (from >> 3));
	}
	else if (fileAmbiguity) {
		san += static_cast<char>('a' + (from & 7));
	}
	else if (rankAmbiguity) {
		san += static_cast<char>('1' + (from >> 3));
	}

	// Handle captures  
	if (captured != eNone) {
		san += 'x';
	}

	// Add destination square  
	san += static_cast<char>('a' + (to & 7));
	san += static_cast<char>('1' + (to >> 3));

	// Handle promotion  
	if (promotion != eNone) {
		san += '=';
		san += "NBRQ"[promotion - eKnight];
	}

	//TODO: handle checkmate
	if (isCheck()) {
		san += '+';
	}
	return san;
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
				moves.emplace_back({ u8(from), u8(to), ePawn, piece_board[to], u8(promo) });
		}
		else {
			moves.emplace_back({u8(from), u8(to), ePawn, piece_board[to]
		});
		}
	}
	while (right_captures) {
		unsigned long to;
		BB::bitscan_reset(to, right_captures);
		int from = to - ((us == eWhite) ? 9 : -7);
		if ((from >> 3) == promo_rank) {
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back({u8(from), u8(to), ePawn, piece_board[to], u8(promo)
		});
		}
		else {
			moves.emplace_back({ u8(from), u8(to), ePawn, piece_board[to] });
		}
	}


	if (ep_square != -1) {
		int ep_from = ep_square + (us == eWhite ? -8 : 8);
		// Left capture
		if ((ep_square & 7) > 0 && (pawns & BB::set_bit(ep_from - 1))) {
			moves.emplace_back({ u8(ep_from - 1), u8(ep_square), ePawn, ePawn, eNone, true });
		}
		// Right capture
		if ((ep_square & 7) < 7 && (pawns & BB::set_bit(ep_from + 1))) {
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
			moves.emplace_back({ u8(from), u8(to), piece, piece_board[to] });
		}
	}
}

void Board::filterToLegal(StaticVector<Move>& moves) {
	//could be made quicker, perhaps only checking pieces between king and attackers
	int new_i = 0;
	//bool current_check = isCheck();
	for (int i = 0; i < moves.size();i++) {
		Move move = moves[i];
		if (move.isCastle()) {
			if (getAttackers((move.to() + move.from()) / 2, us)) {
				//moves.erase(moves.begin() + i);
				continue;
			}
		}
		if (is3fold() || half_move == 100) {
			//moves.erase(moves.begin() + i);
			continue;
		}

		doMove(move);
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

	if (piece_board[move.from()] == move.piece() && piece_board[move.to()] == move.captured()) {
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
	u64 square_mask = BB::set_bit(square);
	u64 our_occ = boards[side][0];
	u64 their_occ = boards[!side][0];
	u64 all_occ = our_occ | their_occ;

	// Check pawns  
	int left_capture = (side == eBlack) ? -7 : 9;
	int right_capture = (side == eBlack) ? -9 : 7;

	if ((square % 8) != 7)
		attackers |= BB::set_bit(square + left_capture) & boards[!side][ePawn];
	if ((square % 8) != 0)
		attackers |= BB::set_bit(square + right_capture) & boards[!side][ePawn];

	// Check knights  
	attackers |= BB::knight_attacks[square] & boards[!side][eKnight];
	//check bishops
	attackers |= BB::get_bishop_attacks(square, all_occ) & ~our_occ & boards[!side][eBishop];

	// Check rooks
	attackers |= BB::get_rook_attacks(square, all_occ) & ~our_occ & boards[!side][eRook];

	//check queen
	attackers |= BB::get_queen_attacks(square, all_occ) & ~our_occ & boards[!side][eQueen];

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
		return evalFullUpdate();
	}

	int16_t game_phase = 24 -
		BB::popcnt(boards[eWhite][eKnight]) -
		BB::popcnt(boards[eWhite][eBishop]) -
		BB::popcnt(boards[eWhite][eRook]) * 2 -
		BB::popcnt(boards[eWhite][eQueen]) * 4 -
		BB::popcnt(boards[eBlack][eKnight]) -
		BB::popcnt(boards[eBlack][eBishop]) -
		BB::popcnt(boards[eBlack][eRook]) * 2 -
		BB::popcnt(boards[eBlack][eQueen]) * 4
		;

	u8 from = move.from();
	u8 to = move.to();
	if (us != eWhite) {
		from ^= 56;
		to ^= 56;
	}

	int mg_val = mg_table[move.piece()][from];
	int eg_val = eg_table[move.piece()][from];
	int old_val = (mg_val + (game_phase * eg_val - mg_val) / 24);


	mg_val = mg_table[move.piece()][to];
	eg_val = eg_table[move.piece()][to];
	int new_val = (mg_val + (game_phase * eg_val - mg_val) / 24);

	int update = new_val - old_val;

	out = eval + ((us == eWhite) ? -update : update);


	int t = evalFullUpdate();

	return out;
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
		piece_board[i] = initial_piece_board[i];
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

bool Board::is3fold() {
	if (state_stack.size() < half_move || state_stack.empty()) return false;
	int counter = 0;
	for (int i = 1; i <= half_move; i++) {
		if (state_stack[state_stack.size() - i].hash == hash) {
			counter++;
		}
	}
	if (counter >= 2) {
		return true;
	}
	return false;
}

u64 Board::calcHash() const {
	u64 out_hash = 0;
	for (int sq = 0; sq < 64; sq++) {
		if (piece_board[sq] != eNone) {
			out_hash ^= z.piece_at[sq * 12 + (piece_board[sq] - 1) + (getSide(sq) * 6)];
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
			hash ^= z.piece_at[((state_stack.end() - 2)->move.to() * 12) + (ePawn - 1) + (us * 6)]; //invert captured piece
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

int Board::getMobility(bool side) const {
	int mobility = 0;
	for (u8 p = 2; p <= eKing; p++) {
		u64 attackers = boards[side][p];
		u64 all_occ = boards[eBlack][0] | boards[eWhite][0];
		u64 our_occ = boards[side][0];
		unsigned long from;
		while (attackers) {
			BB::bitscan_reset(from, attackers);
			u64 targets;
			switch (p) {
			case eKnight: targets = BB::knight_attacks[from]; break;
			case eBishop: targets = BB::get_bishop_attacks(from, all_occ); break;
			case eRook: targets = BB::get_rook_attacks(from, all_occ); break;
			case eQueen: targets = BB::get_queen_attacks(from, all_occ); break;
			case eKing: targets = BB::king_attacks[from]; break;
			}
			//mobility += BB::popcnt(targets & ~our_occ);
			//extra points for captures
			mobility += BB::popcnt(targets & boards[!side][0]);
		}
	}
	return mobility;
}

