/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <set>

#include "evaluate.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "timeman.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

using namespace std;

extern vector<string> setup_bench(const Position&, istream&);

namespace {

  // FEN string of the initial position, normal chess
  const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


  // position() is called when engine receives the "position" UCI command.
  // The function sets up the position described in the given FEN string ("fen")
  // or the starting position ("startpos") and then makes the moves given in the
  // following move list ("moves").

  void position(Position& pos, istringstream& is, StateListPtr& states) {

    Move m;
    string token, fen;

    is >> token;

    if (token == "startpos")
    {
        fen = StartFEN;
        is >> token; // Consume "moves" token if any
    }
    else if (token == "fen")
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one
    pos.set(fen, Options["UCI_Chess960"], &states->back(), Threads.main());

    // Parse move list (if any)
    while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE)
    {
        states->emplace_back();
        pos.do_move(m, states->back());
    }
  }


  // setoption() is called when engine receives the "setoption" UCI command. The
  // function updates the UCI option ("name") to the given value ("value").

  void setoption(istringstream& is) {

    string token, name, value;

    is >> token; // Consume "name" token

    // Read option name (can contain spaces)
    while (is >> token && token != "value")
        name += string(" ", name.empty() ? 0 : 1) + token;

    // Read option value (can contain spaces)
    while (is >> token)
        value += string(" ", value.empty() ? 0 : 1) + token;

    if (Options.count(name))
        Options[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
  }


  // go() is called when engine receives the "go" UCI command. The function sets
  // the thinking time and other parameters from the input string, then starts
  // the search.

  void go(Position& pos, istringstream& is, StateListPtr& states) {

    Search::LimitsType limits;
    string token;
    bool ponderMode = false;

    limits.startTime = now(); // As early as possible!

    while (is >> token)
        if (token == "searchmoves")
            while (is >> token)
                limits.searchmoves.push_back(UCI::to_move(pos, token));

        else if (token == "wtime")     is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "mate")      is >> limits.mate;
        else if (token == "perft")     is >> limits.perft;
        else if (token == "infinite")  limits.infinite = 1;
        else if (token == "ponder")    ponderMode = true;

    Threads.start_thinking(pos, states, limits, ponderMode);
  }

  void multipv_gen(Position& pos, Search::LimitsType limits, Depth depth, set<string>& fens, Value range) {

    limits.startTime = now();
    StateListPtr states(new std::deque<StateInfo>(1));
    Position newpos;
    newpos.set(pos.fen(), Options["UCI_Chess960"], &states->back(), Threads.main());
    Threads.start_thinking(newpos, states, limits);
    Threads.main()->wait_for_search_finished();

    vector<Move> good_moves;

    const Search::RootMoves& rootMoves = pos.this_thread()->rootMoves;
    size_t PVIdx = pos.this_thread()->PVIdx;
    size_t multiPV = std::min((size_t)Options["MultiPV"], rootMoves.size());
    Value bias = Options["AbsScoreBias"] * PawnValueEg / 100;
    bool abs_move_score = Options["AbsMoveScore"];

    Value v0;

    for (size_t i = 0; i < multiPV; ++i)
    {
        bool updated = (i <= PVIdx);

        Value v = updated ? rootMoves[i].score : rootMoves[i].previousScore;
        if (i == 0)
            v0 = v;

        if (abs_move_score ? std::abs((pos.side_to_move() == WHITE? v : -v) - bias) <= range
                           : v0 - v <= range)
            good_moves.push_back(rootMoves[i].pv[0]);
    }

    for (Move m : good_moves)
    {
        StateInfo st;
        pos.do_move(m, st);
        if (depth <= ONE_PLY)
        {
            string fen = pos.fen();
            if (Options["TrimFEN"])
                fen.erase(fen.rfind(" ", fen.rfind(" ") - 1));
            fens.insert(fen);
        }
        else
            multipv_gen(pos, limits, depth - ONE_PLY, fens, range * Options["DepthFactor"] / 100);
        pos.undo_move(m);
    }

  }

  uint64_t perft_gen(Position& pos, Depth depth, set<string>& fens) {

    StateInfo st;
    uint64_t nodes = 0;

    if (depth < ONE_PLY)
    {
        string fen = pos.fen();
        if (Options["TrimFEN"])
            fen.erase(fen.rfind(" ", fen.rfind(" ") - 1));
        fens.insert(fen);
        return ++nodes;
    }

    for (const auto& m : MoveList<LEGAL>(pos))
    {
        pos.do_move(m, st);
        nodes += perft_gen(pos, depth - ONE_PLY, fens);
        pos.undo_move(m);
    }
    return nodes;
  }

  void generate(Position& pos, istringstream& is, set<string>& fens) {

    Search::LimitsType limits;
    string token;

    int depth;
    is >> depth;

    while (is >> token)
        if (token == "depth")          is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "perft")     limits.perft = depth;

    if (limits.perft)
        perft_gen(pos, limits.perft * ONE_PLY, fens);
    else
        multipv_gen(pos, limits, depth * ONE_PLY, fens, Options["MoveScoreRange"] * PawnValueEg / 100);

  }

  void filter(istringstream& is, set<string>& fens) {

    Search::LimitsType limits;
    string token;

    while (is >> token)
        if (token == "depth")          is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;

    StateListPtr states;
    Position pos;
    set<string> filtered_fens;

    Value range         = Options["MoveScoreRange"] * PawnValueEg / 100;
    Value abs_range     = Options["AbsScoreRange"]  * PawnValueEg / 100;
    Value bias          = Options["AbsScoreBias"]   * PawnValueEg / 100;
    bool abs_move_score = Options["AbsMoveScore"];

    for (const auto& fen : fens)
    {
        limits.startTime = now();
        states = StateListPtr(new std::deque<StateInfo>(1));
        pos.set(fen, Options["UCI_Chess960"], &states->back(), Threads.main());
        Threads.start_thinking(pos, states, limits);
        Threads.main()->wait_for_search_finished();

        const Search::RootMoves& rootMoves = pos.this_thread()->rootMoves;
        size_t PVIdx = pos.this_thread()->PVIdx;
        size_t multiPV = std::min((size_t)Options["MultiPV"], rootMoves.size());


        Value v, v0;
        bool exclude = false;

        for (size_t i = 0; i < multiPV; ++i)
        {
            bool updated = (i <= PVIdx);

            v = updated ? rootMoves[i].score : rootMoves[i].previousScore;
            if (i == 0)
            {
                if (std::abs((pos.side_to_move() == WHITE? v : -v) - bias) > abs_range)
                {
                    exclude = true;
                    break;
                }
                v0 = v;
            }
            else if (abs_move_score ? std::abs((pos.side_to_move() == WHITE? v : -v) - bias) > range
                                    : v0 - v > range)
            {
                exclude = true;
                break;
            }
        }

        if (exclude)
            continue;

        filtered_fens.insert(fen);
    }
    fens = filtered_fens;
  }

  void print(set<string>& fens) {
    for (const auto& fen : fens)
        sync_cout << fen << sync_endl;
  }

  void save(set<string>& fens) {
    ofstream file(Options["EPDPath"]);
    for (const auto& fen : fens)
        file << fen << endl;
  }

  // bench() is called when engine receives the "bench" command. Firstly
  // a list of UCI commands is setup according to bench parameters, then
  // it is run one by one printing a summary at the end.

  void bench(Position& pos, istream& args, StateListPtr& states) {

    string token;
    uint64_t num, nodes = 0, cnt = 1;

    vector<string> list = setup_bench(pos, args);
    num = count_if(list.begin(), list.end(), [](string s) { return s.find("go ") == 0; });

    TimePoint elapsed = now();

    for (const auto& cmd : list)
    {
        istringstream is(cmd);
        is >> skipws >> token;

        if (token == "go")
        {
            cerr << "\nPosition: " << cnt++ << '/' << num << endl;
            go(pos, is, states);
            Threads.main()->wait_for_search_finished();
            nodes += Threads.nodes_searched();
        }
        else if (token == "setoption")  setoption(is);
        else if (token == "position")   position(pos, is, states);
        else if (token == "ucinewgame") Search::clear();
    }

    elapsed = now() - elapsed + 1; // Ensure positivity to avoid a 'divide by zero'

    dbg_print(); // Just before exiting

    cerr << "\n==========================="
         << "\nTotal time (ms) : " << elapsed
         << "\nNodes searched  : " << nodes
         << "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
  }

} // namespace


/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.

void UCI::loop(int argc, char* argv[]) {

  Position pos;
  string token, cmd;
  StateListPtr states(new std::deque<StateInfo>(1));
  auto uiThread = std::make_shared<Thread>(0);
  set<string> fens;

  pos.set(StartFEN, false, &states->back(), uiThread.get());

  for (int i = 1; i < argc; ++i)
      cmd += std::string(argv[i]) + " ";

  do {
      if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or EOF
          cmd = "quit";

      istringstream is(cmd);

      token.clear(); // Avoid a stale if getline() returns empty or blank line
      is >> skipws >> token;

      // The GUI sends 'ponderhit' to tell us the user has played the expected move.
      // So 'ponderhit' will be sent if we were told to ponder on the same move the
      // user has played. We should continue searching but switch from pondering to
      // normal search. In case Threads.stopOnPonderhit is set we are waiting for
      // 'ponderhit' to stop the search, for instance if max search depth is reached.
      if (    token == "quit"
          ||  token == "stop"
          || (token == "ponderhit" && Threads.stopOnPonderhit))
          Threads.stop = true;

      else if (token == "ponderhit")
          Threads.ponder = false; // Switch to normal search

      else if (token == "uci")
          sync_cout << "id name " << engine_info(true)
                    << "\n"       << Options
                    << "\nuciok"  << sync_endl;

      // Book generation commands
      else if (token == "generate")   generate(pos, is, fens);
      else if (token == "filter")     filter(is, fens);
      else if (token == "clear")      fens.clear();
      else if (token == "size")       sync_cout << fens.size() << sync_endl;
      else if (token == "print")      print(fens);
      else if (token == "save")       save(fens);

      else if (token == "setoption")  setoption(is);
      else if (token == "go")         go(pos, is, states);
      else if (token == "position")   position(pos, is, states);
      else if (token == "ucinewgame") Search::clear();
      else if (token == "isready")    sync_cout << "readyok" << sync_endl;

      // Additional custom non-UCI commands, mainly for debugging
      else if (token == "flip")  pos.flip();
      else if (token == "bench") bench(pos, is, states);
      else if (token == "d")     sync_cout << pos << sync_endl;
      else if (token == "eval")  sync_cout << Eval::trace(pos) << sync_endl;
      else
          sync_cout << "Unknown command: " << cmd << sync_endl;

  } while (token != "quit" && argc == 1); // Command line args are one-shot
}


/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCI::value(Value v) {

  assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

  stringstream ss;

  if (abs(v) < VALUE_MATE - MAX_PLY)
      ss << "cp " << v * 100 / PawnValueEg;
  else
      ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

  return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCI::square(Square s) {
  return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
}


/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

string UCI::move(Move m, bool chess960) {

  Square from = from_sq(m);
  Square to = to_sq(m);

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "0000";

  if (type_of(m) == CASTLING && !chess960)
      to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));

  string move = UCI::square(from) + UCI::square(to);

  if (type_of(m) == PROMOTION)
      move += " pnbrqk"[promotion_type(m)];

  return move;
}


/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

Move UCI::to_move(const Position& pos, string& str) {

  if (str.length() == 5) // Junior could send promotion piece in uppercase
      str[4] = char(tolower(str[4]));

  for (const auto& m : MoveList<LEGAL>(pos))
      if (str == UCI::move(m, pos.is_chess960()))
          return m;

  return MOVE_NONE;
}
