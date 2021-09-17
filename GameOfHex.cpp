// Game of Hex
// Author: Marcos Padilla
// Date: 05/11/2021
// 
// The board is represented by a graph.
// Each board position corresponds to one node identified by a number and marked with a sign
// Signs: '.' = available, 'X' = player1, 'O' = player2.
// There is an edge connecting two nodes if those nodes are neighbor positions in the board. 
// There are 4 extra virtual nodes (WEST, EAST, NORTH and SOUTH) connected to nodes of the start and end positions in the board.
// WEST and EAST start 'O'. NORTH and SOUTH start 'X'.
// At each move, the selected position is marked with the player's sign and assigned edges that correspond to said position
// If start virtual node and end virtual node are in the same path, then there is a winner.

// AI plays using Monte Carlo simulations to make the best move at each round.
// To implement a best move for AI, every available position in the board is evaluated to check what is the best next move. 
// A win probability (probability of win in a set of >= 1000 random simulations) is calculated and the position with
// higher probability is chosen.
// To optimize the probability calculation, a simulation is interrupted before its end if that position can't beat the best current
// probability anymore. 
// Many internal structures of the graph were modified to optimize Monte Carlo evaluation, resulting in significant
// time reductions
// For instance, the inclusion of virtual nodes and evaluating a win only after the MC sim game was over
// allowed a reduction in processing time of ~ 50%
// As a result, in a 11x11 board, the AI is able to choose the best move as best as ~30 seconds in some instances. 
//

#include <iostream>
#include <iomanip> 
#include <climits>
#include <vector>
#include <list>
#include <random>
#include <algorithm>
#include <ctime>
#include <string>
#include <chrono>
#include <tuple>
#include <thread>
using namespace std;

const int INFINIT = INT_MAX;
static int sizeofBoard = 0;
// SIMUL set the default number of simulations for each Monte Carlo move evaluation  
const int SIMUL = 1000;

inline pair<int, int> coordinates(string command){
	string substring = command.substr(1,2);
	int x = 0;
	int y = 0;

	y = command[0] - 65; 
	x = stoi(substring) - 1;

	return {x, y};
}

// Class that represents the game board as a graph
class Graph {
	private:
	vector<int> adjMatrix;	// Adjacency matrix of the graph
	vector<char> sign;		// Keeps record of sign (X, O, or . , none)
	int numVertices;		// Number of vertices in the graph
	int numEdges;			// Number of edges in the graph
	int startNode[3];		// Stores start node for Player 1(1) and Player 2(2)
	int endNode[3];			// Stores end node for Player 1(1) and Player 2(2)

	public:
	Graph() {};
	Graph(int numVertices);		// Parameterized Graph constructor
	int V() const;						// Returns the number of vertices in the graph
	int E() const;						// Returns the number of edges in the graph
	bool adjacent (const int& x, const int& y) const;	// Tests whether there is an edge from node x to node y
	int get_edge_value(const int& x, const int& y) const;	// Returns the value associated to the edge (x,y) from Node x to Node y
	void set_edge_value(const int& x, const int& y, const int& value); // Sets the value associated to the edge (x,y) to value
	char get_sign(const int& x, const int& y) const;			// Returns sign for node(x,y)
	void set_sign(const int& x, const int& y, const char& s);	// Sets sign for node(x,y)
	int get_startNode(const int& playerNum) const;				// Returns start node based on player
	int get_endNode(const int& playerNum) const;				// Returns end node based on player
};

  // Initialize the matrix to zero
Graph::Graph(int numVertices) {
	this->numVertices = numVertices;
	this->numEdges = 0;

	// Creation of n x n adjacency matrix, initialized edges to INFINIT
	adjMatrix.resize(numVertices * numVertices, INFINIT);
	sign.resize(numVertices * numVertices, '.');
	for (int i = 0; i < numVertices; ++i){
		adjMatrix[i * numVertices + i] = 0; // For edge(i,j) where i = j, set edge value as 0
	}

	int vnodeWest, vnodeEast, vnodeNorth, vnodeSouth;
  	vnodeWest = sizeofBoard*sizeofBoard;	// Node (d^2) represents WEST virtual node
	vnodeEast = sizeofBoard*sizeofBoard + 1;	// Node (d^2 + 1) represents EAST virtual node
	vnodeNorth = sizeofBoard*sizeofBoard + 2; 	// Node (d^2 + 2) represents SOUTH virtual node
	vnodeSouth = sizeofBoard*sizeofBoard + 3;	// Node (d^2 + 3) represents NORTH virtual node

	int x, y;	// x,y coordinates for setting node sign

	// Connect all north nodes(row 0) to virtual north node
	x = vnodeNorth / sizeofBoard;
	y = vnodeNorth % sizeofBoard;
	set_sign(x, y, 'X');
	for (int i = 0; i < sizeofBoard; i++){
		set_edge_value(vnodeNorth, i, 1);
	}
	// Connect all south nodes(row n - 1) to virtual south node
	x = vnodeSouth / sizeofBoard;
	y = vnodeSouth % sizeofBoard;
	set_sign(x, y, 'X');
	for (int j = V() - 1; j >= V() - sizeofBoard; j--){
		set_edge_value(vnodeSouth, j, 1);
	}
	// Connect all west nodes(column 0) to virtual west node
	x = vnodeWest / sizeofBoard;
	y = vnodeWest % sizeofBoard;
	set_sign(x, y, 'O');
	for (int i = 0; i < V(); i = i + sizeofBoard){
		set_edge_value(vnodeWest, i, 1);
	}
	// Connect all east nodes(column n - 1) to virtual east node
	x = vnodeEast / sizeofBoard;
	y = vnodeEast % sizeofBoard;
	set_sign(x, y, 'O');
	for (int j = sizeofBoard - 1; j < V(); j = j + sizeofBoard){
		set_edge_value(vnodeEast, j, 1);
	}

	// Player 1 connects from North - South
	startNode[1] = vnodeNorth;
	endNode[1] = vnodeSouth;

	// Player 2 connects from West - East
	startNode[2] = vnodeWest;
	endNode[2] = vnodeEast;
	
}

int Graph::V() const{
	return numVertices - 4; // Returns the number of vertices in the graph
}

int Graph::E() const{
	return numEdges; // Returns the number of edges in the graph
}

  // Return true if x and y are neighbors, false if not
bool Graph::adjacent (const int& x, const int& y) const{
  	return (get_edge_value(x, y) != INFINIT && get_edge_value(x, y) != 0);
}

int Graph::get_edge_value(const int& x, const int& y) const{
	return (adjMatrix[x * numVertices + y]); // Return edge value
}

void Graph::set_edge_value(const int& x, const int& y, const int& value){
	if (adjacent(x,y) == false){ // If there is not an existing edge, increment number of edges
  		++numEdges;
	}
	adjMatrix[x * numVertices + y] = adjMatrix[y * numVertices + x] = value; // add new edge value
}

char Graph::get_sign(const int& x, const int& y) const{
	return (sign[x * numVertices + y]); // Return sign value
}

void Graph::set_sign(const int& x, const int& y, const char& s){
	sign[x * numVertices + y] = s; // add new sign value	
}

int Graph::get_startNode(const int& playerNum) const{
	return (startNode[playerNum]);	// Return start node
}

int Graph::get_endNode(const int& playerNum) const{
	return (endNode[playerNum]);	// Return end node
}

// Evaluates game winner
class Evaluate{
	public:
	bool isReachable(const Graph& g, int s, const int& d, const char& sign);
	int winnerAI(const Graph& g, const int& playerNum);
};


// A BFS based function to check whether d is reachable from s.
bool Evaluate::isReachable(const Graph& g, int s, const int& d, const char& sign)
{   
    list<int> adj; // list of neighbors y of x, if any

    // Base case
    if (s == d)
        return true;
 
    // Mark all the vertices as not visited
    bool* visited = new bool[g.V()];
    for (int i = 0; i < g.V() + 4; i++)
        visited[i] = false;
 
    // Create a queue for BFS
    list<int> queue;
 
    // Mark the current node as visited and enqueue it
    visited[s] = true;
    queue.push_back(s);
 
    // it will be used to get all adjacent vertices of a vertex
    // list<int>::iterator i;
 
    while (!queue.empty()) {
        
        // Dequeue a vertex from queue and print it
        s = queue.front();
        queue.pop_front();

        // cout << "Neighbors for node " << s << " are: " << endl;
        for (int y = 0; y < g.V() + 4; y++){
			// Convert u node into i,j positions
			int i = y / sizeofBoard;
			int j = y % sizeofBoard;
			int m = s / sizeofBoard;
			int n = s % sizeofBoard;
            if (g.adjacent(s,y) == true && g.get_sign(i,j) == sign && g.get_sign(m,n) == sign){
                adj.push_back(y);
                // cout << "node " << y << endl;
            }
	    }

        // Get all adjacent vertices of the dequeued vertex s
        // If a adjacent has not been visited, then mark it
        // visited  and enqueue it
        for (auto i: adj) {
 
            // If this adjacent node is the destination node,
            // then return true
            if (i == d)
                return true;
 
            // Else, continue to do BFS
            if (!visited[i]) {
                visited[i] = true;
                queue.push_back(i);
            }
        }
    }
 
    // If BFS is complete without visiting d
	// cout << " returned false" << endl;
    return false;
}

// 1st and main method for determining winner, using a BFS approach
int Evaluate::winnerAI(const Graph& g, const int& playerNum){
	int winner = 0; // 0: No winner, 1: Player 1 winner, 2: Player 2 winner
	bool won;
	char sign;
	int i,j;

	if (playerNum == 1){
		sign = 'X';

		// Check if player 1 has won
		i = g.get_startNode(playerNum);
		j = g.get_endNode(playerNum);
		won = isReachable(g, i, j, sign);
		if (won == true){
			return playerNum;
		}
		// Else check if player 2 has won
		else{
			i = g.get_startNode(2);
			j = g.get_endNode(2);
			won = isReachable(g, i, j, 'O');
			if (won == true){
				return 2;	
			}
		}
	}
	else{
		sign = 'O';

		// Check if player 2 has won
		i = g.get_startNode(playerNum);
		j = g.get_endNode(playerNum);
		won = isReachable(g, i, j, sign);
		if (won == true){
			return playerNum;
		}
		// Else check if player 1 has won
		else{
			i = g.get_startNode(1);
			j = g.get_endNode(1);
			won = isReachable(g, i, j, 'X');
			if (won == true)
				return 1;	
		}
	}
	
	return winner;

}

// Class in charge of displaying board, determining AI's move, etc.
class hexGame{
	public:
	Evaluate game;	// Class object init, to evaluate game winner
	void setEdges(const int& x, const int& y, Graph* g);	
	void drawBoard(const Graph& g);
	bool validMove(const Graph& g, const string& command);
	vector<pair <int, int> > availablePositions(const Graph& g) const;
	void aiMove(Graph* g, const int& playerNum);	// Sets the best possible move returned from the mcs function
	pair<int, int> monteCarloSims(const Graph& g, const int& playerNum);	// mcs function responsible for determining AI's best move
	double probMonteCarlo(Graph g, const pair<int,int>& i, const double& bestProb, const int& playerNum, const int& numsim=SIMUL);
	bool playerMove(Graph* g, string command, const int& playerNum);

};

// Sets corresponding edges to node (x,y)
void hexGame::setEdges(const int& x, const int& y, Graph* g){

	// Maps (x,y) coordinate to corresponding node number on N sized board, from Node 0 to Node N - 1.
	// e.g. for a board of size 7: Node coord (0, 6) = ( (0 * 7) + 6 ) = Node 6 
	int iNode = (x * sizeofBoard + y);	// Node i
	int jNode;	// Node j from which Node i shares an edge with

	
	// Set the number of edges based on node (x,y)'s position
	// Corner pieces : Have either 2 or 3 edges depending on corner
	// Edge pieces : Have 4 edges
	// Internal pieces : Have 6 edges

	if (x > 0){
		jNode = ( (x - 1) * sizeofBoard + y ); // Node j is to the upper left of Node i
		(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
		if (y < sizeofBoard - 1){
			jNode = ( (x - 1) * sizeofBoard + y + 1 ); // Node j is to the upper right of Node i
			(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
		}
	}
	if (y > 0){
		jNode = (x * sizeofBoard + y - 1); // Node j is to the left of Node i
		(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
		if (x < sizeofBoard - 1){
			jNode = ( (x + 1) * sizeofBoard + y - 1 ); // Node j is to the lower left of Node i
			(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
		}
	}
	if (y < sizeofBoard - 1){
		jNode = (x * sizeofBoard + y + 1); // Node j is to the right of Node i
		(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
	}
	if (x < sizeofBoard - 1){
		jNode = ( (x + 1) * sizeofBoard + y ); // Node j is to the lower right of Node i
		(*g).set_edge_value(iNode, jNode, 1); // Set the edge from Node i to j
	}
}

// Draws game board
void hexGame::drawBoard(const Graph& g){

	// Prints NORTH label

	cout << endl << setw(2 * sizeofBoard + 4) << "NORTH" << endl;

	// Prints Top Column Letters of Board

	cout << endl << "  ";
	for (int i = 0; i < sizeofBoard; i++)
		cout << static_cast<char>(i + 'A') << "   ";
	cout << endl << endl;

	// Prints Left and Right Row Numbers of Board

	for (int row = 0; row < sizeofBoard; row++){
		if (row < 9){
			for (int j = 0; j < (row * 2); j++)
				cout << " " ;
		}
		else{
			for (int j = 0; j < (row * 2) - 1; j++)
				cout << " " ;
		}
		cout << row + 1 << "  "; // Print left row number
		for (int col = 0; col < sizeofBoard; col++){
			cout << g.get_sign(row, col);
			if (col < sizeofBoard - 1)
				// cout << "   ";
				cout << " - ";
		}
		cout << "   " << row + 1; // Print right row number
		if (row < sizeofBoard - 1){
			cout << endl << "  ";
			for (int j = 0; j < (row * 2) + 1; j++)
				cout << " " ;
			// cout << "  ";
			cout << " \\";
			for (int col = 0; col < sizeofBoard - 1; col++){
				// cout << "    ";
				cout << " / \\";
			}
			cout << endl;
		}
	}
	cout << endl << endl;

	// Print Bottom Column Letters of Board

	for (int row = 0; row < sizeofBoard * 2; row++)
		cout << " " ;
	cout << "  ";
	for (int i = 0; i < sizeofBoard; i++)
		cout << static_cast<char>(i + 'A') << "   ";
	cout << endl << endl;

	// Prints SOUTH label

	cout << setw(4 * (sizeofBoard) + 3) << "SOUTH" << endl << endl;

}

// Determines validity of human's move
bool hexGame::validMove(const Graph& g, const string& command){
	char maxNum = '0' + sizeofBoard;	// The max row number based on the size of the board chosen
	char maxLetter = 'A' + sizeofBoard - 1;	// The max column letter based on the size of the board chosen

	// If user inputs -1, they have quit, exit program

    if (command == "-1"){
        cout << "You have quit the match." << endl;
        exit(EXIT_SUCCESS);
    }

	// If input length is more than 2 AND the board size < 10, invalid. Ex: board size 9, input 'A10' = invalid
	// Or if the input row number surpasses the board size, invalid. Ex: board size 7, input 'A8' = invalid
	// Or if the input col letter surpasses the board size, invalid. Ex: board size 3, input 'D2' = invalid

	if (command.length() > 2 && sizeofBoard < 10 || (command[1] > maxNum) 
	|| (command[0] > maxLetter) ){
		cout << command << " is not a valid entry! Entry must be within a size of " << sizeofBoard << endl;
		return false;
	}

	// If input length is less than 2 or greater than 3, invalid. Ex: input 'A' = invalid, input 'A100' = invalid
	// Or if input col letter is not anything between A - K, invalid. Ex: board size 11, input 'L11' = invalid

    if (command.length() < 2 || command.length() > 3 || command[0] < 'A' || command[0] > 'K' ){
        cout << command << " is not a valid entry!" << endl;
        return false;
    }

	// If the input row number is not a number, invalid. Ex: input 'AK' = invalid
	// Or if the board size >= 10 AND the input row number of double digits is not a number, invalid. Ex: input 'A1K' = invalid
	// Or if the board size >= 10 AND the input row number of double digits is > 11, invalid. Ex: board size 11, input 'A21' = invalid
	// Or if the board size == 10 AND the input row number of double digits is > 10, invalid. Ex: board size 10, input 'A11' = invalid

    if ( (isdigit(command[1]) == 0) || (command.length() == 3 && isdigit(command[2]) == 0) 
    || (command.length() == 3 && (command[1] > '1' || command[2] > '1') ) 
	|| (sizeofBoard == 10 && command.length() == 3 && (command[1] > '1' || command[2] > '0')) ) {
        cout << command << " is not a valid entry!" << endl;
        return false;
    }
    
	auto [x, y] = coordinates(command);	// Convert string to x,y coordinate to check if the place is already occupied
	
	// If the move is valid, do one final check to see if the position is already occupied

    if (g.get_sign(x,y) != '.'){
    	cout << command << " is already occupied. Choose another entry." << endl;
    	return false;
	}

	// Else return true
	
    return true;
}

// Returns vector of pair of coord for available positions on the board
vector<pair <int, int> > hexGame::availablePositions(const Graph& g) const{
	vector<pair <int,int> > availVect;	// vector holding positions that are not occupied by 'X' or 'O')
	for (int i = 0; i < sizeofBoard; ++i){
		for (int j = 0; j < sizeofBoard; ++j){
			if (g.get_sign(i,j) == '.'){
				availVect.push_back(make_pair(i,j));
			}
		}
	}

	unsigned int seed = chrono::steady_clock::now().time_since_epoch().count();
	auto rng = default_random_engine{seed};
	shuffle(availVect.begin(), availVect.end(), rng);


	return availVect;
}

// Function responsible for making a move decision for the AI
void hexGame::aiMove(Graph* g, const int& playerNum){
	char sign = 'X';

	if ( playerNum == 1)
		sign = 'X';
	else
		sign = 'O';

	auto [x,y] = monteCarloSims(*g, playerNum);
	cout << "AI, where would you like to place your move?: ";
	cout << static_cast<char>(y + 'A');
	cout << x + 1 << endl;

	(*g).set_sign(x, y, sign);	// Set player's valid position as X/O on the board coord (x,y)
	setEdges(x, y, g);	// Set edges for valid position	
}

// Function responsible for returning the best possible move for the AI based on the win prob for each possible move
pair<int, int> hexGame::monteCarloSims(const Graph& g, const int& playerNum) {
	pair<int,int> bestMove;
	double bestprob = -1.0;
	double probMC = 0.0;
	double visited = 0.0;
	double total = 0.0;
	vector<thread> threads;
	

	// cout << "Listing available positions" << endl;
	cout << "Thinking..." << endl;
	vector< pair <int,int> > candidates = availablePositions(g);
	total = static_cast<double>(candidates.size());	

	for (auto i:candidates){
		probMC = probMonteCarlo(g, i, bestprob, playerNum, SIMUL);	// For each candidate position, evaluate its Monte Carlo probability
		// cout << "probMC = " << probMC << " for candidate " << i.first << ", " << i.second << endl;
		if (bestprob < probMC){	// If its Monte Carlo probability is the best known, set this candidate as the best move
			bestprob = probMC;
			bestMove = i;
		}
		visited++;	// Update the visited moves counter
	}
	// cout << "returning bestMove " << bestMove.first << ", " << bestMove.second << endl;
	return bestMove;
}

// Function that executes the monte carlo simulations and evaluates the win prob for each move
double hexGame::probMonteCarlo(Graph g, const pair<int,int>& i, const double& bestProb, const int& playerNum, const int &numsim) {
	char sign = 'X';
	char signH = 'O';
	int winner = 0;	// To determine winner of round
	int numwins = 0; // Holds number of wins for position
	int total;	// Get the total number of available positions in the board
	int goesNext;	// To determine which player goes next

	Graph currentG; // Create a graph class object
	vector< pair <int,int> > available, availablecopy;	// To determine available positions
	vector< pair <int,int> >::iterator posptr;	// Position pointer for the avaialble positions
	

	if (playerNum == 1){
		sign = 'X';
		signH = 'O';
	}
	else{
		sign = 'O';
		signH = 'X';
	}

	auto[x,y] = i;	// Convert node i into (x,y) position on board
	g.set_sign(x, y, sign);	// Set sign for valid position
	setEdges(x, y, &g);	// Set edges for valid position
	
	int winnerG = game.winnerAI(g, playerNum);	// Determine the winning state of the current position
	winner = winnerG;	// Copy winner state

	int it = 0;	// Number of iterations of simulation is initially set to 0
	while ( (it < numsim) && (( (numsim-it) + numwins) > (bestProb*numsim) )) {	// If this position can't beat bestprob, interrupt simulation
		goesNext = playerNum; // AI goes first
		goesNext = (goesNext * 2) % 3; // Alternates between 1 and 2, signifying each player respectively
		currentG = g;	// Make a copy of graph g, to safely manipulate in the simulations to follow
		available = availablePositions(g);	// Get a copy of all avalable positions in the board
		total = available.size();			// Get the total number of available positions in the board
		posptr = available.begin();	// Point to the first random move

		while (winner == 0) {	// Play the game until there is a winner
			auto[i,j] = *posptr;	// Set random moves coord to i,j
			if (goesNext == playerNum){ // If AI's turn, set random coord i,j
				currentG.set_sign(i, j, sign);
				setEdges(i, j, &currentG);
			}
			else{	// Else if human's turn, set random coord i,j
				currentG.set_sign(i, j, signH);
				setEdges(i, j, &currentG);
			}
			total--;	// Decrement total available positions
			goesNext = (goesNext * 2) % 3;	// After a move has been made go on to next player move
			if (total == 0){	// Once total available positions reaches 0, run algo to check for win
				winner = game.winnerAI(currentG, playerNum);	// Check for winner
			}
			posptr++;	// Go to the next random position
		}	// End of while

		if (winner == playerNum){	// If the winner is the function caller, increments the number of wins 
			numwins++;
		}

		winner = winnerG;	// Reset winner value eval to original initial move
		++it;	// nth iteration is complete, increment it to continue next iterations
  	}
	return (static_cast<double>(numwins)/static_cast<double>(numsim));	// Return win probability
}

// Function handles player move, checks for validity, places move, etc.
bool hexGame::playerMove(Graph* g, string command, const int& playerNum){
	command[0] = toupper(command[0]); // Make any lowercase character uppercase (e.g. a1 -> A1)
	char sign = 'X';
	if ( playerNum == 1){
		sign = 'X';
	}
	else{
		sign = 'O';
	}
	bool valid = false;

	valid = validMove(*g, command); // Check validity of move

        if (valid){	// If move is valid, store the coordinates, and continue
			auto [x, y] = coordinates(command); // e.g. converts string input A1 to ints (0,0) as x,y respectively

			(*g).set_sign(x, y, sign);	// Set player's valid position as X/O on the board coord (x,y)
			setEdges(x, y, g);	// Set edges for valid position
			valid = true;
			
        }
	return valid;
}

// Class responsible for handling game flow
class Game {
  public:
    void start();

  private:
    int moveCount = 0;
	int movesAI = 0;
	bool valid = -1;
	char swap = 'y';			// User input for whether or not player wants to swap 'X' for 'O'
	string command;				// User input for position of 'X' placed on board
	Evaluate evaluate;			// Determine winner
	hexGame hex;				// Constructure for hexGame class
	int winner = 0;				// 0 : Nobody won, 2 : Player 2 won, 1: Player 1 won 
	int player1 = 1;			// Value of 1: X, Value of -1: O
	int player2 = 2;			// Value of 1: X, Value of -1: O
	int user = 0;				// Lets the program know which player is the user
	int computer = 0;			// Lets the program know which player is the AI
	int goesNext = 1;			// Tells the program who goes first/next, AI or Human
};

// Starts the game session
void Game::start(){

	// Print intro, ask user for board size, take input

	cout << "-----------------------------------------------------------------------" << endl;
	cout << "Welcome to the game of Hex. Enter -1 to quit game anytime." << endl;
	cout << "What size board would you like to play with? Enter size (2 - 11): ";
	cin >> sizeofBoard;
	cout << "-----------------------------------------------------------------------" << endl;

	// Validate proper board size (between 2 - 11)

	while (sizeofBoard < 2 || sizeofBoard > 11){
		cout << endl << "Please enter a valid size of 7 or 11: ";
		cin >> sizeofBoard;
	}

	// Initialize Graph g, representing the game board
	Graph g(sizeofBoard * sizeofBoard + 4);	// n x n total nodes + 4 virtual nodes

	if (moveCount == 0 && movesAI == 0){ // Draw game board + instructions if beginning of game
			if (user == player2){
				cout << "Human, you have agreed to go second, you are now Player 2, sign O" << endl;
				cout << "AI, is now Player 1, sign X" << endl;
			}
			cout << "*****************************************" << endl;
			cout << "Player 1, connects X from North to South" << endl;
			cout << "Player 2, connects O from East to West" << endl;
			cout << "*****************************************" << endl;

	    	hex.drawBoard(g);	// Draw the initial game start hex board

			cout << "Ready to play?" << endl;
			cin >> swap;
		}

	// Ask user if they want to go second instead, swap signs and place with AI if so

	cout << "You, Player 1, are assigned X, while the AI, Player 2, is assigned O" << endl;
	cout << "You will go first. Would you like to go second instead? (Y/N) ";
	cin >> swap;
	cout << "-----------------------------------------------------------------------" << endl;
	swap = toupper(swap);

	// Validate swap response

	if (swap == 'N'){
		user = player1;
		computer = player2;
	}
	else{
		user = player2;
		computer = player1;
	}

	// This loop controls the game flow
	// The game will display a blank game board at the beginning of each game
	// X will go first, which may be the user or AI, depending on what the user chose

	while (command != "-1" && winner == 0){	// While user does not quit or there is no winner,

		if (goesNext != user){	// If AI turn, AI makes a move
			cout << "AI is deciding for the best move..." << endl;
			
			hex.aiMove(&g, computer); // Make a move decision for AI
			++movesAI;	// Increment AI move count

			if (movesAI >= sizeofBoard){ // If the move count = size of board or more, check for winner
				winner = evaluate.winnerAI(g, computer);
			}

		}

		else{	// Else if Human turn, perform the following
			cout << "Human, where would you like to place your move? (i.e. A1, B2, etc.): ";	// Ask user for input
			cin >> command;	// Store user input as a string

			valid = hex.playerMove(&g, command, user); // Returns true or false valid move
			if (valid){	// If valid, increment player move count
				moveCount++;
				if (moveCount >= sizeofBoard){ // If valid moves reaches size of board, check for winner
					winner = evaluate.winnerAI(g, user);
				}
			}
		}

		hex.drawBoard(g);	// Draw the current hex board

		if (winner != 0)	// If there is a winner, break loop, end game
			break;	

		if (valid)			// If the previous move was valid, goesNext alternates, next player makes move
			goesNext = (goesNext * 2) % 3; // Alternates between 1 and 2, signifying each player respectively

	}	// End of while/Game Flow
	
	if (winner == user){ // If the winner was the user, print congrats message
		cout << "\nYOU HAVE WON THE GAME." << endl;
		cout << "Total move count for player " << user << ": " << moveCount << endl;
	}
	else{
		cout << "AI has won" << endl;
		cout << "Total move count for player " << computer << ": " << moveCount << endl;

	}

}

// Main function

int main(){

	Game game;
	game.start();

	return 0; // End of program.
}
