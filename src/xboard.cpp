#include "xboard.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace aunty_sue {
  enum class xboard_verb {
    Xboard,
    ProtoVer,
    Accepted,
    Rejected,
    New,
    Variant,
    Quit,
    Random,
    Force,
    Go,
    PlayOther,
    White,
    Black,
    Level,
    St,
    Sd,
    Nps,
    Time,
    OTim,
    UserMove,
    MoveNow,
    Result,
    SetBoard,
    Edit,
    Hint,
    Bk,
    Undo,
    Remove,
    Hard,
    Easy,
    Post,
    NoPost,
    Analyze,
    Name,
    Rating,
    Ics,
    Computer,
    Pause,
    Resume,
    Memory,
    Corse,
    EgtPath,
    Option,
    Exclude,
    Include,
    SetScore,
    Lift,
    Put,
    Hover
  };

  std::pair<xboard_verb, std::vector<std::string>> parse_line(std::string line) {
    std::pair<xboard_verb, std::vector<std::string>> ret;

    std::istringstream ss(std::move(line));
    std::string verb;

    if (ss.eof())
      throw std::invalid_argument("Given empty line");

    ss >> verb;

    static std::map<std::string, xboard_verb> verb_tab = {
      {"xboard", xboard_verb::Xboard},
      {"protover", xboard_verb::ProtoVer},
      {"force", xboard_verb::Force},
      {"random", xboard_verb::Random},
      {"level", xboard_verb::Level},
      {"post", xboard_verb::Post},
      {"hard", xboard_verb::Hard},
      {"accepted", xboard_verb::Accepted},
      {"rejected", xboard_verb::Rejected},
      {"new", xboard_verb::New},
      {"level", xboard_verb::Level},
      {"quit", xboard_verb::Quit},
      {"usermove", xboard_verb::UserMove},
      {"hint", xboard_verb::Hint},
      {"variant", xboard_verb::Variant},
    };

    if (auto iter = verb_tab.find(verb); iter != verb_tab.end())
      ret.first = iter->second;
    else
      throw std::invalid_argument("Unknown verb");

    while (!ss.eof())
      ss >> ret.second.emplace_back();

    return ret;
  }

  std::ofstream fs;

  void run_engine(XBoardEngine& eng, std::istream& in, std::ostream& out) {
    std::string line;

    std::filesystem::remove("/tmp/aunty_sue.log");

    while (std::getline(in, line)) {
      fs = std::ofstream{"/tmp/aunty_sue.log", std::fstream::app};
      fs << ">> " << line << std::endl;
      fs.flush();

      auto toks = parse_line(line);

      switch (toks.first) {
        case xboard_verb::Xboard: {
          eng.reset();
        } break;
        case xboard_verb::Force: {
          eng.stop();
        } break;
        case xboard_verb::New: {
          // Default black
          eng.new_game(false);
        } break;
        case xboard_verb::Variant: {
          if (toks.second.at(0) != "auntysue")
            throw std::invalid_argument("Bad variant");

          out << "setup 8x8+0_suicide rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" << std::endl;
        } break;
        case xboard_verb::ProtoVer: {
          out << "feature usermove=1" << std::endl;
          out << "feature time=0" << std::endl;
          out << "feature variants=\"auntysue\"" << std::endl;
          out << "feature done=1" << std::endl;
          fs << "<< [feature list]" << std::endl;
        } break;
        case xboard_verb::UserMove: {
          eng.start();
          auto resp = move2str(eng.respond(str2move(toks.second.at(0))));
          out << "move " << std::string_view{resp.data(), resp.size()} << std::endl;
        } break;
        case xboard_verb::Quit: return;
        default: {}
      }
    }
  }
}
