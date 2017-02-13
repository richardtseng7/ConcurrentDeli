#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "thread.h"
#include <map>
#include <math.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>

struct orders {
	int sandwichNum;
	int cashierNum;
};

using namespace std;

std::vector<orders> corkboard;
int max_orders;
int numCashier; 
std::map<int, std::vector<int> > cashierMap; 
unsigned int hasRoom = 2;
unsigned int isFull = 3;
unsigned int cashierExists = 4;
int lastOrder = - 1;

// helper method checks if cashier
bool ifContains(int cashierID) {
	if (corkboard.empty()) {
		return false;
	}
	for (int i = 0; i < corkboard.size(); i++) {
		orders curOrder = corkboard[i];
		int curID = curOrder.cashierNum;
		if (curID == cashierID) {
			return true;
		}
	}
	return false;
}

void cashiers(void* cashierid){
	int cashierID = (intptr_t) cashierid;
	
	// while( cashierMap[cashierID].size > 0) {
	thread_lock(1);	// corkboard is full
	while (corkboard.size() == max_orders){
		thread_wait(1, hasRoom);
	}
	// cashier already has an order on corkboard
	while (ifContains((intptr_t) cashierID)) {
		thread_yield();
	}
	// cashier is done (has no more orders left)
	if (cashierMap[cashierID].size() == 0){
		max_orders = max_orders - 1;
		thread_yield();
	}
	// add order from cashier to corkboard
	orders toAdd;
	toAdd.cashierNum = cashierID;
	toAdd.sandwichNum = cashierMap[cashierID][0];
	corkboard.push_back(toAdd);
	// print sandwich order
	cout << "POSTED: cashier " << toAdd.cashierNum << " sandwich " << toAdd.sandwichNum << endl; 
	if (corkboard.size() == max_orders){
		thread_signal(1, isFull);
	}
	thread_unlock(1);
	// }
}


void sandwichMaker(void* arg){
	thread_lock(1);
	


	if (corkboard.size() == 0){
		return;
	}
	while(corkboard.size() < max_orders){ // while there is still room in corkboard, let cashier add orders
		thread_wait(1, isFull);
	}
	// find most similar sandwich to make
	int min = INT_MAX;
	int currCashier;
	int ind;
	int currSandwich;
	for (int i = 0; i < corkboard.size(); i++){
		int diff = abs(lastOrder - currSandwich);
		if (diff < min){ 
			min = diff;
			currCashier = corkboard[i].cashierNum;
			currSandwich = corkboard[i].sandwichNum;
			ind = i;
		}
	}
	//print cur sandwich number & cur cashier id
	cout << "READY: cashier " << currCashier << " sandwich " << currSandwich << endl; 
	lastOrder = currSandwich;
	//remove currSandwich from the map
	cashierMap[currCashier].erase(cashierMap[currCashier].begin());
	//remove last sandwich order from corkboard
	corkboard.erase(corkboard.begin() + ind);
	//broadcast that now the corkboard hasRoom!
	thread_broadcast(1, hasRoom);
	thread_unlock(1);
}


void cashierInit(void* arg){
	// initialize all cashiers threads
	for (int i = 0; i < numCashier; i++){ 
		thread_create((thread_startfunc_t) cashiers, (void *) (intptr_t) i);
	}
	// initialize sandwich maker thread
	thread_create((thread_startfunc_t) sandwichMaker, (void *) 14);
}


int main(int argc, char* argv[]) {
	std::vector<orders> corkboard;
	max_orders = atoi(argv[1]);
	numCashier = argc - 2;
	if (numCashier == 0) {
		return 0;
	}
	//add sandwich orders to map here
	for (int i = 2; i < argc; i++) {
		std::vector<int> arrList; // makes arraylist of sandwichNums for each cashier
		string line;
		ifstream myFile (argv[i]); // opens file
		// adds all sandwich numbers to arrList
		if (myFile.is_open()) { 
			while (getline(myFile, line)) { // loop through all lines of oepn file
				int n = atoi(line.c_str()); // converts string to int
				arrList.push_back(n); // adds sandwichNum to arrList
			}
			cashierMap[i - 2] = arrList; // maps cashier number to corresponding sandwich orders
		}
	}
	thread_libinit((thread_startfunc_t) cashierInit, (void *) 7);
}