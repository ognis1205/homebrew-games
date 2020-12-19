/*
 * Tetris:
 * Uncurses-based Tetris Implementation.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#include <bitset>
#include <cstring>
#include <iostream>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

enum Cell {
  kEmpty,
  kIShape,
  kJShape,
  kLShape,
  kOShape,
  kSShape,
  kTShape,
  kZShape
};

enum Key {
  kNone,
  kLeft,
  kRight,
  kClock,
  kCounter,
  kDrop
};

constexpr bitset<16> kTetrominos[8] = {
  0b0000000000000000,
  0b0010001000100010,
  0b0010011001000000,
  0b0100011000100000,
  0b0000011001100000,
  0b0010011000100000,
  0b0000011000100010,
  0b0000011001000100
};

int RotateIndex4x4(const int& x, const int& y, const int& r) {
  switch (r % 4) {
  case 0:
    return (y * 4) + x;
  case 1:
    return 12 + y - (x * 4);
  case 2:
    return 15 - (y * 4) - x;
  case 3:
    return 3 - y + (x * 4);
  }
  return 0;
}

struct Tetromino {
  int type;
  int row;
  int col;
  int rotate;
};

struct Config {
  int rows = 22;
  int cols = 10;
  int display_ratio = 1;
};

class Context {
 public:
  Context(const int& rows, const int& cols)
      : rows{rows}, cols{cols}, score{0}, key{Key::kNone} {
    srand(static_cast<unsigned int>(time(NULL)));
    field_ = new char[rows * cols];
    memset(field_, Cell::kEmpty, rows * cols);
    Next();
    Next();
  }

  ~Context() { delete[] field_; }

  char CellAt(const int& row, const int& col) const {
    return field_[cols * row + col];
  }

  bool Update() {
    if (ticks_till_drop_-- <= 0) {
      Remove();
      curr.row++;
      if (IsConsistent()) {
        ticks_till_drop_ = kTicksTillDrop;
        Put();
      } else {
        curr.row--;
        Put();
        Next();
      }
    }
    HandleKey();
    ScoreUp(CheckLines());
    return !GameOver();
  }

  const int rows;
  const int cols;
  int score;
  Key key;
  Tetromino curr, next;

 private:
  static const int kTicksTillDrop = 500;

  static constexpr int kLineMultiplier[] = {0, 40, 100, 300, 1200};

  void Next() {
    curr = next;
    next = Tetromino();
    next.type = rand() % 7 + 1;
    next.row = 0;
    next.col = cols / 2 - 2;
    next.rotate = 0;
  }

  bool IsValidCell(const int& row, const int& col) const {
    return 0 <= row && row < rows && 0 <= col && col < cols;
  }

  bool IsConsistent() const {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (kTetrominos[curr.type][RotateIndex4x4(x, y, curr.rotate)] &&
            (!IsValidCell(curr.row + x, curr.col + y) ||
             CellAt(curr.row + x, curr.col + y) != Cell::kEmpty))
          return false;
      }
    }
    return true;
  }

  void Fill(const int& row, const int& col, const char& cell) {
    field_[cols * row + col] = cell;
  }

  void Put() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (kTetrominos[curr.type][RotateIndex4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell(curr.type));
      }
    }
  }

  void Remove() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (kTetrominos[curr.type][RotateIndex4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell::kEmpty);
      }
    }
  }

  void MoveTo(const int& dir) {
    Remove();
    curr.col += dir;
    if (!IsConsistent()) {
      curr.col -= dir;
    }
    Put();
  }

  void RotateTo(const int& dir) {
    Remove();
    while (true) {
      curr.rotate = (curr.rotate + dir) % 4;
      if (IsConsistent()) break;
      curr.col--;
      if (IsConsistent()) break;
      curr.col += 2;
      if (IsConsistent()) break;
      curr.col--;
    }
    Put();
  }

  void Drop() {
    Remove();
    curr.row++;
    if (!IsConsistent()) {
      curr.row--;
    }
    Put();
  }

  bool LineFilled(const int& row) const {
    for (int col = 0; col < cols; col++) {
      if (CellAt(row, col) == Cell::kEmpty) return false;
    }
    return true;
  }

  void ShiftLines(int row) {
    for (int i = row - 1; i >= 0; i--) {
      for (int j = 0; j < cols; j++) {
        Fill(i + 1, j, CellAt(i, j));
        Fill(i, j, Cell::kEmpty);
      }
    }
  }

  int CheckLines() {
    int lines = 0;
    Remove();
    for (int row = rows - 1; row >= 0; row--) {
      if (LineFilled(row)) {
        ShiftLines(row++);
        lines++;
      }
    }
    Put();
    return lines;
  }

  void HandleKey() {
    switch (key) {
      case kLeft:
        MoveTo(-1);
        break;
      case kRight:
        MoveTo(1);
        break;
      case kDrop:
        Drop();
        break;
      case kClock:
        RotateTo(1);
        break;
      case kCounter:
        RotateTo(-1);
        break;
      default:
        break;
    }
  }

  void ScoreUp(const int& lines) { score += Context::kLineMultiplier[lines]; }

  bool GameOver() {
    Remove();
    for (int row = 0; row < 2; row++) {
      for (int col = 0; col < cols; col++) {
        if (CellAt(row, col) != Cell::kEmpty) {
          return true;
        }
      }
    }
    Put();
    return false;
  }

  char* field_;
  int ticks_till_drop_;
};

class Tetris {
 public:
  Tetris(const Config& config)
      : config_{config}, context_{config_.rows, config_.cols} {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);
    curs_set(0);
    start_color();
    init_pair(Cell::kIShape, COLOR_CYAN, COLOR_BLACK);
    init_pair(Cell::kJShape, COLOR_BLUE, COLOR_BLACK);
    init_pair(Cell::kLShape, COLOR_WHITE, COLOR_BLACK);
    init_pair(Cell::kOShape, COLOR_YELLOW, COLOR_BLACK);
    init_pair(Cell::kSShape, COLOR_GREEN, COLOR_BLACK);
    init_pair(Cell::kTShape, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Cell::kZShape, COLOR_RED, COLOR_BLACK);
    init_pair(Cell::kEmpty, COLOR_BLACK, COLOR_BLACK);
    field_ = newwin(
      config_.display_ratio * context_.rows + 2,
      2 * config_.display_ratio * context_.cols + 2,
      0, 0);
    next_ = newwin(
      config_.display_ratio * 4 + 2,
      2 * config_.display_ratio * 4 + 2,
      0, 2 * config_.display_ratio * context_.cols + 2 + config_.display_ratio);
    score_ = newwin(
      config_.display_ratio * 12,
      config_.display_ratio * 20,
      config_.display_ratio * 12 + 2, 2 * config_.display_ratio * context_.cols + 2 + config_.display_ratio);
  }

  ~Tetris() {
    wclear(stdscr);
    endwin();
  }

  bool Tick() {
    RefreshField();
    RefreshNext();
    RefreshScore();
    return context_.Update();
  }

  void SendKey(const Key& key) { context_.key = key; }

 private:
  void RefreshField() {
    werase(field_);
    box(field_, 0, 0);
    for (int row = 0; row < config_.display_ratio * context_.rows; row++) {
      wmove(field_, row + 1, 1);
      for (int col = 0; col < config_.display_ratio * context_.cols; col++) {
        if (context_.CellAt(row / config_.display_ratio, col / config_.display_ratio) != Cell::kEmpty) {
          Draw(field_, context_.CellAt(row / config_.display_ratio, col / config_.display_ratio));
        } else {
          Draw(field_);
        }
      }
    }
    wnoutrefresh(field_);
  }

  void RefreshNext() {
    werase(next_);
    box(next_, 0, 0);
    for (int x = 0; x < config_.display_ratio * 4; x++) {
      wmove(next_, x + 1, 1);
      for (int y = 0; y < config_.display_ratio * 4; y++) {
        if (kTetrominos[context_.next.type][RotateIndex4x4(x / config_.display_ratio, y / config_.display_ratio, context_.next.rotate)]) {
          Draw(next_, Cell(context_.next.type));
        } else {
          Draw(next_);
        }
      }
    }
    wnoutrefresh(next_);
  }

  void RefreshScore() {
    werase(score_);
    wprintw(score_, "Score\n%d\n", context_.score);
    wnoutrefresh(score_);
  }

  void Draw(WINDOW* window) {
    waddch((window), ' ');
    waddch((window), ' ');
  }

  void Draw(WINDOW* window, const char& cell) {
    waddch(window, ' ' | A_REVERSE | COLOR_PAIR(cell));
    waddch(window, ' ' | A_REVERSE | COLOR_PAIR(cell));
  }

  Config config_;
  Context context_;
  WINDOW* field_;
  WINDOW* next_;
  WINDOW* score_;
};

int main(int argc, char* argv[]) {
  Config config;
  opterr = 0;
  int option;
  while ((option = getopt(argc, argv, "r:c:d:")) != -1 ) {
    switch (option) {
    case 'r':
      config.rows = atoi(optarg);
      break;
    case 'c':
      config.cols = atoi(optarg);
      break;
    case 'd':
      config.display_ratio = atoi(optarg);
      break;
    case '?':
    case 'h':
    default :
      cout << "show usage." << endl;
      break;
    case -1:
      break;
    }
  }

  Tetris tetris(config);
  while (tetris.Tick()) {
    doupdate();
    usleep(1000);
    switch (getch()) {
      case KEY_LEFT:
        tetris.SendKey(Key::kLeft);
        break;
      case KEY_RIGHT:
        tetris.SendKey(Key::kRight);
        break;
      case KEY_UP:
        tetris.SendKey(Key::kClock);
        break;
      case KEY_DOWN:
        tetris.SendKey(Key::kDrop);
        break;
      default:
        tetris.SendKey(Key::kNone);
    }
  }

  return 0;
}
