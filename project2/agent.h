#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "model.h"


//rnd env : random bag size = 3 done by bag and op_space last_op
//taketurn : init = 9 block : done by man(x,8)%2
//slide
//output
extern int last_op;
extern n_tuple_net tnet;
extern const int N_TUPLE;

class agent {
//friend class epsisode;
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent { //evil as instance
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 })/*, popup(0, 9)*/ {
			idx = 0; //consruction wont re run for another episode
		}

	virtual void open_episode(const std::string& flag = "") {
		idx = 0;
	}

	virtual action take_action(const board& after) { //affect ep_state by returning action
		if(idx == 0){
			std::shuffle(bag, bag + 3, engine); //reuse engine ok?
			//printf("\nshuffle!!!\n");
		}

		if(last_op == -1){ //cant use ~last_op, maybe type issue
			std::shuffle(space.begin(), space.end(), engine);
			for (int pos : space) {
				if (after(pos) != 0) continue;
				//printf("\nno op : place bag[%d] = %d @ %d\n", idx, bag[idx], pos);
				board::cell tile = bag[idx];
				idx = (idx + 1) % 3;
				return action::place(pos, tile); //how to affect ep_state
			}
			return action();
		}else{
			std::shuffle(opspace[last_op], opspace[last_op] + 4, engine);
			for (int pos : opspace[last_op]) {
				if (after(pos) != 0) continue;
				board::cell tile = bag[idx];
				//printf("\nop code = %d : place bag[%d] = %d @ %d\n", last_op, idx, bag[idx], pos);
				idx = (idx + 1) % 3;
				return action::place(pos, tile);
			}
			return action();
		}
	}

private:
	std::array<int, 16> space;
	int opspace[4][4] = { {12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15} }; //urdl
	//std::uniform_int_distribution<int> popup;
	int bag[3] = {1, 2, 3};
	int idx;
};

/**
 * dummy player
 * select a legal action randomly
 */
 
class player : public agent { //no need random?
public:
	player(const std::string& args = "") : agent("name=n_tuple_net role=player " + args),
		opcode({ 0, 1, 2, 3 }) { }

	/*virtual void close_episode(const std::string& flag = "") {
		tnet.fit_ep(stat.data.back(), alpha = 0.1/8);
	}*/

	virtual action take_action(const board& before) { //to work with 
		bool e_valid = false;
		int best_op = -1;
		float max_v = 0.0; //max after state explored
		float op_v;
		for (int op : opcode) {
			board::board b = board(before);
			board::reward reward = b.slide(op);

			if(reward == -1) continue; else e_valid = true; //

			n_tuple_net::fids ids = n_tuple_net::get_ids(b);
			op_v = float(reward);
			for(int i = 0 ; i < N_TUPLE ; i++ ) op_v += ids[0][i];
			if(op_v >= max_v){
				max_v = op_v;
				best_op = op;
			}
		}
		if(e_valid == false ) return action();
		else return action::slide(best_op);
	}

private:
	std::array<int, 4> opcode;
	//model::n_tuple_net tnet;
};
