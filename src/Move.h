#pragma once
#include <stdexcept>
#include <stdint.h>
#include <string>
#include "Misc.h"

class Move 
{
private:
	uint32_t data = 0;
	//bit 0-5: from
	//bits 6-11: to
	//bits 12-14: piece type
	//bits 15-17: captured piece type
	//bits 18-20: promotion 
	//bit 21: ep flag
	//bit 22-32: rating
public:
	constexpr Move() = default;

	constexpr Move(const u8 from, const u8 to) {
		data |= from & 0x3F;
		data |= (to & 0x3F) << 6;
	};
	constexpr Move(const u8 from, const u8 to, const u8 moved) : Move(from, to) {
		data |= (moved & 0x7) << 12;
	};

	constexpr Move(const u8 from, const u8 to, const u8 moved, const u8 capture) : Move(from, to, moved) {
		data |= (capture & 0x7) << 15;
	};
	
	constexpr Move(const u8 from, const u8 to, const u8 moved, const u8 capture, const u8 promotion) : Move(from, to, moved, capture) {
		data |= (promotion & 0x7) << 18;
	};

	constexpr Move(const u8 from, const u8 to, const u8 moved, const u8 capture, const u8 promotion, const bool ep_flag) : Move(from, to, moved, capture, promotion) {
		data |= static_cast<uint32_t>(ep_flag) << 21;
	};
	
	[[nodiscard]] u8 from() const { return data & 0x3F; }
	[[nodiscard]] u8 to() const { return (data >> 6) & 0x3F; }
	[[nodiscard]] u8 piece() const { return (data >> 12) & 0x7; }
	void setMoved(const u8 piece) { data |= (piece & 0x7) << 12; }

	[[nodiscard]] bool isCastle() const {
		if (piece() == eKing && std::abs(to() - from()) == 2) return true;
		return false;
    }
	
	[[nodiscard]] u8 captured() const { return (data >> 15) & 0x7; }
	void setCaptured(const u8 piece) { data |= (piece & 0x7) << 15; }

	[[nodiscard]] u8 promotion() const { return (data >> 18) & 0x7; }
	void setPromotion(const u8 piece) { data |= (piece & 0x7) << 18; }

	[[nodiscard]] bool isEnPassant() const { return data & (1 << 21); }
	void setEnPassant(const bool ep_flag) { data |= static_cast<uint32_t>(ep_flag) << 21; }

	[[nodiscard]] std::string toUci() const {
		std::string out;
		out += static_cast<char>('a' + (from() & 0x7));
		out += static_cast<char>('1' + ((from() & 0x38) >> 3));
		out += static_cast<char>('a' + (to() & 0x7));
		out += static_cast<char>('1' + ((to() & 0x38) >> 3));
		switch (promotion()) {
			case eKnight: out += "n"; break;
			case eBishop: out += "b"; break;
			case eRook: out += "r"; break;
			case eQueen: out += "q"; break;
			default: break; // No promotion
		}

		return out;
	}

	[[nodiscard]] uint32_t raw() const { return data; }

	[[nodiscard]] constexpr bool operator==(const Move& other) const {
		return data == other.data;
	}

	[[nodiscard]] explicit constexpr operator bool() const {
		return data != 0;
	}

    void setEval(int eval_score) {
	    // Bits 22-31 (10 bits) are used for eval score, signed from -512 to +511
	    // Clamp eval_score to [-512, 511]
	    if (eval_score < -512) eval_score = -512;
	    if (eval_score > 511) eval_score = 511;
	    // Convert to unsigned for storage (two's complement in 10 bits)
	    uint32_t eval_bits = static_cast<uint32_t>(eval_score & 0x3FF);
	    // Clear previous eval bits
	    data &= ~(0x3FFu << 22);
	    // Set new eval bits
	    data |= (eval_bits << 22);
    }
};
