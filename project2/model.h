#pragma once //?
#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <iterator>
#include <string>
#include <cmath>
#include <cassert>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"

//instrustions
//n_tuple net trained with td(0)

//824 7719 @ 5000ep with 1/320 0.9/100ep

const int N = pow(15, 4) + 500;
const int N_TUPLE = 8;
const int N_isomorphism = 8; //rotattions + mirrored rotations

class weight {
friend class agent;
public:
	weight() {}
	weight(size_t len) : value(len) {}
	weight(weight&& f) : value(std::move(f.value)) {}
	weight(const weight& f) = default;

	weight& operator =(const weight& f) = default;
	float& operator[] (size_t i) { return value[i]; }
	const float& operator[] (size_t i) const { return value[i]; }
	size_t size() const { return value.size(); }

public:
	friend std::ostream& operator <<(std::ostream& out, const weight& w) {
		auto& value = w.value;
		uint64_t size = value.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
		out.write(reinterpret_cast<const char*>(value.data()), sizeof(float) * size);
		return out;
	}
	friend std::istream& operator >>(std::istream& in, weight& w) {
		auto& value = w.value;
		uint64_t size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
		value.resize(size);
		in.read(reinterpret_cast<char*>(value.data()), sizeof(float) * size);
		return in;
	}

protected:
	std::vector<float> value;
};

class n_tuple_net {
public:
	n_tuple_net() {}
	n_tuple_net(size_t len){ for(int i = 0 ; i < N_TUPLE ; i ++) w[i] = weight(len); } //initialize with len of zeros, map clear
	//n_tuple_net(const weight& f){ w = f; }
	n_tuple_net(const n_tuple_net& net) = default;

public:
	typedef std::array< std::array<int, N_TUPLE>, N_isomorphism> fids; //a[iso][tuple]

	void reset(size_t len){
		for(int i = 0 ; i < N_TUPLE ; i++ ) w[i] = weight(len);
	}

	static inline int num2id(int num){
		if( num >= 0 && num <= 3) return num;
		else return int(log2(num/3)) + 3; 
	}

	static fids get_ids(board b){
		fids ids;

		for(int i_iso = 0; i_iso < N_isomorphism/2 ; i_iso++)
		{
			b.rotate_right();
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) {sum += pow(15, j)*num2id(b[i][j]);}
				ids[i_iso][i] = sum;
			}
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) {sum += pow(15, j)*num2id(b[j][i]);}
				ids[i_iso][i+4] = sum;
			}
		}
		b.transpose(); //can we do this by i_isoust sum*2?
		for(int i_iso = N_isomorphism/2; i_iso < N_isomorphism ; i_iso++)
		{
			b.rotate_right();
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, j)*num2id(b[i][j]);
				ids[i_iso][i] = sum;
			}
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, j)*num2id(b[j][i]);
				ids[i_iso][i+4] = sum; //bug w
			}
		}
		b.transpose();


		return ids;
	}

	inline float get_v(const fids& ids)
	{
		float ret = 0.0;
		//can use a single iso to evaluate?
		//for(int i_tup = 0 ; i_tup < N_TUPLE ; i_tup++) { ret += w[i_tup][ids[0][i_tup]];}
		for(int i_iso = 0 ; i_iso < N_isomorphism ; i_iso++) for(int i_tup = 0 ; i_tup < N_TUPLE ; i_tup++) { ret += w[i_tup][ids[i_iso][i_tup]];}
		//printf("%f\n", ret); //lr = 0.1/8 doesnt converge!!!! 0+ diverge

		return ret;
	}

	//void train(int ep_nums = 1000){} write in main?

	void fit_ep (episode ep, float alpha = 0.1f/N_TUPLE) { //why not n_iso, in ptt
		const auto& moves = ep.ep_moves;
		const auto& states = ep.ep_states;
		// ep0(init) mv0 ep1 mv1 ... mvn epn+1

		float vnow;
		float td_target;

		//int k = (moves.back().code.type() == action::type_flag('s') ) ? 3 : 4 ; //size-k is second last after (slide) state
		int k = ((states.size()-1) % 2 == 0) ? 3 : 4; //even check

		fids ids = get_ids(states[states.size() - k + 2]);
		vnow = get_v(ids);
		for(int i_iso = 0 ; i_iso < N_isomorphism ; i_iso++){ //actually as a*b 
			for(int i_tup = 0 ; i_tup < N_TUPLE ; i_tup++)
			{
				float& vt = w[i_tup][ids[i_iso][i_tup]];
				vt += alpha * (0.0 - vnow); //td-target = 0.0
			}
		}

		for(int i = states.size() - k ; i > 9 ; i -= 2){ //must have 9 board with placement, 0 = {0}, 9 = after 9th placement, 10 = first after (slide) state
			fids ids = get_ids(states[i]);
			vnow = get_v(ids);
			//std::assert(moves[i+1].code.type() == action::type_flag('s'));
			td_target = get_v(get_ids(states[i+2])) + moves[i+1].reward;
			//reward is good, get_v diverge
			for(int i_iso = 0 ; i_iso < N_isomorphism ; i_iso++) for(int i_tup = 0 ; i_tup < N_TUPLE ; i_tup++)
			{
				float& vt = w[i_tup][ids[i_iso][i_tup]];
				vt += alpha * (td_target - vnow); 
			}
			//vlast = get_v(ids);
		} 
	}
public:
	friend std::ostream& operator <<(std::ostream& out, const n_tuple_net& tnet) {
		auto& w = tnet.w;
		for(int i = 0 ; i < N_TUPLE ; i++) out << w[i];
		return out;
	}
	friend std::istream& operator >>(std::istream& in, n_tuple_net& tnet) {
		auto& w = tnet.w;
		for(int i = 0 ; i < N_TUPLE ; i++) in >> w[i];
		return in;
	}

public:
	//std::map<board::board, int> mbi[i]; //should be row  
	weight w[N_TUPLE];
};
