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

class Context {
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

  Context(const int& rows, const int& cols) : rows{rows}, cols{cols}, score{0}, key{Key::kNone} {
    srand(static_cast<unsigned int>(time(NULL)));
    field_ = new char[rows * cols];
    memset(field_, Cell::kE, rows * cols);
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
        if (Context::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)]
            && (!IsValidCell(curr.row + x, curr.col + y) || CellAt(curr.row + x, curr.col + y) != Cell::kE))
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
        if (Context::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell(curr.type));
      }
    }
  }

  void Remove() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Context::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
          Fill(curr.row + x, curr.col + y, Cell::kE);
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

  void ScoreUp(const int& lines) {
    score += Context::kLineMultiplier[lines];
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

class Tetris {
 public:
  Tetris(const int& rows, const int& cols) : context_{rows, cols} {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);
    curs_set(0); 
    start_color();
    init_pair(Context::Cell::kI, COLOR_CYAN, COLOR_BLACK);
    init_pair(Context::Cell::kJ, COLOR_BLUE, COLOR_BLACK);
    init_pair(Context::Cell::kL, COLOR_WHITE, COLOR_BLACK);
    init_pair(Context::Cell::kO, COLOR_YELLOW, COLOR_BLACK);
    init_pair(Context::Cell::kS, COLOR_GREEN, COLOR_BLACK);
    init_pair(Context::Cell::kT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Context::Cell::kZ, COLOR_RED, COLOR_BLACK);
    init_pair(Context::Cell::kE, COLOR_BLACK, COLOR_BLACK);
    field_ = newwin(context_.rows + 2, 2 * context_.cols + 2, 0, 0);
    next_ = newwin(6, 10, 0, 2 * (context_.cols + 1) + 1);
    score_ = newwin(6, 10, 14, 2 * (context_.cols + 1 ) + 1);
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

  void SendKey(const Context::Key& key) {
    context_.key = key;
  }

 private:
  void RefreshField() {
    werase(field_);
    box(field_, 0, 0);
    for (int row = 0; row < context_.rows; row++) {
      wmove(field_, row + 1, 1);
      for (int col = 0; col < context_.cols; col++) {
        if (context_.CellAt(row, col) != Context::Cell::kE) {
          Draw(field_, context_.CellAt(row, col));
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
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
        if (Context::kTetrominos[context_.next.type][Context::Rotate4x4(x, y, context_.next.rotate)]) {
          wmove(next_, x + 1, y * 2 + 1);
          Draw(next_, Context::Cell(context_.next.type));
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

  Context context_;
  WINDOW* field_;
  WINDOW* next_;
  WINDOW* score_;
};

int main(int argc, char* argv[]) {
  Tetris tetris(22, 10);

  while (tetris.Tick()) {
    doupdate();
    usleep(1000);
    switch (getch()) {
    case KEY_LEFT:
      tetris.SendKey(Context::Key::kLeft);
      break;
    case KEY_RIGHT:
      tetris.SendKey(Context::Key::kRight);
      break;
    case KEY_UP:
      tetris.SendKey(Context::Key::kClock);
      break;
    case KEY_DOWN:
      tetris.SendKey(Context::Key::kDrop);
      break;
    default:
      tetris.SendKey(Context::Key::kNone);
    }
  }

  return 0;
}
