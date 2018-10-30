#pragma once
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <chrono>
#include <numeric>
#include "board.h"
#include "action.h"
//#include "agent.h"

//take turn init = 9

class statistic;
class agent;

class episode {
friend class statistic;
friend class n_tuple_net;
public:
	episode() : ep_state(initial_state()), ep_score(0), ep_time(0) { ep_moves.reserve(10000); ep_states.reserve(10000);} //care overload

public:
	board& state() { return ep_state; }
	const board& state() const { return ep_state; }
	board::reward score() const { return ep_score; }

	void open_episode(const std::string& tag) {
		ep_open = { tag, millisec() };
	}
	void close_episode(const std::string& tag) {
		ep_close = { tag, millisec() };
	}
	bool apply_action(action move) {
		//added state for training
		ep_states.push_back(ep_state); //state return reference which is no good
		board::reward reward = move.apply(state());
		if (reward == -1) return false;
		ep_moves.emplace_back(move, reward, millisec() - ep_time); //construct directly at vector back //move list construction
		ep_score += reward; //no need to change if use diff of round score
		return true;
	}
	agent& take_turns(agent& play, agent& evil) {
		ep_time = millisec();
		return (std::max(step(), size_t(8)) % 2) ? play : evil; //9init
	}
	agent& last_turns(agent& play, agent& evil) {
		return take_turns(evil, play);
	}

public:
	size_t step(unsigned who = -1u) const {
		int size = ep_moves.size(); // 'int' is important for handling 0
		switch (who) {
		case action::slide::type: return (size - 8) / 2; //change size-1 to size -8
		case action::place::type: return (size - (size - 8) / 2);
		default:                  return size;
		}
	}

	time_t time(unsigned who = -1u) const {
		time_t time = 0;
		size_t i = 2;
		switch (who) {
		case action::place::type:
			if (ep_moves.size()) time += ep_moves[0].time, i = 1;
			// no break;
		case action::slide::type:
			while (i < ep_moves.size()) time += ep_moves[i].time, i += 2; //?
			break;
		default:
			time = ep_close.when - ep_open.when;
			break;
		}
		return time;
	}

	std::vector<action> actions(unsigned who = -1u) const { //may need change //no by li-shan //actually no call wtf
		std::vector<action> res;
		size_t i = 2;
		switch (who) {
		case action::place::type:
			if (ep_moves.size()) res.push_back(ep_moves[0]), i = 1;
			// no break;
		case action::slide::type:
			while (i < ep_moves.size()) res.push_back(ep_moves[i]), i += 2;
			break;
		default:
			res.assign(ep_moves.begin(), ep_moves.end());
			break;
		}
		return res;
	}

public:

	friend std::ostream& operator <<(std::ostream& out, const episode& ep) {
		out << ep.ep_open << '|';
		for (const move& mv : ep.ep_moves) out << mv;
		out << '|' << ep.ep_close;
		return out;
	}
	friend std::istream& operator >>(std::istream& in, episode& ep) {
		ep = {};
		std::string token;
		std::getline(in, token, '|');
		std::stringstream(token) >> ep.ep_open;
		std::getline(in, token, '|');
		for (std::stringstream moves(token); !moves.eof(); moves.peek()) {
			ep.ep_moves.emplace_back();
			moves >> ep.ep_moves.back();
			ep.ep_score += action(ep.ep_moves.back()).apply(ep.ep_state);
		}
		std::getline(in, token, '|');
		std::stringstream(token) >> ep.ep_close;
		return in;
	}

protected:

	struct move { // struct for move outputs
		action code;
		board::reward reward; //need change after proj2
		time_t time;
		move(action code = {}, board::reward reward = 0, time_t time = 0) : code(code), reward(reward), time(time) {}

		operator action() const { return code; }
		friend std::ostream& operator <<(std::ostream& out, const move& m) {
			out << m.code; //code is of type action
			if (m.reward) out << '[' << std::dec << m.reward << ']';
			if (m.time) out << '(' << std::dec << m.time << ')';
			return out;
		}
		friend std::istream& operator >>(std::istream& in, move& m) {
			in >> m.code;
			m.reward = 0;
			m.time = 0;
			if (in.peek() == '[') {
				in.ignore(1);
				in >> std::dec >> m.reward;
				in.ignore(1);
			}
			if (in.peek() == '(') {
				in.ignore(1);
				in >> std::dec >> m.time;
				in.ignore(1);
			}
			return in;
		}
	};

	struct meta { //struct for open/close episode output
		std::string tag;
		time_t when;
		meta(const std::string& tag = "N/A", time_t when = 0) : tag(tag), when(when) {}

		friend std::ostream& operator <<(std::ostream& out, const meta& m) {
			return out << m.tag << "@" << std::dec << m.when;
		}
		friend std::istream& operator >>(std::istream& in, meta& m) {
			return std::getline(in, m.tag, '@') >> std::dec >> m.when;
		}
	};

	static board initial_state() {
		return {}; //==all zeros???
	}
	static time_t millisec() {
		auto now = std::chrono::system_clock::now().time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	}

private:
	board ep_state;
	board::reward ep_score;
	std::vector<move> ep_moves;
	std::vector<board> ep_states; //for training
	time_t ep_time;

	meta ep_open;
	meta ep_close;
};
