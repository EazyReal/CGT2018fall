#pragma once //?
#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <iterator>
#include <string>
#include <cmath>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistic.h"

//instrustions
//train with td(0)

const int N = pow(15, 4);
const int N_TUPLE = 8;
const int N_isomorphism = 8; //rotattions + mirrored rotations
//typedef std::vector<float> vec;
//typedef std::vector<row> vec_r;
//typedef std::map<board, int>;
//vec b2keys(const board& b){ }//pass by value ok?

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
	typedef std::array< std::array<int, N_TUPLE>, N_isomorphism> fids; 

	void reset(size_t len){
		for(int i = 0 ; i < N_TUPLE ; i++ ) w[i] = weight(len);
	}

	static inline int num2id(int num){
		if( num >= 0 && num <= 3) return num;
		return log2(num/3) + 3; //shoud work
	}

	static fids get_ids(board::board b){
		fids ids;

		for(int ii = 0; ii < N_isomorphism/2 ; ii++)
		{
			b.rotate_right();
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, i)*num2id(b[i][j]);
				ids[ii][i] = sum;
			}
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, i)*num2id(b[j][i]);
				ids[ii][i+4] = sum;
			}
		}
		b.transpose(); //can we do this by just sum*2?
		for(int ii = N_isomorphism/2; ii < N_isomorphism ; ii++)
		{
			b.rotate_right();
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, i)*num2id(b[i][j]);
				ids[ii][i] = sum;
			}
			for(int i = 0 ; i < 4 ; i++ ){
				int sum = 0;
				for(int j = 0 ; j < 4 ; j++ ) sum += pow(15, i)*num2id(b[j][i]);
				ids[ii][i+4] = sum;
			}
		}
		b.transpose();

		return ids;
	}

	//void train(int ep_nums = 1000){} write in main?

	void fit_ep (episode ep, float alpha = 0.1f/N_TUPLE) { //train with eps or create eps?
		const auto& moves = ep.ep_moves;
		const auto& states = ep.ep_states;

		//mbi[get_id(states.back())] = 0; //terminal case td target = 0 => train to 0
		float vlast[N_isomorphism][N_TUPLE] = {0.0}; //for update
		int k = ( moves.back().code.type() == action::type_flag('s') ) ? 3 : 4 ; //size-k is first after (slide) state
		fids ids = get_ids(states[states.size() - k + 2]);
		for(int j = 0 ; j < N_isomorphism ; j++){ //actually as a*b 
			for(int jj = 0 ; jj < N_TUPLE ; jj++)
			{
				float& vt = w[jj][ids[j][jj]];
				vt = (1.0 - alpha) * vt;
				vlast[j][jj] = vt;
			}
		}

		for(int i = states.size() - k ; i > 9 ; i -= 2){ //must have 9 board with placement, 0 = {0}, 9 = after 9th placement
			fids ids = get_ids(states[i]);
			for(int j = 0 ; j < N_isomorphism ; j++){ //actually as a*b 
				for(int jj = 0 ; jj < N_TUPLE ; jj++)
				{
					float& vt = w[jj][ids[j][jj]];
					vt = vt + alpha * (vlast[j][jj] + moves[i-1].reward - vt);
					vlast[j][jj] = vt;
				}
			}
		} 
	}

protected:
	//std::map<board::board, int> mbi[i]; //should be row  
	weight w[N_TUPLE];
};
