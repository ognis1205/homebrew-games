/*
 * Tetris.
 */
#include <bitset>
#include <cstring>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

class Tetris {
 public:
  enum Cell { kI, kJ, kL, kO, kS, kT, kZ, kE };

  enum Move { kLeft, kRight, kClock, kCounter, kDrop, kNone };

  static constexpr bitset<16> kTetrominos[7] = {
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
    field_ = new char[rows * cols];
    memset(field_, Cell::kE, rows * cols);
    NewTetromino();
    NewTetromino();
  }

  ~Tetris() { delete[] field_; }

  char CellAt(const int& row, const int& col) const {
    return field_[cols * row + col];
  }

  bool Tick(const Move& move) {
    ticks_till_down_--;
    if (ticks_till_down_ <= 0) {
      RemoveTetromino();
      curr.row++;
      if (TetrominoFits()) {
	ticks_till_down_ = kTicksTillDown;
	PutTetromino();
      } else {
	curr.row--;
	PutTetromino();
	NewTetromino();
      }
    }
    switch (move) {
    case kLeft:
      MoveTetromino(-1);
      break;
    case kRight:
      MoveTetromino(1);
      break;
    case kDrop:
      DropTetromino();
      break;
    case kClock:
      RotateTetromino(1);
      break;
    case kCounter:
      RotateTetromino(-1);
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
  static const int kTicksTillDown = 500;

  static constexpr int kLineMultiplier[] = {0, 40, 100, 300, 1200};

  void NewTetromino() {
    curr = next;
    next = Tetromino();
    next.type = rand() % 7;
    next.row = 0;
    next.col = cols / 2 - 2;
    next.rotate = 0;
  }
  
  void DrawCell(const int& row, const int& col, const char& cell) {
    field_[cols * row + col] = cell;
  }

  bool CellIsValid(const int& row, const int& col) const {
    return 0 <= row && row < rows && 0 <= col && col < cols;
  }

  bool TetrominoFits() const {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
	if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)]
	    && (!CellIsValid(curr.row + x, curr.col + y) || CellAt(curr.row + x, curr.col + y) != Cell::kE))
	  return false;
      }
    }
    return true;
  }

  void PutTetromino() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
	if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
	  DrawCell(curr.row + x, curr.col + y, Cell(curr.type));
      }
    }
  }

  void RemoveTetromino() {
    for (int x = 0; x < 4; x++) {
      for (int y = 0; y < 4; y++) {
	if (Tetris::kTetrominos[curr.type][Rotate4x4(x, y, curr.rotate)])
	  DrawCell(curr.row + x, curr.col + y, Cell::kE);
      }
    }
  }

  void MoveTetromino(const int& dir) {
    RemoveTetromino();
    curr.col += dir;
    if (!TetrominoFits()) {
      curr.col -= dir;
    }
    PutTetromino();
  }

  void DropTetromino() {
    RemoveTetromino();
    while (TetrominoFits()) {
      curr.row++;
    }
    curr.row--;
    PutTetromino();
    NewTetromino();
  }

  void RotateTetromino(const int& dir) {
    RemoveTetromino();
    while (true) {
      curr.rotate = (curr.rotate + dir) % 4;
      if (TetrominoFits()) break;
      curr.col--;
      if (TetrominoFits()) break;
      curr.col += 2;
      if (TetrominoFits()) break;
      curr.col--;
    }
    PutTetromino();
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
	DrawCell(i + 1, j, CellAt(i, j));
	DrawCell(i, j, Cell::kE);
      }
    }
  }
  
  int CheckLines() {
    int lines = 0;
    RemoveTetromino();
    for (int row = rows - 1; row >= 0; row--) {
      if (LineFilled(row)) {
	ShiftLines(row++);
	lines++;
      }
    }
    PutTetromino();
    return lines;
  }

  void Score(const int& lines) {
    score += Tetris::kLineMultiplier[lines];
  }

  bool GameOver() {
    RemoveTetromino();
    for (int row = 0; row < 2; row++) {
      for (int col = 0; col < cols; col++) {
	if (CellAt(row, col) != Cell::kE) {
	  return true;
	}
      }
    }
    PutTetromino();
    return false;
  }
  
  char *field_;
  int ticks_till_down_;
};

class Window {
 public:
  Window(Tetris& tetris) : tetris_{tetris} {
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

  ~Window() {
    wclear(stdscr);
    endwin();
  }

  bool Tick(const Tetris::Move& move) {
    DisplayField();
    DisplayNext();
    DisplayScore();
    return tetris_.Tick(move);
  }

 private:
  void DisplayField() {
    werase(field_);
    box(field_, 0, 0);
    for (int row = 0; row < tetris_.rows; row++) {
      wmove(field_, row + 1, 1);
      for (int col = 0; col < tetris_.cols; col++) {
	if (tetris_.CellAt(row, col) != Tetris::Cell::kE) {
	  PutBlock(field_, tetris_.CellAt(row, col));
	} else {
	  PutBlock(field_);
	}
      }
    }
    wnoutrefresh(field_);
  }

  void DisplayNext() {
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
	  PutBlock(next_, Tetris::Cell(tetris_.next.type));
	}
      }
    }
    wnoutrefresh(next_);
  }

  void DisplayScore() {
    werase(score_);
    wprintw(score_, "Score\n%d\n", tetris_.score);
    wnoutrefresh(score_);
  }

  void PutBlock(WINDOW* window) {
    waddch((window), ' ');
    waddch((window), ' ');
  }

  void PutBlock(WINDOW* window, const char& cell) {
    waddch(window, ' ' | A_REVERSE | COLOR_PAIR(cell));
    waddch(window, ' ' | A_REVERSE | COLOR_PAIR(cell));
  }

  Tetris& tetris_;
  WINDOW* field_;
  WINDOW* next_;
  WINDOW* score_;
};

int main(int argc, char* argv[]) {
  Tetris tetris(22, 10);
  Window window(tetris);
  Tetris::Move key = Tetris::Move::kNone;

  while (window.Tick(key)) {
    doupdate();
    usleep(100);
    switch (getch()) {
    case KEY_LEFT:
      key = Tetris::Move::kLeft;
      break;
    case KEY_RIGHT:
      key = Tetris::Move::kRight;
      break;
    case KEY_UP:
      key = Tetris::Move::kClock;
      break;
    case KEY_DOWN:
      key = Tetris::Move::kDrop;
      break;
    default:
      key = Tetris::Move::kNone;
    }
  }

  return 0;
}
