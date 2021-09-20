### Foreword
The AI for the game of Hex employs a heuristic approach in contrast to standard brute-force algorithms for most turn based 
games (i.e.  Minimax algorithm with Alpha-beta pruning). 

To approximate the most optimal move for the AI, every available 
position in the  board is evaluated to check for the best next move. A win probability (in a set of >= 1000 random simulations) 
is calculated and the position with higher win probability is chosen. This allows the program to opt out of elaborate 
evaluation functions of game trees to decide on the best possible move. Implementation of MCS is computationally demanding, 
such to the extent that many optimizations were made, including changes to the internal graph structure representing the 
game board, along with other changes as noted later in this README. 
As a result, the AI is able to determine an optimal move on a 11 x 11 game board in under 30 seconds.


### Body
The board is represented by a graph.
Each board position corresponds to one node identified by a number and marked with a sign.
Signs: '.' = available, 'X' = player1, 'O' = player2.
There is an edge connecting two nodes if those nodes are neighbor positions in the board. 
There are 4 extra virtual nodes (WEST, EAST, NORTH and SOUTH) connected to nodes of the start and end positions in the board.
WEST and EAST start 'O'. NORTH and SOUTH start 'X'.
At each move, the selected position is marked with the player's sign and assigned edges that correspond to said position
If start virtual node and end virtual node are in the same path, then there is a winner.

AI plays using Monte Carlo simulations to make the best move at each round.
To implement a best move for AI, every available position in the board is evaluated to check what is the best next move. 
A win probability (probability of win in a set of >= 1000 random simulations) is calculated and the position with
higher probability is chosen.


### Optimizations
To optimize the probability calculation, a simulation is interrupted before its end if that position can't beat the best current
probability anymore. 
Many internal structures of the graph were modified to optimize Monte Carlo evaluation, resulting in significant
time reductions
For instance, the inclusion of virtual nodes and evaluating a win only after the MC sim game was over
allowed a reduction in processing time of ~ 50%
E.g., in a 3x3 game of hex:

```
	NORTH

	0 - 1 - 2
	 \ / \ / \
  ㅤWEST  3 - 4 - 5   EAST
	   \ / \ / \
	    6 - 7 - 8 

		SOUTH
		
```
		

Without virtual nodes, one would have to call a BFS function multiple times to determine if there exists a path
between WEST and EAST nodes. 

For instance, one would have to call the isConnected function 9 times to determine a winner from 
WEST node 0 to EAST node 2, then to EAST node 5, then to EAST node 8.
Then again from WEST node 3 to EAST node 2, then to EAST node 5, then to EAST node 8.
And finally, from WEST node 6 to EAST nodes 2, 5, and 8.
In other words, we would have n<sup>2</sup> operations for n nodes.

With the creation of the virtual nodes. We connect WEST nodes 0,3,8 to WEST node(or node 9, which isnt displayed, hence virtual)
and connected EAST nodes 2,5,8 to EAST node(or node 10, also not displayed on the board). 
Thus, the BFS isConnected function would only need to be called once to check for connectivity from WEST - EAST.
And then once more if we check for NORTH - SOUTH connectivity. The computation reduction is vast.
As a result, in a 11x11 board, the AI is able to choose the best move as best as ~30 seconds in some instances. 


### NOTES FOR POSSIBLE FURTHER IMPROVEMENT
The AI works fairly well provided the AI goes first(to overcome absence of a swap rule). But at times there are moves
the AI decides on that are not at its full advantage. This is expected as the Monte Carlo approach does not give
the most optimal move but an approximation to the most optimal move.
Perhaps a combination of a Minimax Algorithm(with Alpha-Beta pruning) and MCS would improve the AI to pick the best
move at a more optimal rate. Another possible improvement would be the implementation of concurrency to take advantage of the 
user's CPU's multi core, if available.
