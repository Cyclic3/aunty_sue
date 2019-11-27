#pragma once

#include <array>
#include <iostream>

namespace aunty_sue {
  using coords_t = std::pair<int8_t, int8_t>;
  using move_t = std::pair<coords_t, coords_t>;

  constexpr bool validate_coords(coords_t c) {
    return c.first < 8 && c.second < 8;
  }

  constexpr std::array<char, 4> move2str(move_t move) {
    constexpr std::array<char, 8> files = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    constexpr std::array<char, 8> ranks = {'1', '2', '3', '4', '5', '6', '7', '8'};
    return {
      files.at(move.first.second), ranks.at(move.first.first),
      files.at(move.second.second), ranks.at(move.second.first),
    };
  }

  constexpr move_t str2move(std::string_view s) {
    if (s.size() != 4)
      throw std::invalid_argument("A move must be formed of 4 letters");

    move_t candidate = {
      {static_cast<uint8_t>(s[1] - '1'), static_cast<uint8_t>(s[0] - 'a')},
      {static_cast<uint8_t>(s[3] - '1'), static_cast<uint8_t>(s[2] - 'a')},
    };

    if (!validate_coords(candidate.first) || !validate_coords(candidate.second))
      throw std::invalid_argument("Bad coordinates parsed");

    return candidate;
  }

  enum piece_t : uint16_t {
    EmptySquare = 0,
    Rook = 1, Knight = 2, Bishop = 4, Queen = 8, King = 16, Pawn = 32,
    BLACK_SIDE = 64,
    WHITE_SIDE = 128,
    HAS_MOVED = 256,

    PIECE_TYPE_MASK = Rook|Knight|Bishop|Queen|King|Pawn
  };

  constexpr piece_t black(piece_t p) { return static_cast<piece_t>(BLACK_SIDE | p); }
  constexpr piece_t white(piece_t p) { return static_cast<piece_t>(WHITE_SIDE | p); }

  using row_t = std::array<piece_t, 8>;
  using board_t = std::array<row_t, 8>;

  constexpr board_t default_board = board_t{
    row_t{ white(Rook), white(Knight), white(Bishop), white(King), white(Queen), white(Bishop), white(Knight), white(Rook) },
    row_t{ white(Pawn), white(Pawn), white(Pawn), white(Pawn), white(Pawn), white(Pawn), white(Pawn), white(Pawn) },
    row_t{ EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare },
    row_t{ EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare },
    row_t{ EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare },
    row_t{ EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare, EmptySquare },
    row_t{ black(Pawn), black(Pawn), black(Pawn), black(Pawn), black(Pawn), black(Pawn), black(Pawn), black(Pawn) },
    row_t{ black(Rook), black(Knight), black(Bishop), black(King), black(Queen), black(Bishop), black(Knight), black(Rook) },
  };

  enum class game_state {
    BlackWins,
    WhiteWins,
    Draw,
    InProgress,
    /// Used as an intermediate result for state calculations
    NotAWin,
    Unknown
  };

  inline game_state get_board_state(board_t& board) {
    int white_remains = false, black_remains = false;

    // Check if we ran out of pieces
    for (auto& rank : board) {
      for (auto& square : rank) {
        if (square & WHITE_SIDE)
          white_remains = true;
        else if (square & BLACK_SIDE)
          black_remains = true;
      }
    }

    if (white_remains) {
      if (black_remains)
        return game_state::NotAWin;
      else
        return game_state::BlackWins;
    }
    else
      return game_state::WhiteWins;
  }

  struct illegal_move : public std::exception {
    const char* what() const noexcept override { return "The opponent made an illegal move"; }
  };
  struct game_over : public std::exception {
    game_state final_state;
    const char* what() const noexcept override {
      switch(final_state) {
        case game_state::BlackWins: return "Black won the game";
        case game_state::WhiteWins: return "White won the game";
        case game_state::Draw : return "The game was a draw";
        default: abort();
      }
    }

    inline game_over(game_state state) : final_state{state} {}
  };

  struct XBoardEngine {
    virtual void reset() = 0;
    /// May be called multiple times without a stop
    virtual void start() = 0;
    virtual void stop() = 0;
    /// Should call start
    virtual void new_game(bool is_white, board_t b = default_board) = 0;

    /// Must throw illegal_move if the move is, well, illegal
    ///
    /// Must throw we_lost if the engine has lost
    virtual move_t respond(move_t) = 0;

    virtual ~XBoardEngine() = default;
  };

  void run_engine(XBoardEngine& eng, std::istream& in = std::cin, std::ostream& out = std::cout);

  extern std::ofstream fs;
}
