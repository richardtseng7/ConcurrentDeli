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

using namespace std;

struct orders {
	int sandwichNum;
	int cashierNum;
};

std::vector<orders> corkboard;
int max_orders;
int numCashier; 
std::map<int, std::vector<int> > cashierMap; 

unsigned int hasRoom = 2;
unsigned int isFull = 3;
int lastOrder = - 1;
int waiters = 0;

// helper method checks if cashier has posted on corkboard
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


void sandwichMaker(void* arg){
	thread_lock(1);
	while (numCashier != 0){
		//DEBUG
		printf("sandwichMaker \n");
		for (int i = 0; i < corkboard.size(); i++){
			printf("Cashier %d: Sandwich %d, \n", corkboard[i].cashierNum, corkboard[i].sandwichNum);
		}

		// while there is still room in corkboard, let cashier add orders
		while(((int) corkboard.size()) < max_orders){ 
			thread_broadcast(1, hasRoom);
			thread_wait(1, isFull);
		}
		// find most similar sandwich to make
		int min = INT_MAX;
		int ind;
		int currCashier;
		int currSandwich;
		for (int i = 0; i < ((int) corkboard.size()); i++){
			int diff = abs(lastOrder - corkboard[i].sandwichNum);
			if (diff < min){ 
				min = diff;
				ind = i;
				currCashier = corkboard[i].cashierNum;
				currSandwich = corkboard[i].sandwichNum;
			}
		}
		lastOrder = currSandwich;
		thread_broadcast(1, hasRoom);
		if (waiters > 0){
			waiters = waiters - 1;
			thread_signal(1, (currCashier + 100));
		}
		//print currSandwich number & currCashierID
		cout << "READY: cashier " << currCashier << " sandwich " << currSandwich << endl; 
		//remove last sandwich order from corkboard
		corkboard.erase(corkboard.begin() + ind);
		
	}	
	thread_unlock(1);
}


void cashiers(void* cashierid){
	thread_lock(1);	
	int cashierID = (intptr_t) cashierid;
	//while there are still orders
	while (cashierMap[cashierID].size() > 0){
	
		// cashier already has an order on corkboard
		while (ifContains((intptr_t) cashierID)) {
			waiters++;
			thread_wait(1, (cashierID + 100));
		}
		while (((int)corkboard.size()) == max_orders){
			thread_signal(1, isFull);
			thread_wait(1, hasRoom);
		}
		if (numCashier < max_orders){
			max_orders = numCashier;
		}	

		printf("corkboard size: %d, max_orders: %d", (int) corkboard.size(), max_orders);
		printf("Cashier %d:", cashierID);
		for(int i = 0; i < cashierMap[cashierID].size(); i++){
			printf("%d,", cashierMap[cashierID][i]);
		}
		printf("\n");
		// add order from cashierMap to corkboard
		orders toAdd;
		toAdd.cashierNum = cashierID;
		toAdd.sandwichNum = cashierMap[cashierID][0];
		corkboard.push_back(toAdd);
		cashierMap[cashierID].erase(cashierMap[cashierID].begin());

		// print sandwich order
		cout << "POSTED: cashier " << toAdd.cashierNum << " sandwich " << toAdd.sandwichNum << endl; 
	}
	numCashier = numCashier - 1;
	if (corkboard.size() > max_orders){
		thread_signal(1, isFull);
	}
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
	//std::vector<orders> corkboard;
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