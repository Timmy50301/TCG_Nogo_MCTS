/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games (TCG 2021)
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include <fstream>
#include <vector> 
#include <iostream>

class agent {
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

/**
 * base agent for agents with randomness
 */
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
 * random player for both side
 * put a legal piece randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
		space(board::size_x * board::size_y), who(board::empty) {
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") who = board::black;
		if (role() == "white") who = board::white;
		if (who == board::empty)
			throw std::invalid_argument("invalid role: " + role());
		for (size_t i = 0; i < space.size(); i++)
			space[i] = action::place(i, who);
	}
/*
	virtual action take_action(const board& state) {
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal)
				return move;
		}
		return action();
	}
*/
	virtual action take_action(const board& state) {

		float lamda;						//ucb公式的係數
		int expand;							//展開的node數目（估計）與時間正相關
		int times;
		int simulation;						//rollout次數
		//prior technique
		std::vector<int> vec;
		for(int i=0 ; i<81 ; i++){
			board tes = state;
			if(tes.place(i)==board::legal){
				vec.push_back(i);
			}
		}
		// board h;
		// h = state;
		// if(h.place(7)==board::legal){return action::place(7, who);}
		// h = state;
		// if(h.place(17)==board::legal){return action::place(17, who);}
		// h = state;
		// if(h.place(79)==board::legal){return action::place(79, who);}
		// h = state;
		// if(h.place(71)==board::legal){return action::place(71, who);}
		// h = state;
		// if(h.place(1)==board::legal){return action::place(1, who);}
		// h = state;
		// if(h.place(9)==board::legal){return action::place(9, who);}
		// h = state;
		// if(h.place(73)==board::legal){return action::place(73, who);}
		// h = state;
		// if(h.place(63)==board::legal){return action::place(63, who);}

		if(vec.size() >= 50){
			std::random_shuffle(vec.begin(), vec.end());
			return action::place(vec[0], who);
		}else if(vec.size() >=10){
			lamda = 1.414;
			expand = 250;
			times = 1;
			simulation = 50;
		}else {
			lamda = 1.414;
			expand = 100;
			times = 1;
			simulation = 50;
		}
		vec.clear();

		
		
		//nodetree
		struct node {
			int layer;
			int parent;						//parent's 編碼
			std::vector<int> children;		//所有childrens' 編碼存進vector
			int move;						//0~80
			board instant;
			int visit;
			int win;						//I win
			float ucb;
		};
		std::vector<node> nodetree;
		nodetree.clear();


		//
		node n;
		nodetree.push_back(n);
		nodetree[0].instant = state;
		nodetree[0].layer = 0;

		int op=1;
		int oq1;
		int oq2;
		bool stop = true;
		board s0;						//盤面

		int number = 1;					//node的編碼(已經是nodetree的index了)
		int a = 0;						//layer頭的編碼
		int b = 0;						//layer尾的編碼

		//展開到剛好超過(int expand)個node with legal move（以layer為單位）
		while(number<expand){								
			stop = true;
			for(int p=a ; p<=b ; p++){					//p => parent
				for(int m=0 ; m<81 ; m++){				//m => move
					s0 = nodetree[p].instant;
					if(s0.place(m)==board::legal){
						nodetree.push_back(n);
						nodetree[number].instant = s0;		
						nodetree[number].move = m;
						nodetree[number].parent = p;
						nodetree[p].children.push_back(number);
						nodetree[number].layer = nodetree[p].layer+1;
						nodetree[number].visit = 0;
						nodetree[number].win = 0;
						number ++;
						stop = false;
					}
				}
			}
			if(stop==true){
				oq1 = b+1;
				oq2 = number-1;						//oq => lastlayer的編碼為oq1~oq2
				break;
				}
			if(a==0){op =number-1;}					//op => layer1的編碼為1~op
			a = b+1;
			b = number-1;
			oq1 = a;
			oq2 = b;
		}


		//nodetree 從index(1 到 number-1) 全部rollout一次
		std::vector<int> valid;
		for(int z=0 ; z<times ; z++){
			for(int n=1 ; n<number ; n++){					//n => node編號
				board next = nodetree[n].instant;
				int lay = nodetree[n].layer;				//lay => 終局在layer幾
				
				while(1){
					valid.clear();
					for(int i=0 ; i<81 ; i++){
						board judge = next;
						if(judge.place(i) == board::legal){
							valid.push_back(i);
						}
					}
					if(valid.empty() == 1){break;}
					std::random_shuffle(valid.begin(), valid.end());
					next.place(valid[0]);
					lay ++;
				}
				//終局在奇數layer(lay==odd)=> black wins
				nodetree[n].visit++;
				if(lay%2!=0){nodetree[n].win++;}
			}
		}
		/*
		//nodetree expand 完的最後一層layer 各rollout "times"次
		for(int z=0 ; z<times ; z++){
			for(int n=oq1 ; n<=oq2 ; n++){					//n => node編號
				board next = nodetree[n].instant;
				int lay = nodetree[n].layer;				//lay => 終局在layer幾
				
				while(1){
					valid.clear();
					for(int i=0 ; i<81 ; i++){
						board judge = next;
						if(judge.place(i) == board::legal){
							valid.push_back(i);
						}
					}
					if(valid.empty() == 1){break;}
					std::random_shuffle(valid.begin(), valid.end());
					next.place(valid[0]);
					lay ++;
				}
				//終局在奇數layer(lay==odd)=> black wins
				nodetree[n].visit++;
				if(lay%2!=0){nodetree[n].win++;}
			}
		}
		*/




		//backpropagate(visit&win)
		for(int n=number-1 ; n>0 ; n--){
			nodetree[nodetree[n].parent].visit += nodetree[n].visit;
			nodetree[nodetree[n].parent].win += nodetree[n].win;
		}
		//calculate UCB
		for(int n=1 ; n<number ; n++){
			if(nodetree[n].layer%2!=0){		//layer為奇數以自己角度算ucb
				nodetree[n].ucb = nodetree[n].win/nodetree[n].visit + lamda*sqrt(log(nodetree[nodetree[n].parent].visit)/nodetree[n].visit);
			}
			if(nodetree[n].layer%2==0){		//layer為偶數以對手角度算ucb
				nodetree[n].ucb = (nodetree[n].visit-nodetree[n].win)/nodetree[n].visit + lamda*sqrt(log(nodetree[nodetree[n].parent].visit)/nodetree[n].visit);
			}
		}

/*
		for(int iteration=1 ; iteration<=simulation ; iteration++){
			//找出要進行rollout的number =bestnum
			float bestucb = 0;
			int bestnum = 1;
			for(int i=1 ; i<=op ;i++){
				if(nodetree[i].ucb >= bestucb){
					bestucb = nodetree[i].ucb;
					bestnum = i;
				}
			}
			int nextbestnum = bestnum;
			while(1){
				bestucb = 0;
				if(nodetree[bestnum].children.empty() == 1){break;}
				for(int i=0 ; i<int(nodetree[bestnum].children.size()) ; i++){
					if(nodetree[nodetree[bestnum].children[i]].ucb >= bestucb){
						bestucb = nodetree[nodetree[bestnum].children[i]].ucb;
						nextbestnum = nodetree[bestnum].children[i];
					}
				}
				bestnum = nextbestnum;
			}
			//從nodetree[bestnumber].instance進行rollout
			board next = nodetree[bestnum].instant;
			int lay = nodetree[bestnum].layer;					//lay => 終局在layer幾
			while(1){
				valid.clear();
				for(int i=0 ; i<81 ; i++){
					board judge = next;
					if(judge.place(i) == board::legal){
						valid.push_back(i);
					}
				}
				if(valid.empty() == 1){break;}
				std::random_shuffle(valid.begin(), valid.end());
				next.place(valid[0]);
				lay ++;
			}
			//backpropagate
			while(1){
				nodetree[bestnum].visit ++;
				if(lay%2!=0){						//終局在奇數layer(lay==odd)=> black wins
					nodetree[bestnum].win ++;
				}
				if(bestnum==0){break;}
				bestnum = nodetree[bestnum].parent;
			}
			//update UCB
			for(int n=1 ; n<number ; n++){
				if(nodetree[n].layer%2!=0){		//layer為奇數以自己角度算ucb
					nodetree[n].ucb = nodetree[n].win/nodetree[n].visit + lamda*sqrt(log(nodetree[nodetree[n].parent].visit)/nodetree[n].visit);
				}
				if(nodetree[n].layer%2==0){		//layer為偶數以對手角度算ucb
					nodetree[n].ucb = (nodetree[n].visit-nodetree[n].win)/nodetree[n].visit + lamda*sqrt(log(nodetree[nodetree[n].parent].visit)/nodetree[n].visit);
				}
			}
		}
*/

		//選擇最佳步
		float finalbestucb = 0;
		int finalbestmove = 0;
		for(int n=1 ; n<=op ; n++){					//layer1的編碼為1~op
			if(nodetree[n].ucb>=finalbestucb){
				finalbestucb = nodetree[n].ucb;
				finalbestmove = nodetree[n].move;
			}
		}
		return action::place(finalbestmove, who);


/*
//test
		for(int i=0 ; i<81 ; i++){
			board hhh=state;
			if(hhh.place(i)==board::legal){
				return action::place(i, who);
			}
		}
*/

		return action();
	}

private:
	std::vector<action::place> space;
	board::piece_type who;
};
