#pragma once

#include "xboard.hpp"

//#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS true
//#include <tbb/concurrent_map.h>

#include <condition_variable>
#include <optional>
#include <variant>
#include <mutex>

#include <boost/asio/thread_pool.hpp>

namespace aunty_sue {
  class sue : public XBoardEngine {
  private:
    using leaf_t = double;

    struct brain_t {
      std::atomic<bool> thinking = false;
      boost::asio::thread_pool pool;
      std::condition_variable stopping_condvar;
      std::mutex stopping_mutex;

      inline bool init() {
        return !thinking.exchange(true);
      }
      void stop();
      void wait();
    };

    struct node_t {
      std::map<move_t, std::unique_ptr<node_t>> responses;
      leaf_t weight;
      board_t board;
      bool is_white;
      game_state state = game_state::Unknown;

      using thought_t = std::pair<node_t*, decltype(responses)::iterator>;

      std::optional<thought_t> find_best_response(move_t);

      void update_moves();

      void update_state() {
        state = get_board_state(board);
      }

      void quick_eval();
      void process(brain_t& brain);
      void evaluate();

      inline void add_move(move_t m) {
        auto& node = *responses.emplace(m, std::make_unique<node_t>(board, !is_white)).first->second;
        auto& from = node.board[m.first.first][m.first.second];
        auto& to = node.board[m.second.first][m.second.second];
        to = static_cast<piece_t>(from | HAS_MOVED);
        from = EmptySquare;
      }

      inline node_t(decltype(board) board_, bool white_side) :
        board{std::move(board_)}, is_white{white_side} {
        update_state();
      }
    };

  private:
    brain_t brain;
    std::unique_ptr<node_t> root;

  public:
    void reset() override {}
    void start() override;
    void stop() override {
      brain.stop();
    }
    inline void new_game(bool is_white, board_t b) override {
      root = std::make_unique<node_t>(std::move(b), !is_white);
      start();
    }
    move_t respond(move_t move) override;
  };
}
