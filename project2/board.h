#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <cmath>

/**
 * array-based board for 2048
 *
 * index (1-d form):
 *  (0)  (1)  (2)  (3)
 *  (4)  (5)  (6)  (7)
 *  (8)  (9) (10) (11)
 * (12) (13) (14) (15)
 *
 */

//slide
extern int last_op;

class board {
friend class weight;
friend class n_tuple_net;
public:
	typedef uint32_t cell;
	typedef std::array<cell, 4> row;
	typedef std::array<row, 4> grid;
	typedef uint64_t data;
	typedef int reward;

public:
	board() : tile(), attr(0) {}
	board(const grid& b, data v = 0) : tile(b), attr(v) {}
	board(const board& b) = default;
	board& operator =(const board& b) = default;

	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 4][i % 4]; }
	const cell& operator ()(unsigned i) const { return tile[i / 4][i % 4]; }

	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 16) return -1;
		if (tile != 1 && tile != 2 && tile != 3) return -1; // need to change:bugged for 1 day(solved by judge: action=??)
		operator()(pos) = tile; //actually place
		return tile == 3 ? 3 : 0; //mismatch solved
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	reward slide(unsigned opcode) {
		last_op = opcode & 0b11; // for random env random space
		switch (opcode & 0b11) { //fast mod 4
		case 0: return slide_up();
		case 1: return slide_right();
		case 2: return slide_down();
		case 3: return slide_left();
		default: return -1;
		}
	}

	//to do

	inline bool mergible(cell a, cell b){
		if(a == 1 && b == 2) return true;
		if(a == 2 && b == 1) return true;
		if(a == b) return (a == 2 || a == 1 || a == 6144) ? false : true; //max tile dealt with
	}

	// 3^(log2(s/3)+1)
	inline reward scoref(const board& b){ //speed up?
		reward sum = 0;
		for (int r = 0; r < 4; r++) {
			for (int c = 0; c < 4; c++) {
				int x = b.tile[r][c];
				if(x > 2) sum += pow(3, log2(x/3)+1); //why bugged
			}
		}
		return sum;
	}

	reward slide_left() {
		board prev = *this;
		//need to be blocked to be merged
		//one time merged one on one line
		//mergible 0 special case / or only deal with left
		//10/23 11:30 rewrite is good

		for (int r = 0; r < 4; r++) {
			bool merged = false;
			auto& row = tile[r];
			//bool blocked = (row[0] != 0);
			for (int c = 1; c < 4; c++) {
				int now = row[c];
				int pre = row[c-1];
				if (now == 0) continue;
				if (pre != 0) {
					if (mergible(now, pre) &&  !merged) {
						row[c-1] = now + pre; 
						row[c] = 0;
						merged = true;
					} else {
						continue;
					}
				} else {
					row[c-1] = now + pre; //pre = 0
					row[c] = 0;
				}
			}
		}

		return (*this != prev) ? (scoref(*this) - scoref(prev)) : -1;
		//return (*this != prev) ? 1 : -1;
	}
	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		return score;
	}
	reward slide_up() {
		rotate_right();
		reward score = slide_right();
		rotate_left();
		return score;
	}
	reward slide_down() {
		rotate_right();
		reward score = slide_left();
		rotate_left();
		return score;
	}

	void transpose() {
		for (int r = 0; r < 4; r++) {
			for (int c = r + 1; c < 4; c++) {
				std::swap(tile[r][c], tile[c][r]);
			}
		}
	}

	void reflect_horizontal() {
		for (int r = 0; r < 4; r++) {
			std::swap(tile[r][0], tile[r][3]);
			std::swap(tile[r][1], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 4; c++) {
			std::swap(tile[0][c], tile[3][c]);
			std::swap(tile[1][c], tile[2][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */
	void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}

	void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	void reverse() { reflect_horizontal(); reflect_vertical(); }

public:
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		out << "+------------------------+" << std::endl;
		for (auto& row : b.tile) {
			out << "|" << std::dec;
			for (auto t : row) out << std::setw(6) << t;//((1 << t) & -2u);//
			out << "|" << std::endl;
		}
		out << "+------------------------+" << std::endl;
		return out;
	}

private:
	grid tile;
	data attr;
};
