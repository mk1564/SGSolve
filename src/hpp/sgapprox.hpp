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

#ifndef _SGAPPROX_HPP
#define _SGAPPROX_HPP

#include "sgcommon.hpp"
#include "sgutilities.hpp"
#include "sgenv.hpp"
#include "sggame.hpp"
#include "sgexception.hpp"
#include "sgsolution_pencilsharpening.hpp"
#include "sgnamespace.hpp"

//! Approximation of the equilibrium payoff correspondence.
/*! This class contains an approximation of the equilibrium payoff
  correspondence. At its core, it contains a sequence of extreme
  tuples that have been generated thus far, a pivot, and a
  direction. The main method, SGApprox::generate(), finds a
  new direction that will not intersect the equilibrium payoff
  correspondence and updates the pivot in that direction. By
  successively calling SGApprox::generate(), the
  approximation will be refined and asymptotically it will converge
  to the equilibrium payoff correspondence. 

  Part of the pencil sharpening algorithm. 
  
  \ingroup src
*/
class SGApprox
{
private:
  const SGEnv & env; /*!< Constant reference to the parent
                        environment. */
  const SGGame & game; /*!< Constant reference to the game being
                          solved. */
  SGSolution_PencilSharpening & soln; /*!< Reference to the SGSolution object in which output
                    is being stored. */

  const double delta; /*!< The discount factor, copied from
                         SGApprox::game. */
  const int numPlayers; /*!< The number of players, always 2. */
  const int numStates; /*!< The number of states, copied from
                          SGApprox::game. */

  std::ofstream logfs; /*!< File stream for log file. */
  
  int numIterations; /*!< Elapsed number of iterations. */
  int numRevolutions; /*!< Elapsed number of revolutions. */
  double errorLevel; /*!< Current error level */
  
  vector<bool> facingEastNorth; /*!< Indicates whether the current
                                   direction points east (if
                                   facingEastNorth[0]=true) and north
                                   (if facingEastNorth[1]=true). */
  bool passNorth; /*!< Flag that is true if the algorithm switched
                     from pointing south to pointing north on the
                     current iteration. */
  bool sufficiencyFlag; /*!< Flag that indicates if sufficient
                           conditions have been met while searching
                           for the best direction. */
  vector<bool> updatedThreatTuple; /*!< updatedThreatTuple[i] = true
                                      if player i's threat tuple was
                                      updated on the current
                                      iteration. */

  vector< list<SGAction_PencilSharpening> > actions; /*!< actions[state] is a list of
                                       actions that can still be
                                       supported according to the
                                       current approximation. */
  vector<SGTuple> extremeTuples; /*!< Past trajectory of the pivot. */

  SGTuple threatTuple; /*!< Current threat tuple. */

  SGTuple pivot; /*!< Current pivot. */
  SGPoint currentDirection; /*!< The current direction. */
  vector< const SGAction_PencilSharpening* > actionTuple; /*!< actionTuple[state] is a
                                            pointer to the SGAction
                                            object that generates
                                            pivot[state]. */
  vector<SG::Regime> regimeTuple; /*!< regimeTuple[state] gives the
				    manner in which pivot[state] was
				    generated. */
  
  list<SGAction_PencilSharpening>::const_iterator bestAction; /*!< Pointer to the
					       action profile that
					       generates the
					       shallowest
					       direction. */
  SGPoint bestDirection; /*!< The shallowest direction at the current
                            iteration. */
  SG::Regime bestRegime; /*!< Indicates which incentive
                                      constraints were binding for the
                                      best direction. */

  SGAction_PencilSharpening nullAction; /*!< Used to initialize the action tuple. */

  int westPoint; /*!< Index within SGApprox::extremeTuples of
                    the westernmost tuple on the previous
                    revolution. */
  int newWest; /*!< Index within SGApprox::extremeTuples of the
		 westernmost tuple on the current revolution. */
  int oldWest; /*!< Previous value of westPoint. */

  //! Calculates the minimum IC continuation values
  /*! This method calculates for each SGAction object in
      SGApprox::actions the minimum incentive compatible
      continuation value, relative to the current threat tuple. */
  void updateMinPayoffs();

  //! Calculates binding continuation values
  /*! For each SGAction objection in SGApprox::actions, this
      method computes the extreme binding continuation values relative
      to the current threat tuple and the trajectory of the pivot on
      the previous revolution. */
  void calculateBindingContinuations();

  //! Trims binding continuation values
  /*! For each SGAction in SGApprox::actions, this method
      "trims" the binding payoff segents by intersecting them with the
      half-space correspondence that is below the pivot in the
      direction normal to the current direction. Not currently being
      used. */
  void trimBindingContinuations() ;

  //! Calculates the best direction
  /*! Iteraties over the SGAction objects in SGApprox::actions
      to find the shallowest admissible direction, and stores it in
      bestDirection. */
  void findBestDirection();

  //! Calculates the new pivot
  /*! After the best direction has been found, this method updates the
      pivot in the new current direction. First, it calculates the
      maximum movements in the current direction that would not
      violate incentive compatibility, and then iterates
      SGApprox::updatePivot until the pivot converges. */
  void calculateNewPivot();

  //! Updates the pivot
  /*! Bellman operator that advances the pivots corresponding to
      non-binding states in the current direction. If an IC constraint
      would be violated in state s, the movement in state s is set to
      maxMovement[s] and the state is put into the binding
      regime. Returns the distance the pivot moves. */
  double updatePivot(vector<double> & movements, 
		     vector<double> & changes,
		     vector<SG::Regime> & regimeTuple,
		     const vector<double> & maxMovement,
		     const vector<SG::Regime> & maxMovementConstraints);

  //! Updates flags before the next iteration
  /*! This method checks whether or not the threat tuple has increased
      and sets the flags for recalculating binding continuation
      values. Also checks whether or not the cardinal direction has
      changed, and updates SGApprox::facingEastNorth and
      SGApprox::passNorth. */
  void updateFlags();

  //! Calculates the distance between revolutions
  /*! Returns the distance between successive iterations. Only runs
      when SGApprox::passNorth is true. Currently only runs
      when the number of SGTuple objects on successive revolutions is
      the same, and then takes the maximum over all distances between
      tuples that are in corresponding positions in the
      revolutions. */
  double distance(int newStart, int newEnd, int oldStart, int oldEnd) const;

  double distHelper(const SGPoint & p, 
		    const SGPoint & qA, 
		    const SGPoint & qB) const;

  //! Checks whether or not newDirection is shallower than best, relative to current
  /*! Returns true if the cosine between newDirection and best is
      greater than SGEnv::improveTol, or if best and newDirection
      are colinear, whether or not best has a larger norm. Non-static
      so it can read the parameter values in
      SGApprox::env. */
  bool improves(const SGPoint & current, 
		const SGPoint & best, 
		const SGPoint & newDirection) const;

  //! Outputs progress to the log file every iteration
  void logAppend(ofstream & logfs,
		 int iter, int rev, const SGTuple & tuple,
		 int state, int action);

public:
  //! Constructor for SGApprox class
  SGApprox(const SGEnv & _env,
	   const SGGame & _game,
	   SGSolution_PencilSharpening & _soln):
    env(_env), game(_game), soln(_soln),
    delta(game.getDelta()), numPlayers(game.getNumPlayers()),
    numStates(game.getNumStates()), errorLevel(1), sufficiencyFlag(true),
    nullAction(env)
  { }
  
  //! Prepares the approximation for generation
  /*! Opens the log file, constructs the actions array, initializes
      extremeTuples to a large "box" correspondence that contains the
      equilibrium payoff correspondence. Also initializes the pivot
      and the first direction. Sets the flags so that
      SGApprox::generate() will calculate new binding
      continuation values on the first pass. */
  void initialize();

  //! Returns the number of iterations thus far
  int getNumIterations() const {return numIterations; }
  //! Returns the number of revolutions of the pivot thus far
  int getNumRevolutions() const {return numRevolutions; }
  //! Returns the number of tuples in the extremeTuples array
  int getNumExtremeTuples() const {return extremeTuples.size(); }
  //! Returns the regime in which the best test direction was
  //! generated
  SG::Regime getBestRegime() const {return bestRegime; }
  //! Returns a constant iterator for the SGAction in which the best
  //! test direction was generated
  list<SGAction_PencilSharpening>::const_iterator  getBestAction() const { return bestAction; }
  //! Returns the best test direction
  const SGPoint & getBestDirection() const { return bestDirection; }
  //! Returns the current action tuple
  const vector<const SGAction_PencilSharpening*> & getActionTuple() const { return actionTuple; }
  //! Returns the current regime tuple
  const vector<SG::Regime> & getRegimeTuple() const { return regimeTuple; }
  //! Returns the current direction
  const SGPoint & getDirection() const { return currentDirection; }
  //! Returns the current pivot
  const SGTuple & getPivot() const {return pivot; }
  //! Returns the current threat tuple
  const SGTuple & getThreatTuple() const {return threatTuple; }
  //! Returns the array of SGAction objects that can currently be
  //! supported
  const vector< list<SGAction_PencilSharpening> > & getActions() const { return actions; }
  //! Returns the array of extreme tuples
  const vector<SGTuple> & getExtremeTuples() const {return extremeTuples; }

  //! Returns a string indicating the algorithms progress
  std::string progressString() const;

  //! Refines the approximation
  /*! Main public routine for the SGApprox class. Updates
   minimum IC continuation values and binding continuation values,
   finds the best new direction, advances the pivot, and resets the
   flags. Also returns the distance between revolutions when a
   revolution is completed. Otherwise, returns 1. */
  double generate(bool storeIteration = true);

  //! Algorithm just passed north.
  /*! Indicates whether or not the current direction passed north on
      this iteration. */
  bool passedNorth() const
  {return passNorth; }

  //! Destructor
  /*! Only purpose right now is to close the log file. */
  void end();

};


#endif
