/*
 * Tetris:
 * Uncurses-based Tetris Implementation.
 * Written by and Copyright (C) 2020 Shingo OKAWA shingo.okawa.g.h.c@gmail.com
 * Trademarks are owned by their respect owners.
 */
#include <bitset>
#include <cstring>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

class Tetris {
 public:
  enum Cell { kE, kI, kJ, kL, kO, kS, kT, kZ };

  enum Key { kLeft, kRight, kClock, kCounter, kDrop, kNone };

  static constexpr bitset<16> kTetrominos[8] = {
    0b0000000000000000,
    0b0010001000100010,
    0b0010011001000000,
    0b0100011000100000,
    0b0000011001100000,
    0b0010011000100000,
    0b0000011000100010,
    0b0000011001000100
  };

  struct Tetromino {
    int type;
    int row;
    int col;
    int rotate;
  };

  static int Rotate4x4(const int& x, const int& y, const int& r) {
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

  Tetris(const int& rows, const int& cols) : rows{rows}, cols{cols} {
    srand((unsigned int)time(NULL));
    field_ = new char[rows * cols];
    memset(field_, Cell::kE, rows * cols);
    New();
    New();
  }

  ~Tetris() { delete[] field_; }

  char CellAt(const int& row, const int& col) const {
    return field_[cols * row + col];
  }

  bool Tick(const Key& key) {
    ticks_till_drop_--;
    if (ticks_till_drop_ <= 0) {
      Remove();
      curr.row++;
      if (Fit()) {
        ticks_till_drop_ = kTicksTillDrop;
        Put();
      } else {
        curr.row--;
        Put();
        New();
      }
    }
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
    Score(CheckLines());
    return !GameOver();
  }

  int rows;
  int cols;
  int score;
  Tetromino curr, next;

 private:
  static const int kTicksTillDrop = 500;

  static constexpr int kLineMultiplier[] = {0, 40, 100, 300, 1200};

  void New() {
    curr = next;
    next = Tetromino();
    next.type = rand() % 7 + 1;
    next.row = 0;
    next.col = cols / 2 - 2;
    next.rotate = 0;
  }
  
  void Fill(const int& row, const int& col, const char& cell) {
    field_[cols * row + col] = cell;
  }

  bool IsValid(const int& row, const int& col) const {
    return 0 <= row && row < rows && 0 <= col && col < cols;
  }

  bool Fit() const {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)]
            && (!IsValid(curr.row + x, curr.col + y) || CellAt(curr.row + x, curr.col + y) != Cell::kE))
          return false;
      }
    }
    return true;
  }

  void Put() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell(curr.type));
      }
    }
  }

  void Remove() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell::kE);
      }
    }
  }

  void MoveTo(const int& dir) {
    Remove();
    curr.col += dir;
    if (!Fit()) {
      curr.col -= dir;
    }
    Put();
  }

  void Drop() {
    Remove();
    curr.row++;
    if (!Fit()) {
      curr.row--;
    }
    Put();
  }

  void RotateTo(const int& dir) {
    Remove();
    while (true) {
      curr.rotate = (curr.rotate + dir) % 4;
      if (Fit()) break;
      curr.col--;
      if (Fit()) break;
      curr.col += 2;
      if (Fit()) break;
      curr.col--;
    }
    Put();
  }

  bool LineFilled(const int& row) const {
    for (int col = 0; col < cols; col++) {
      if (CellAt(row, col) == Cell::kE)
        return false;
    }
    return true;
  }

  void ShiftLines(int row) {
    for (int i = row - 1; i >= 0; i--) {
      for (int j = 0; j < cols; j++) {
        Fill(i + 1, j, CellAt(i, j));
        Fill(i, j, Cell::kE);
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

  void Score(const int& lines) {
    score += Tetris::kLineMultiplier[lines];
  }

  bool GameOver() {
    Remove();
    for (int row = 0; row < 2; row++) {
      for (int col = 0; col < cols; col++) {
        if (CellAt(row, col) != Cell::kE) {
          return true;
        }
      }
    }
    Put();
    return false;
  }
  
  char *field_;
  int ticks_till_drop_;
};

class Console {
 public:
  Console(const int& rows, const int& cols) : tetris_{rows, cols} {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);
    curs_set(0); 
    start_color();
    init_pair(Tetris::Cell::kI, COLOR_CYAN, COLOR_BLACK);
    init_pair(Tetris::Cell::kJ, COLOR_BLUE, COLOR_BLACK);
    init_pair(Tetris::Cell::kL, COLOR_WHITE, COLOR_BLACK);
    init_pair(Tetris::Cell::kO, COLOR_YELLOW, COLOR_BLACK);
    init_pair(Tetris::Cell::kS, COLOR_GREEN, COLOR_BLACK);
    init_pair(Tetris::Cell::kT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Tetris::Cell::kZ, COLOR_RED, COLOR_BLACK);
    init_pair(Tetris::Cell::kE, COLOR_BLACK, COLOR_BLACK);
    field_ = newwin(tetris_.rows + 2, 2 * tetris_.cols + 2, 0, 0);
    next_ = newwin(6, 10, 0, 2 * (tetris_.cols + 1) + 1);
    score_ = newwin(6, 10, 14, 2 * (tetris_.cols + 1 ) + 1);
  }

  ~Console() {
    wclear(stdscr);
    endwin();
  }

  bool Tick(const Tetris::Key& key) {
    RefreshField();
    RefreshNext();
    RefreshScore();
    return tetris_.Tick(key);
  }

 private:
  void RefreshField() {
    werase(field_);
    box(field_, 0, 0);
    for (int row = 0; row < tetris_.rows; row++) {
      wmove(field_, row + 1, 1);
      for (int col = 0; col < tetris_.cols; col++) {
        if (tetris_.CellAt(row, col) != Tetris::Cell::kE) {
          Draw(field_, tetris_.CellAt(row, col));
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
    if (tetris_.next.type == -1) {
      wnoutrefresh(next_);
      return;
    }

    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Tetris::kTetrominos[tetris_.next.type][Tetris::Rotate4x4(x, y, tetris_.next.rotate)]) {
          wmove(next_, x + 1, y * 2 + 1);
          Draw(next_, Tetris::Cell(tetris_.next.type));
        }
      }
    }
    wnoutrefresh(next_);
  }

  void RefreshScore() {
    werase(score_);
    wprintw(score_, "Score\n%d\n", tetris_.score);
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

  Tetris tetris_;
  WINDOW* field_;
  WINDOW* next_;
  WINDOW* score_;
};

int main(int argc, char* argv[]) {
  Console console(22, 10);
  Tetris::Key key = Tetris::Key::kNone;

  while (console.Tick(key)) {
    doupdate();
    usleep(1000);
    switch (getch()) {
    case KEY_LEFT:
      key = Tetris::Key::kLeft;
      break;
    case KEY_RIGHT:
      key = Tetris::Key::kRight;
      break;
    case KEY_UP:
      key = Tetris::Key::kClock;
      break;
    case KEY_DOWN:
      key = Tetris::Key::kDrop;
      break;
    default:
      key = Tetris::Key::kNone;
    }
  }

  return 0;
}
