#include "sue.hpp"

#include <thread>
#include <chrono>

#include <fstream>

#include <boost/asio/post.hpp>

using namespace std::chrono_literals;

namespace aunty_sue {
  void sue::brain_t::stop() {
    std::unique_lock lock{stopping_mutex};
    if (thinking) {
      thinking = false;
      stopping_condvar.notify_all();

      pool->stop();
      pool->join();
    }
  }

  void sue::brain_t::wait() {
    std::unique_lock lock{stopping_mutex};
    stopping_condvar.wait(lock, [this]{return !thinking; });
  }

  void sue::node_t::quick_eval() {
    // TODO: actual piecewise calc
    int sum = 0;
    for (auto& rank : board) {
      for (auto& square : rank) {
        if (square & WHITE_SIDE)
          ++sum;
        else if (square & BLACK_SIDE)
          --sum;
      }
    }

    weight = is_white ? -sum : sum;
  }

  void sue::node_t::evaluate() {
    switch (state) {
      case game_state::Draw: {
        weight = 0;
      } return;
      case game_state::WhiteWins: {
        weight = is_white ?
              std::numeric_limits<leaf_t>::infinity() :
              -std::numeric_limits<leaf_t>::infinity();
      } return;
      case game_state::BlackWins: {
        weight = is_white ?
              -std::numeric_limits<leaf_t>::infinity() :
              std::numeric_limits<leaf_t>::infinity();
      } return;
      case game_state::InProgress: {
        // Clear any previous weight calculation
        weight = 0;

        for (auto& i : responses) {
          i.second->evaluate();
          // Subtract, as we don't want them to win!
          //
          // I have messed this up before, to hilarious effect
          weight -= i.second->weight;
        }
      } break;
      // If we have no idea what is going on, perform a quick eval, and return
      default: {
        quick_eval();
      } return;
    }
  }

  void sue::node_t::process(brain_t& brain) {
    if (brain.max_move_seen < half_moves_made) {
      int n = half_moves_made;
      if (brain.max_move_seen.exchange(n) != half_moves_made)
        std::cout << "ply 0 0 " << half_moves_made << " 0" << std::endl;
    }

    // Check if we understand what this state is
    if (state == game_state::NotAWin) {
      update_moves();
      state = (responses.size() == 0) ? game_state::Draw : game_state::InProgress;
    }


    // We don't need to think about a finished game
    if (state != game_state::InProgress)
      return;

    // We don't need to think about a finished game
    if (state != game_state::InProgress)
      return;

    // If we get here, there we are in an in-progress gam

    // If we need to stop, quickly terminate
    if (!brain.thinking) // The best line of code I have ever written
      return;

    // Perform a preliminary analysis, so we can optimise with a
    for (auto& i : responses) {
      if (i.second->weight != i.second->weight)
        i.second->quick_eval();
    }

    // TODO: proper breadth-first seach, prioritising good moves, with a curve on depth spent
    for (auto& i : responses) {
      auto* possibility = i.second.get();
      boost::asio::post(*brain.pool, [possibility, &brain] {
        possibility->process(brain);
      });
    }
  }

  void sue::node_t::update_moves() {
    auto side_mask = is_white ? WHITE_SIDE : BLACK_SIDE;
    auto enemy_mask = is_white ? BLACK_SIDE : WHITE_SIDE;

    int8_t pawn_promote_rank = is_white ? (8 - 1) : (1 - 1);
    int8_t pawn_precomp_two_rank = is_white ? (4 - 1) : (5 - 1);

    bool can_take = false;

    for (int8_t rank = 0; rank < 8; ++rank) {
      for (int8_t file = 0; file < 8; ++file) {
        auto& square = board[rank][file];
        if (square & enemy_mask)
          continue;

        coords_t from = {rank, file};
        auto p = static_cast<piece_t>(square & PIECE_TYPE_MASK);

        switch (square & PIECE_TYPE_MASK) {
          case Pawn: {
            if (rank == pawn_promote_rank)
              continue; //TODO
//              abort(); // TODO

            uint8_t forward_rank = is_white ? (rank + 1) : (rank - 1);

            coords_t one_target = {forward_rank, file};
            // If we can't move one, we can't move two
            if (!can_take && board[one_target.first][one_target.second] == EmptySquare) {
              add_move({from, one_target});

              coords_t two_target = {pawn_precomp_two_rank, file};
              if (!(square & HAS_MOVED) && board[two_target.first][two_target.second] == EmptySquare)
                add_move(move_t{from, two_target});
            }

            {
              coords_t left_target = {forward_rank, file - 1};
              if (file != 0 && (board[left_target.first][left_target.second] & enemy_mask)) {
                if (!can_take)
                  responses.clear();
                can_take = true;
                add_move(move_t{from, left_target});
              }
            }

            {
              coords_t right_target = {forward_rank, file + 1};
              if (file != 7 && (board[right_target.first][right_target.second] & enemy_mask)) {
                if (!can_take)
                  responses.clear();
                can_take = true;
                add_move(move_t{from, right_target});
              }
            }
          } break;
        }
      }
    }
  }

  std::optional<sue::node_t::thought_t> sue::node_t::find_best_response(move_t move) {
    switch(state) {
      // Check if the game is over
      case game_state::InProgress: break;
      //If we haven't looked into this move, we cannot find a valid, let alone good, response move
      //
      // Ask for more time
      case game_state::Unknown: return std::nullopt;
      default: throw game_over{state};
    }

    // Look for our move in the table
    auto iter = responses.find(move);

    // If we haven't seen this move, then we are even more boned.
    //
    // Since we are of course infallable, this must be an illegal move.
    if (iter == responses.end())
      throw illegal_move{};
    auto& choices = iter->second->responses;

    // The game state check proves that we have at least one candidate choice
    auto current_best = choices.begin();

    auto choice_iter = current_best;
    ++choice_iter;

    for (; choice_iter != choices.end(); ++choice_iter) {
      auto s = choice_iter->second->state;
      switch (choice_iter->second->state) {
        case game_state::WhiteWins: {
          // If this move loses us the game, then the current best cannot be any worse
          if (!is_white)
            continue;
          // Alternatively, if this wins us the game, then the current best cannot be any better
          else
            current_best = iter;
        } break;
        case game_state::BlackWins: {
          // If this move loses us the game, then the current best cannot be any worse
          if (is_white)
            continue;
          // Alternatively, if this wins us the game, then the current best cannot be any better
          else
            current_best = iter;
        } break;
        case game_state::Draw: {
          // A draw is worth going for if we are losing,
          // or we do not know what the current best choice entails (NaN returns false)
          if (current_best->second->weight < 0)
            current_best = choice_iter;
        } break;
        case game_state::InProgress: {
          // We go for a "devil you know" strategy.
          //
          // As such, this comparison will return false for a NaN,
          // meaning that we skip any uncomputed state
          if (current_best->second->weight < choice_iter->second->weight)
            current_best = choice_iter;
        } break;

        case game_state::NotAWin: {
          return std::nullopt;
        } break;

        default: throw std::logic_error{"Something terrible happened with game_state..."};
      }
    }

    return sue::node_t::thought_t{iter->second.get(), current_best};
  }

  void sue::start() {
    if (brain.init())
      boost::asio::post(*brain.pool, [this] {
        root->process(brain);
      });
  }

  move_t sue::respond(move_t move) {
    stop();
    std::optional<sue::node_t::thought_t> res;
    while (!(res = root->find_best_response(move))) {
      // Give it another chance
      start();
      std::this_thread::sleep_for(100ms);
      stop();
    }

    // This is faster
    auto new_node = res->first->responses.extract(res->second);
    root = std::move(new_node.mapped());

    start();
    return new_node.key();
  }
}
