// This file is part of the SGSolve library for stochastic games
// Copyright (C) 2019 Benjamin A. Brooks
// 
// SGSolve free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// SGSolve is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see
// <http://www.gnu.org/licenses/>.
// 
// Benjamin A. Brooks
// ben@benjaminbrooks.net
// Chicago, IL

#ifndef _SGSOLVER_MAXMINMAX_3PLAYER_HPP
#define _SGSOLVER_MAXMINMAX_3PLAYER_HPP

#include "sgcommon.hpp"
#include "sgutilities.hpp"
#include "sgenv.hpp"
#include "sggame.hpp"
#include "sgaction_maxminmax.hpp"
#include "sgexception.hpp"
#include "sgsolution_maxminmax.hpp"
#include "sgedgepolicy.hpp"

//! Class for solving stochastic games
/*! This class implements the max-min-max algorithm of Abreu, Brooks,
  and Sannikov (2019) for three players. It contains the parameters
  for the algorithm, the solve method, as well as the data structure
  produced by solve. It calculates the equilibrium payoff
  correspondence corresponding to an SGGame object.

  \ingroup src
 */
class SGSolver_MaxMinMax_3Player
{
private:
  // Data

  //! SGEnv object to hold parameters
  const SGEnv & env;
  //! Constant reference to the game to be solved.
  const SGGame & game; 
  //! SGSolution object used by SGApprox to store data.
  SGSolution_MaxMinMax soln;

  // References to objects in the game
  const double delta; /*!< The discount factor, copied from
                         SGApprox_V2::game. */
  const int numPlayers; /*!< The number of players, always 2. */
  const int numStates; /*!< The number of states, copied from
                          SGApprox_V2::game. */

  const vector< vector<bool> > & eqActions; /*!< Constant refernece to
                                            actions that are allowed
                                            to be played in
                                            equilibrium in the game. */
  const vector< vector<SGPoint> > & payoffs; /*!< Constant reference to payoffs in the game. */
  const vector< vector< vector<double> > > & probabilities; /*!< Constant reference to transition probabilities in the game. */
  const vector< vector<int> > numActions; /*!< Number of actions in the game. */
  const vector< int > numActions_totalByState; /*!< Total number of actions in each state. */

  list<SGPoint> directions; /*!< List of directions in which the algorithm bounds payoffs. */
  list< vector<double> > levels; /*!< List of optimal levels attained in the corresponding directions. */

  list<SGPoint> threatDirections;
  SGTuple threatTuple; /*!< The current threat payoffs. */
  vector< list<SGAction_MaxMinMax> > actions; /*!< Actions that can still be played. */
  
  const SGPoint dueEast = SGPoint(1.0,0.0); /*!< The direction due east. */
  const SGPoint dueNorth = SGPoint(0.0,1.0); /*!< The direction due north. */

  int numIter; /*!< The number of iterations computed thus far. */
  double errorLevel; /*!< The current error level. */

  SGPoint payoffLB; /*!< Lower bound on payoffs across all states and actions. */
  SGPoint payoffUB; /*!< Upper bound on payoffs across all states and actions. */

  int numRedundDirs; /*!< Number of redundant directions dropped so far. */
  int numEndogDirs; /*!< Number of endogenous directions added so far. */

  bool debugMode; /*!< Indicator for whether the program is being debugged. */
  
public:
  //! Default constructor
  SGSolver_MaxMinMax_3Player(); 

  //! Constructor
  /*! Creates a new SGSolver_MaxMinMax_3Player object and initializes
      it with the given game. */
  SGSolver_MaxMinMax_3Player(const SGEnv & _env, 
	   const SGGame & _game);

  //! Destructor
  ~SGSolver_MaxMinMax_3Player() {}

  //! Fixed direction solve routine
  /*! Initializes a new SGApproximation object and iteratively
      generates it until one of the stopping criteria have been
      met. Stores progress in the data member. 

      Drops redundant directions and adds new face directions. */
  void solve(const int numDirsApprox = 200,
	     const bool dropRedundant = true,
	     const bool addEndogenous = true);

  //! Endogenous direction solve routine
  /*! Initializes a new SGApproximation object and iteratively
      generates it until one of the stopping criteria have been
      met. Stores progress in the data member. 

      Generates directions endogenously. */
  void solve_endogenous();

  //! One iteration of the endogenous algorithm.
  /*! Return the new error level. */
  double iterate_endogenous();

  //! One iteration of the algorithm.
  /*! Return the new error level. */
  double iterate(const int maxDirections,
		       const bool dropRedundant,
		       const bool addEndogenous);
  
  //! Initializes the solve routines
  void initialize();

  std::string progressString() const;
  
  //! Optimizes the policy for the given direction
  void optimizePolicy(SGTuple & pivot,
		      vector<SGActionIter> & actionTuple,
		      vector<SG::Regime> & regimeTuple,
		      const SGPoint & currDir,
		      const vector<list<SGAction_MaxMinMax> > & actions) const;

  //! Compute all policies that are at least as high as the given
  //! pivot. Returns false if there is a strict improvement, and true
  //! otherwise. Will short circuit if it finds a strict improvement.
  bool computeOptimalPolicies(SGProductPolicy & optPolicies,
			      const SGTuple & pivot,
			      const SGPoint & currDir,
			      const vector<list<SGAction_MaxMinMax> > & actions) const;
  
  //! Find the next clockwise direction at which the optimal tuple
  //! changes
  double sensitivity(SGPoint & optSubDir,
		     const SGTuple & pivot,
		     const SGPoint & currDir,
		     const SGPoint & newDir,
		     const vector<list<SGAction_MaxMinMax> > & actions) const;

  //! Converts a policy function to a payoff function using bellman iteration
  void policyToPayoffs(SGTuple & pivot,
		       const vector<SGActionIter>  & actionTuple,
		       const vector<SG::Regime> & regimeTuple) const;

  //! Returns a constant reference to the SGSolution_MaxMinMax object storing the
  //! output of the computation.
  const SGSolution_MaxMinMax& getSolution() const {return soln;}
};


#endif
