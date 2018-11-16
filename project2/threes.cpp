/**
 * Basic Environment for Game 2048->threes
 * use 'g++ -std=c++11 -O3 -g -o 2048->exthrees 2048.cpp->thress.cpp' to compile the source
 *
 * Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 * http://www.aigames.nctu.edu.tw
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"
#include "model.h"


bool LEARN_ENABLE = true;

int last_op = -1;
n_tuple_net tnet(N);

extern const int N;
extern const int N_TUPLE;
extern const int N_isomorphism;

//const float lr_init = 0.1/(N_TUPLE*N_isomorphism/2); 0.9/100 877/9213 100000 best
//only some none zero weight
float lr_init = 0.1/N_TUPLE;
float decay_rate = 1.0;
int decay_ep = 100;
int ep_count = 0;
float decay = 1.0;

int main(int argc, const char* argv[]) {

	/*
	using std::cout;
	using std::endl;
	board b ;
	for(int i = 0; i < 16; i++) b(i) = 1;
	std::cout << b << std::endl;
	auto x = n_tuple_net::get_ids(b);
	for(int i = 0; i < N_isomorphism; i++)
	{
		for(int j = 0; j < N_TUPLE; j++) cout << x[i][j] << ' ';
		cout << endl;
	}
	return 0;
	*/

	std::cout << "Threes-Demo: ";
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl << std::endl;

	size_t total = 1000, block = 0, limit = 0;
	std::string play_args, evil_args;
	std::string load, save, loadw, savew;
	bool summary = false;
	for (int i = 1; i < argc; i++) {
		std::string para(argv[i]);
		if (para.find("--total=") == 0) {
			total = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--block=") == 0) {
			block = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--limit=") == 0) {
			limit = std::stoull(para.substr(para.find("=") + 1));
		} else if (para.find("--play=") == 0) {
			play_args = para.substr(para.find("=") + 1);
		} else if (para.find("--evil=") == 0) {
			evil_args = para.substr(para.find("=") + 1);
		} else if (para.find("--load=") == 0) {
			load = para.substr(para.find("=") + 1);
		} else if (para.find("--save=") == 0) {
			save = para.substr(para.find("=") + 1);
		} else if (para.find("--summary") == 0) {
			summary = true;
		} else if (para.find("--loadw=") == 0) {
			loadw = para.substr(para.find("=") + 1);
		} else if (para.find("--savew=") == 0) {
			savew = para.substr(para.find("=") + 1);
		} else if (para.find("--nolearn") == 0) {
			LEARN_ENABLE = false;
		} else if (para.find("--lr") == 0) {
			//printf("%s\n", para.substr(para.find("=")+1).c_str());
			lr_init = std::stof(para.substr(para.find("=")+1));
		}
	}

	statistic stat(total, block, limit);

	if (load.size()) {
		std::ifstream in(load, std::ios::in);
		in >> stat;
		in.close();
		summary |= stat.is_finished();
	}

	if (loadw.size()) {
		std::ifstream in(loadw, std::ios::in);
		in >> tnet;
		in.close();
	}

	player play(play_args);
	rndenv evil(evil_args);

	//if(LEARN_ENABLE) tnet.reset(N);

	while (!stat.is_finished()) {
		play.open_episode("~:" + evil.name()); //no realize virtual wtf
		evil.open_episode(play.name() + ":~");


		stat.open_episode(play.name() + ":" + evil.name());
		episode& game = stat.back(); // = data(list of eps).back()

		last_op = -1; //init last_op
		while (true) {
			agent& who = game.take_turns(play, evil); //see who should play
			action move = who.take_action(game.state()); //agent.take_action return action
			if (game.apply_action(move) != true) break; //actually do action
			if (who.check_for_win(game.state())) break;
		}
		agent& win = game.last_turns(play, evil);
		stat.close_episode(win.name());

		play.close_episode(win.name());
		evil.close_episode(win.name());

		if(LEARN_ENABLE){
			tnet.fit_ep(stat.back(), lr_init*decay); //lr adjustment?? //0.1/32 worked 0.1/8 diverged
			ep_count++;
			if(ep_count%decay_ep == 0) decay = decay * decay_rate;
		//64better than 32, 0 better than 16 and 32...
		//improvement record? 
		}
	}

	if (summary) {
		stat.summary();
	}

	//std::cout << stat;

	if (savew.size()) {
		std::ofstream out(savew, std::ios::out | std::ios::trunc);
		out << tnet;
		out.close();
	}

	if (save.size()) {
		std::ofstream out(save, std::ios::out | std::ios::trunc);
		out << stat;
		out.close();
	}

	return 0;
}
