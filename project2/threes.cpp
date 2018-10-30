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

extern const int N;
extern const int N_TUPLE;
extern const int N_isomorphism;

int last_op = -1;
n_tuple_net tnet(N);

int main(int argc, const char* argv[]) {
	std::cout << "Threes-Demo: ";
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl << std::endl;

	size_t total = 1000, block = 0, limit = 0;
	std::string play_args, evil_args;
	std::string load, save;
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
		}
	}

	statistic stat(total, block, limit);

	if (load.size()) {
		std::ifstream in(load, std::ios::in);
		in >> stat;
		in.close();
		summary |= stat.is_finished();
	}

	player play(play_args);
	rndenv evil(evil_args);

	while (!stat.is_finished()) {
		play.open_episode("~:" + evil.name()); //no realize virtual wtf
		evil.open_episode(play.name() + ":~");


		stat.open_episode(play.name() + ":" + evil.name());
		episode& game = stat.back(); // = data(list of eps).back()

		last_op = -1; //init last_op
		while (true) {
			agent& who = game.take_turns(play, evil); //see who should play
			action move = who.take_action(game.state()); //agent.take_action return action
			//
			//std::cout << move << std::endl; //will output in save file anyway
			//std::cout << game.state(); //
			if (game.apply_action(move) != true) break; //actually do action
			if (who.check_for_win(game.state())) break;
		}
		agent& win = game.last_turns(play, evil);
		stat.close_episode(win.name());

		play.close_episode(win.name());
		evil.close_episode(win.name());

		tnet.fit_ep(stat.back(), 0.1/64); //lr adjustment?? //0.1/32 worked 0.1/8 diverged
		//64better than 32, 0 better than 16 and 32...
	}

	if (summary) {
		stat.summary();
	}

	//std::cout << stat;//forget if its my addition or original, checked my addition

	if (save.size()) {
		std::ofstream out(save, std::ios::out | std::ios::trunc);
		out << stat;
		out.close();
	}

	return 0;
}
