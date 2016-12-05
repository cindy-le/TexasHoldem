/*
PokerGame.cpp created by Cindy Le, Adrien Xie, and Yanni Yang
*/


#include "stdafx.h"
#include "PokerGame.h"
#include "stdlib.h"

//#define cout std::cout //cout is not ambiguous

using namespace std;

//A default constructor for fiveCardDraw that initializes dealer to be the first person and discard to be empty. 
PokerGame::PokerGame() {
	dealer = 0;
	pot = 0;
	ante = 1;
	bet = 0;

	discardDeck = Deck();
	deck = Deck();
	deck.standardized(); //52 cards
}

//A method that asks the user cards to discard and then move them to discarDeck.
int PokerGame::discardCards(Player& p) {
	//skip if the player has already folded
	if (p.isFold) return 0;

	cout << endl;
	cout << p << endl;
	cout << "Card to discard? Enter the indices separated by space in a line. " << endl;

	vector<bool> ifDelete;
	string toDiscard; //user response

					  //remove card correspondingly
	if (p.isAuto) {
		ifDelete = p.hand.discardIndex();
		for (size_t k = 0; k < MAX_CARDS_IN_HAND; k++) {
			if (ifDelete[k]) {
				cout << k + 1 << " ";
			}
		}
	}
	else { //wait for user input
		ifDelete = { false,false,false,false,false };
		while (toDiscard.length() == 0) getline(cin, toDiscard);
		toDiscard = " " + toDiscard + " ";
		for (size_t k = 1; k <= MAX_CARDS_IN_HAND; k++) {
			if (toDiscard.find(" " + to_string(k) + " ") != string::npos) {
				ifDelete[k - 1] = true;
			}
		}
	}

	//remove the card from the player to the discard desk
	for (int i = MAX_CARDS_IN_HAND - 1; i >= 0; i--) {
		if (ifDelete[i]) {
			discardDeck.addCard(p.hand[i]);
			p.hand.removeCard(i);
		}
	}

	return 0;
}

//A method that deals as many as that they have discarded.
int PokerGame::dealUntilFull(Player& p) {
	for (size_t i = p.hand.size(); i < MAX_CARDS_IN_HAND; i++) {
		if (deck.size() == 0) {
			if (discardDeck.size() == 0) throw NO_CARD_TO_DEAL; //when both decks are empty
			discardDeck.shuffle();
			p.hand << discardDeck; //deal from the discarded when no card in the main deck
		}
		else {
			p.hand << deck;
		}
	}
	return 0;
}

//A public virtual before_round method that shuffles and then deals one card at a time from the main deck.
int PokerGame::before_round() {
	deck.shuffle();
	int len = players.size();

	// each player place an ante of one chip to the pot
	for (int i = 0; i < len; i++) {
		pot += payChips(*players[i], ante);
	}

	return 0;
}

//round() will be implemented in subclasses

//A public virtual after_round method that sorts players by hand rank, recycles all cards, and asks whether to leave or join.
int PokerGame::after_round() {
	int len = players.size();

	//sort a temporary vector of pointers to players (a copy of the vector member variable)
	vector<shared_ptr<Player>> tempPlayers;
	for (int i = 0; i < len; i++) {
		tempPlayers.push_back(players[i]);
	}
	sort(tempPlayers.begin(), tempPlayers.end(), handCompare);

	//print out player ranks after sorting
	cout << endl;
	for (int i = 0; i < len; i++) {
		after_turn(*tempPlayers[i]);
	}

	//find winner's combo
	int maxIndex = -1;
	for (int i = len - 1; i >= 0; i--) {
		if (tempPlayers[i]->isFold == false) {
			maxIndex = i;
			break;
		}
	}
	if (maxIndex == -1) throw NO_ACTIVES;
	int maxHash = tempPlayers[maxIndex]->hand.hashHand();

	//calculate wins and losses
	vector<shared_ptr<Player>> winners;
	for (int i = len - 1; i >= 0; i--) {
		if ((players[i]->isFold == false) && (players[i]->hand.hashHand() == maxHash)) {
			++players[i]->won;
			players[i]->chip += players[i]->bet;
			winners.push_back(players[i]);
		}
		else {
			++players[i]->lost;
			pot += players[i]->bet;
		}

		//reset variables for each player
		players[i]->bet = 0;
		players[i]->isFold = false;

		//move cards from players to the main deck
		for (int j = players[i]->hand.size() - 1; j >= 0; j--) {
			deck.addCard(players[i]->hand[j]);
			players[i]->hand.removeCard(j);
		}
	}

	//calculate number of winners
	int numOfWinners = winners.size();
	if (numOfWinners == 0) throw NO_WINNERS;
	cout << endl;
	cout << numOfWinners << " winner(s) sharing " << pot << " chips: " << endl;

	//distribute chips from the pot to winner(s)
	int part = (int)floor(pot / numOfWinners);
	for (int i = 0; i < numOfWinners; i++) {
		winners[i]->chip += part;
		cout << winners[i]->name << endl;
	}

	//reset variables for the game
	pot = 0;
	bet = 0;

	//move all cards from discardDeck to the main deck
	while (discardDeck.size() > 0) {
		deck.addCard(discardDeck.popCard());
	}

	//some auto players leave the game
	autoPlayerLeave();

	//ask players who have zero chips
	for (int i = 0; i < len; i++) {
		if (players[i]->chip == 0) {
			string quitName = players[i]->name;
			//re-add the player
			players.erase(players.begin() + i);
			addPlayer(quitName); //where zero-chip condition is checked
		}

	}

	//ask the rest of the players whether to leave the game
	string checktemp;
	string quitName;
	ofstream output;
	int quitIndex = -1;
	bool findNo;
	do {

		cout << endl;
		cout << "Any player leaving? Enter 'No' for nobody. " << endl;
		//assume that no player is named 'no'
		cout << "Player's name: " << endl;
		cin >> quitName;
		checktemp = quitName;
		transform(quitName.begin(), quitName.end(), checktemp.begin(), ::tolower); //accept 'NO' 'no' 'No' 'nO'
		if (checktemp.find("no") != string::npos && checktemp.length() == 2) {
			findNo = true;
		}
		else if (checktemp.find("no*") != string::npos && checktemp.length() == 3) { //check for "no* "
			findNo = true;
		}
		else {
			findNo = false;
		}
		if (!findNo) {
			int len = players.size();
			for (int i = 0; i < len; i++) {
				if (quitName == (*players[i]).name) {
					quitIndex = i;
				}
			}

			if (quitIndex != -1) {
				players.erase(players.begin() + quitIndex);
			}
			else {
				cout << "The player " << quitName << " is not currently playing!" << endl;
			}
		}

	} while (!findNo);

	//ask whether to join the game
	string joinName;

	do {
		cout << endl;
		cout << "Any player joining? Enter 'No' for nobody. " << endl;
		cout << "Player's name: " << endl;
		cin >> joinName;
		checktemp = joinName;
		transform(joinName.begin(), joinName.end(), checktemp.begin(), ::tolower); //accept 'NO' 'no' 'No' 'nO'
		if (checktemp.find("no") != string::npos && joinName.length() == 2) {
			findNo = true;
		}
		else if (checktemp.find("no*") != string::npos && checktemp.length() == 3) { //check for "no* "
			findNo = true;
		}
		else {
			findNo = false;
		}
		if (!findNo) addPlayer(joinName); //it checks for duplicates
	} while (!findNo);

	cout << endl;

	//Switch to next dealer
	if (players.size() != 0) dealer = (dealer + 1) % players.size();

	return 0;
}

int PokerGame::bet_in_turn() {
	size_t len = players.size();
	if (len == 0) throw NO_PLAYERS;

	int active; // the current number of unfolded players
	size_t raiser = 0; // the last player who raises the bet; used to determine if betting phase ends
	size_t i = 0; // the current player to ask for response
	bool finish = false; // whether the betting phase should be terminated

	do {
		unsigned int temp = betChips(*players[i]);
		if (temp > 0) {
			pot += temp;
			raiser = i;
		}

		active = countActive();
		if (active <= 1) finish = true;
		i = (i + 1) % len;
		if (i == raiser) finish = true;

	} while (!finish); // two breaks

	return 0;
}

// Claculate numer of chips a player pays.
unsigned int PokerGame::payChips(Player& p, unsigned int amount) {
	int payed = amount;
	if (p.chip > amount) { //add the amount equal to ante
		p.chip -= amount;
	}
	else { //add all the chips
		payed = p.chip;
		p.chip = 0;
	}
	return payed;
}

// Ask and deduct chips from player and add to a temporary bet variable
unsigned int PokerGame::betChips(Player& p) {
	//skip if the player has already folded
	if (p.isFold) return 0;

	//range for the number of chips valid to bet
	int min = bet;
	int max;

	cout << endl;
	cout << p << endl;

	//ask player if all-in
	if (bet >= p.chip) {
		cout << "Please enter '" << p.chip << "' for ALL-IN, or 'f' for FOLD: " << endl;
		max = p.chip;
		//no one has bet yet
	}
	else if (bet == 0) {
		max = 2;
		cout << "Please enter '0' for CHECK, '1' or '2' for BET, or 'f' for FOLD: " << endl;
	}
	//someone has bet already
	else if (bet + 1 >= p.chip) {
		max = bet + 1;
		cout << "Please enter " << bet << " for CALL, '" << bet + 1 << " for RAISE, or 'f' for FOLD: " << endl;
	}
	else {
		max = bet + 2;
		cout << "Please enter " << bet << " for CALL, '" << bet + 1 << "' or '" << bet + 2 << "' for RAISE, or 'f' for FOLD: " << endl;
	}

	string str;
	int num = -1;

	//get the number to bet from user input
	do {
		getline(cin, str);
		bool findNo = (str.find("f") != string::npos) && (str.length() == 1);
		if (findNo) {
			num = 0;
			p.isFold = true;
			break;
		}
		else if (str.length() != 0) {
			num = atoi(str.c_str());
		}
	} while (num < min || num > max);

	unsigned int numToBet = (unsigned int)num;
	if (numToBet > bet) bet = numToBet;
	return numToBet;
}

//Check for autoplayers whether each of them leaves the game.
int PokerGame::autoPlayerLeave() {
	vector<int> autoPlayers = findAuto();
	int numAuto = autoPlayers.size();
	int numPlayers = players.size();
	unsigned int leaveNum;
	string name;

	//immediate update associated files
	ofstream output;
	for (int i = 0; i < numPlayers; i++) {
		name = (*players[i]).name;
		if (find(autoPlayers.begin(), autoPlayers.end(), i) != autoPlayers.end()) {
			name = name.substr(0, name.size() - 1);
		}
		output.close();
		output.open(name + ".txt");
		output << *players[i];
	}

	//auto players leave with probablity
	for (int i = numAuto - 1; i >= 0; i--) {
		leaveNum = rand() % 100;
		if (autoPlayers[i] == 0) { //the last place
			if (leaveNum < 90) {
				players.erase(players.begin() + autoPlayers[i]);
			}
		}
		else if (autoPlayers[i] == numPlayers) { //the first place
			if (leaveNum < 10) {
				players.erase(players.begin() + autoPlayers[i]);
			}
		}
		else {
			if (leaveNum < 50) { //otherwise
				players.erase(players.begin() + autoPlayers[i]);
			}
		}

	}

	return 0;
}

//Return a list of all auto players.
vector<int> PokerGame::findAuto() {
	int currentPlayerNum = players.size();
	vector<int> autoPlayers;
	for (int i = 0; i < currentPlayerNum; i++) {
		char last = (*players[i]).name.back();
		if (last == '*') {
			autoPlayers.push_back(i);
		}
	}
	return autoPlayers;
}

//Count number of active players who have not folded.
int PokerGame::countActive() {
	int numPlayers = players.size();
	int active = 0;
	for (int i = 0; i < numPlayers; i++) {
		if (players[i]->isFold == false) active++;
	}
	return active;
}