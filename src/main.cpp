#include "sue.hpp"
#include "xboard.hpp"

#include <fstream>

#include <chrono>
#include <thread>

int main() {
//  std::this_thread::sleep_for(std::chrono::seconds{10});
  aunty_sue::sue eng;
  aunty_sue::run_engine(eng);
}
