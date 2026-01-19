#pragma once

#include "Board.h"
#include "Memory.h"
#include "Misc.h"
#include <algorithm>
#include <climits>
#include <ctime>
#include <fstream>
#include <unordered_map>

int constexpr good_cap_cutoff = -16000;
static constexpr int MAX_PLY = 128;

enum class TType : u8 { INVALID, EXACT, FAIL_LOW, BETA_CUT, BEST };

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
  u64 hash_size = 16;
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
  std::array<Move, 2> killers = {Move(0, 0), Move(0, 0)};
  void clear() {
    moves.clear();
    seen_quiets.clear();
    seen_noisies.clear();
    static_eval = 0;
    improving_rate = 0;
    improving = false;
    in_check = false;
    current_move = Move(0, 0);
  }
};

class Engine {

  SearchStack search_stack[MAX_PLY];
  UciOptions uci_options;

  int hash_count = 0;
  int hash_miss = 0;
  // Move best_move;

  std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
  std::array<int, MAX_PLY> pv_length;
  std::vector<TTEntry> tt;

  // Engine state variables
  int max_depth = 0;
  int sel_depth = 0;

  Move root_best = Move(0, 0);
  Move expected_response = Move(0, 0);

  // Timer variables
  std::clock_t start_time = 0;
  int max_time = 0;
  std::vector<PerfT> perf_values;
  int pos_count = 0;
  bool do_bench = false;

  void perftSearch(int depth);
  [[nodiscard]] int alphaBeta(int alpha, int beta, int depth_left,
                              bool cut_node, SearchStack *ss);
  [[nodiscard]] int quiesce(int alpha, int beta, bool cut_node,
                            SearchStack *ss);

public:
  std::array<std::array<std::array<int, 64>, 64>, 2> history_table;
  std::array<std::array<std::array<std::array<int, 64>, 7>, 7>, 2>
      capture_history;
  int nodes = 0;
  int hash_hits = 0;
  int start_ply = 0;
  bool time_over = false;
  Board b = Board();
  TimeControl tc;

  Engine(UciOptions options);

  void reset();

  [[nodiscard]] std::vector<PerfT> doPerftSearch(int depth);
  [[nodiscard]] std::vector<PerfT> doPerftSearch(std::string position,
                                                 int depth);

  void setBoardFEN(std::istringstream &fen);
  void setBoardUCI(std::istringstream &uci);

  void initSearch();
  void bench();

  Move search(int depth);
  [[nodiscard]] std::vector<Move> getPrincipalVariation() const;

  [[nodiscard]] TTEntry probeTT(u64 hash_key) const;
  void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left,
                    Move best);

  [[nodiscard]] bool checkTime(bool strict);
  void calcTime();
  void printPV(int score);
  void updatePV(int depth, Move move);
  void updateQuietHistory(SearchStack *ss, int depth_left);
  void updateNoisyHistory(SearchStack *ss, int depth_left);
};

enum class MoveStage {
  ttMove,
  promotion,
  good_captures,
  killer,
  bad_captures,
  history
};

class MovePick {
  int killer_slot = 0;

public:
  MoveStage stage = MoveStage::ttMove;
  Move getNext(Engine &e, Board &b, SearchStack *ss, int threshold) {
    Move out = Move(0, 0);

    do {

      if (ss->moves.empty()) {
        return Move(0, 0);
      }

      TTEntry entry;

      switch (stage) {
      case MoveStage::ttMove:
        entry = e.probeTT(e.b.getHash());
        if (entry && entry.type != TType::FAIL_LOW) {
          auto pos_best =
              std::find(ss->moves.begin(), ss->moves.end(), entry.best_move);
          if (pos_best != ss->moves.end() && b.isLegal(*pos_best)) {
            out = *pos_best;
            e.hash_hits++;
            *pos_best = ss->moves.back();
            ss->moves.pop_back();
            stage = MoveStage::good_captures;
            break;
          }
        }
        stage = MoveStage::good_captures;
        [[fallthrough]];
      case MoveStage::good_captures: {
        int max = -100000;
        int index = 0;

        for (int i = 0; i < ss->moves.size(); i++) {
          if (ss->moves[i].captured() || ss->moves[i].promotion()) {
            // piece_vals[moves[i].promotion()] * 256 +
            Move m = ss->moves[i];
            int val =
                see_piece_vals[m.promotion()] +
                (m.captured() ? see_piece_vals[m.captured()] * 8 +
                                    e.capture_history[b.us][m.piece()]
                                                     [m.captured()][m.to()]
                              : 0);
            if (val > max &&
                b.staticExchangeEvaluation(ss->moves[i], threshold)) {
              max = val;
              index = i;
            }
          }
        }
        if (max != -100000) {
          out = ss->moves[index];
          ss->moves[index] = ss->moves.back();
          ss->moves.pop_back();
          break;
        }
      }
        stage = MoveStage::killer;
        [[fallthrough]];

      case MoveStage::killer:
        if ((b.ply - e.start_ply > 2) && killer_slot < 2 &&
            (ss - 2)->killers[killer_slot]) {
          Move killer = (ss - 2)->killers[killer_slot++];
          auto pos_best = std::find(ss->moves.begin(), ss->moves.end(), killer);
          if (pos_best != ss->moves.end() && b.isLegal(*pos_best)) {
            /*
            if (!b.isLegal(*pos_best)) {
                    std::cout << "test";
            }*/
            out = *pos_best;
            *pos_best = ss->moves.back();
            ss->moves.pop_back();
            break;
          }
        }
        stage = MoveStage::bad_captures;
        [[fallthrough]];

      case MoveStage::bad_captures: {
        int max = INT_MIN;
        int index = 0;

        for (int i = 0; i < ss->moves.size(); i++) {
          if (ss->moves[i].captured() || ss->moves[i].promotion()) {
            // piece_vals[moves[i].promotion()] * 256 +
            Move m = ss->moves[i];
            int val =
                see_piece_vals[m.promotion()] +
                (m.captured() ? see_piece_vals[m.captured()] * 8 +
                                    e.capture_history[b.us][m.piece()]
                                                     [m.captured()][m.to()]
                              : 0);
            if (val > max) {
              max = val;
              index = i;
            }
          }
        }
        if (max != INT_MIN) {
          out = ss->moves[index];
          ss->moves[index] = ss->moves.back();
          ss->moves.pop_back();
          break;
        }
      }
        stage = MoveStage::history;
        [[fallthrough]];

      case MoveStage::history: {
        int max = INT_MIN;
        int index = 0;

        for (int i = 0; i < ss->moves.size(); i++) {
          int val =
              e.history_table[b.us][ss->moves[i].from()][ss->moves[i].to()];
          if (val > max) {
            max = val;
            index = i;
          }
        }
        out = ss->moves[index];
        ss->moves[index] = ss->moves.back();
        ss->moves.pop_back();
        break;
      }
      }
    } while (!b.isLegal(out));
    return out;
  }
};
