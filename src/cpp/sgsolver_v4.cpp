// This file is part of the SGSolve library for stochastic games
// Copyright (C) 2016 Benjamin A. Brooks
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

#include "sgsolver_v4.hpp"

SGSolver_V4::SGSolver_V4(const SGEnv & _env,
			 const SGGame & _game):
  env(_env),
  game(_game),
  soln(_game),
  numPlayers(_game.getNumPlayers()),
  numStates(_game.getNumStates()),
  delta(_game.getDelta()),
  payoffs(_game.getPayoffs()),
  eqActions(_game.getEquilibriumActions()),
  probabilities(_game.getProbabilities()),
  numActions(_game.getNumActions()),
  numActions_totalByState(_game.getNumActions_total())
{

}

void SGSolver_V4::solve()
{
  // Open a log file
  ofstream logofs("sgsolver_v4_test.log");
  logofs << scientific;
  
  // Initialize directions
  int numDirections = 200;

  // make an even multiple of 4
  numDirections += (4-numDirections%4);

  vector<SGPoint> directions(numDirections);
  vector< vector<double> > levels(numDirections,
				  vector<double>(numStates,0));
  vector<SGTuple> pivots(numDirections);

  SGPoint payoffLB, payoffUB;
  game.getPayoffBounds(payoffUB,payoffLB);

  SGTuple threatTuple(numStates,payoffLB);

  // Clear the solution
  soln.clear();
  
  // Initialize directions
  for (int dir = 0; dir < numDirections; dir++)
    {
      double theta = 2.0*PI
	*static_cast<double>(dir)/static_cast<double>(numDirections);
      directions[dir] = SGPoint(cos(theta),sin(theta));
    } // for dir

  // Print directions to the log
  // Initialize directions
  for (int dir = 0; dir < numDirections; dir++)
    logofs << directions[dir][0] << " ";
  logofs << endl;
  for (int dir = 0; dir < numDirections; dir++)
    logofs << directions[dir][1] << " ";
  logofs << endl;
  
  // Initialize actions with a big box as the feasible set
  vector< list<SGAction_V2> > actions(numStates);
  vector<bool> updateICFlags (2,true);
  cout << "Initial threatTuple: " << threatTuple << endl;
  
  for (int state = 0; state < numStates; state++)
    {
      for (int action = 0; action < numActions_totalByState[state]; action++)
	{
	  actions[state].push_back(SGAction_V2(env,state,action));
	  actions[state].back().calculateMinIC(game,updateICFlags,threatTuple);
	  actions[state].back().resetTrimmedPoints();
	  
	  for (int dir = 0; dir < 4; dir ++)
	    {
	      double theta = 2.0*PI*static_cast<double>(dir)/4.0;
	      SGPoint currDir = SGPoint(cos(theta),sin(theta));

	      // double level = max(currDir[0]*payoffLB[0],currDir[0]*payoffUB[0])
	      // 	+max(currDir[1]*payoffLB[1],currDir[1]*payoffUB[1]);

	      double level = max(currDir*payoffLB,currDir*payoffUB);
	      actions[state].back().trim(currDir,level);
	    } // for dir

	  actions[state].back().updateTrim();
	  
	  // cout << "(state,action)=(" << state << "," << action << "), "
	  //      << "numPoints = (" << actions[state].back().getPoints()[0].size()
	  //      << "," << actions[state].back().getPoints()[1].size() << ")"
	  //      << ", minIC: " << actions[state].back().getMinICPayoffs() << endl;
	}
    } // for state

  // Main loop
  double errorLevel = 1;
  int numIter = 0;

  SGTuple pivot = threatTuple;
  SGTuple feasibleTuple = threatTuple; // A payoff tuple that is feasible for APS

  SGIteration_V2 iter;
  
  while (errorLevel > env.getParam(SG::ERRORTOL)
	 && numIter < env.getParam(SG::MAXITERATIONS) )
    {
      vector<SGActionIter> actionTuple(numStates);
      // Pick the initial actions arbitrarily
      for (int state = 0; state < numStates; state++)
	actionTuple[state] = actions[state].begin();
      
      vector<SG::Regime> regimeTuple(numStates,SG::Binding);

      // Reset the trimmed points for the actions
      for (int state = 0; state < numStates; state++)
	{
	  for (auto ait = actions[state].begin();
	       ait != actions[state].end();
	       ++ait)
	    {
	      ait->updateTrim();

	      // WARNING: NO GUARANTEE THAT THIS POINT IS FEASIBLE
	      if (!(ait->supportable(feasibleTuple.expectation(probabilities[state]
							       [ait->getAction()]))))
		{
		  actions[state].erase(ait++);
		  continue;
		}
	      
	    } // for ait
	} // for state

      if (env.getParam(SG::STOREITERATIONS))
	iter = SGIteration_V2(numIter,actions,threatTuple);
      
      // Reset the error level
      errorLevel = 0;
      
      // Iterate through directions
      for (int dir = 0; dir < directions.size(); dir++)
	{
	  // Compute optimal level in this direction
	  vector<double> newLevels(numStates,0);
	  SGPoint currDir = directions[dir];

	  optimizePolicy(pivot,actionTuple,regimeTuple,currDir,
			 actions,feasibleTuple);

	  for (int state = 0; state < numStates; state++)
	    {
	      // Update the levels and the error level with the
	      // movement in this direction
	      newLevels[state] = pivot[state]*currDir;
	      pivots[dir] = pivot;
	      errorLevel = max(errorLevel,abs(newLevels[state]-levels[dir][state]));
	    } // for state

	  if (env.getParam(SG::STOREITERATIONS))
	    iter.push_back(SGStep(actionTuple,regimeTuple,pivot,
				  SGHyperplane(currDir,newLevels),actions));
	  
	  levels[dir] = newLevels;
	} // for dir


      cout << "Iter: " << numIter
	   << ", errorLvl: " << scientific << errorLevel
	   << ", remaining actions: ( ";
      for (int state = 0; state < numStates; state++)
	cout << actions[state].size() << " ";
      cout << ")" << endl;
      
      if (env.getParam(SG::STOREITERATIONS))
	soln.push_back(iter); // Important to do this before updating the threat point and minIC of the actions

      findFeasibleTuple(feasibleTuple,actions);
      
      // Update the the threat tuple
      for (int state = 0; state < numStates; state++)
	{
	  threatTuple[state][0] = -levels[numDirections/2][state];
	  threatTuple[state][1] = -levels[3*numDirections/4][state];
	}
      // cout << "New threat tuple: " << threatTuple << endl;

      // Recalculate minimum IC continuation payoffs
      for (int state = 0; state < numStates; state++)
	{
	  for (auto ait = actions[state].begin();
	       ait != actions[state].end();
	       ++ait)
	    {
	      ait->calculateMinIC(game,updateICFlags,threatTuple);
	      
	      for (int dir = 0; dir < directions.size(); dir++)
		{
		  // Compute the expected level
		  double expLevel = 0;
		  for (int sp = 0; sp < numStates; sp++)
		    expLevel += probabilities[state][ait->getAction()][sp]
		      * levels[dir][sp];
		  
		  // Trim the action
		  ait->trim(directions[dir],expLevel);
		} // for dir
	      // Trim the actions
	    } // for ait
	  
	} // for state

      // Print new levels to the log file
      for (int state = 0; state < numStates; state++)
	{
	  for (int dir = 0; dir < numDirections; dir++)
	    logofs << levels[dir][state] << " ";
	  logofs << endl;
	  for (int dir = 0; dir < numDirections; dir++)
	    logofs << pivots[dir][state][0] << " ";
	  logofs << endl;
	  for (int dir = 0; dir < numDirections; dir++)
	    logofs << pivots[dir][state][1] << " ";
	  logofs << endl;
	}

      numIter++;
    } // while

  cout << "Converged!" << endl;

  logofs.close();
  
} // solve

void SGSolver_V4::solve_endogenous()
{
  list<SGPoint> directions;
  list< vector<double> > levels;
  list<SGPoint> newDirections;
  list< vector<double> > newLevels;

  SGPoint dueEast(1.0,0.0);
  SGPoint dueNorth(0.0,1.0);
  
  SGPoint payoffLB, payoffUB;
  game.getPayoffBounds(payoffUB,payoffLB);

  SGTuple threatTuple(numStates,payoffLB),
    newThreatTuple(numStates,payoffLB); // newThreatTuple holds the
					// calculation of the next
					// threat point

  // Clear the solution
  soln.clear();
  
  // Initialize actions with a big box as the feasible set
  vector< list<SGAction_V2> > actions(numStates);
  vector<bool> updateICFlags (2,true);
  
  for (int state = 0; state < numStates; state++)
    {
      for (int action = 0; action < numActions_totalByState[state]; action++)
	{
	  actions[state].push_back(SGAction_V2(env,state,action));
	  actions[state].back().calculateMinIC(game,updateICFlags,threatTuple);
	  actions[state].back().resetTrimmedPoints();
	  
	  for (int dir = 0; dir < 4; dir ++)
	    {
	      double theta = 2.0*PI*static_cast<double>(dir)/4.0;
	      SGPoint currDir = SGPoint(cos(theta),sin(theta));

	      double level = max(currDir*payoffLB,currDir*payoffUB);
	      actions[state].back().trim(currDir,level);
	    } // for dir

	  actions[state].back().updateTrim();
	  
	}
    } // for state

  // Main loop
  double errorLevel = 1;
  int numIter = 0;

  SGTuple pivot = threatTuple;
  SGTuple feasibleTuple = threatTuple; // A payoff tuple that is feasible for APS

  SGIteration_V2 iter;
  
  while (errorLevel > env.getParam(SG::ERRORTOL)
	 && numIter < env.getParam(SG::MAXITERATIONS) )
    {
      // Clear the directions and levels
      newDirections.clear();
      newLevels.clear();
      
      vector<SGActionIter> actionTuple(numStates);
      // Pick the initial actions arbitrarily
      for (int state = 0; state < numStates; state++)
	actionTuple[state] = actions[state].begin();
      
      vector<SG::Regime> regimeTuple(numStates,SG::Binding);

      if (env.getParam(SG::STOREITERATIONS))
	iter = SGIteration_V2 (numIter,actions,threatTuple);
      
      // Iterate through directions
      SGPoint currDir = SGPoint(1,0); // Start pointing due east
      bool passEast = false;
      while (!passEast)
	{
	  // Compute optimal level in this direction
	  optimizePolicy(pivot,actionTuple,regimeTuple,currDir,
			 actions,feasibleTuple);

	  // Do sensitivity analysis to find the next direction
	  SGPoint normDir = currDir.getNormal();
	  double bestLevel = sensitivity(pivot,actionTuple,regimeTuple,
				       currDir,actions);

	  SGPoint newDir = 1.0/(bestLevel+1.0)*currDir
	    + bestLevel/(bestLevel+1.0)*normDir;
	  newDir /= newDir.norm();
	  
	  newLevels.push_back(vector<double>(numStates,0));
	  newDirections.push_back(newDir);
	  for (int state = 0; state < numStates; state++)
	    {
	      newLevels.back()[state] = pivot[state]*newDir;
	    } // for state
	  if (env.getParam(SG::STOREITERATIONS))
	    iter.push_back(SGStep(actionTuple,regimeTuple,pivot,
				  SGHyperplane(newDir,newLevels.back()),actions));

	  // Move the direction slightly to break ties
	  newDir.rotateCCW(PI*1e-3);

	  // If new direction passes due west or due south, update the
	  // corresponding threat tuple using the current pivot
	  if (currDir*dueNorth > 0 && newDir*dueNorth <= 0) // Passing due west
	    {
	      for (int state = 0; state < numStates; state++)
		newThreatTuple[state][0] = pivot[state][0];
	    }
	  else if (currDir*dueEast < 0 && newDir*dueEast >= 0) // Passing due south
	    {
	      for (int state = 0; state < numStates; state++)
		newThreatTuple[state][1] = pivot[state][1];
	    }
	  else if (currDir*dueNorth < 0 && newDir*dueNorth >= 0)
	    {
	      passEast = true;
	    }

	  
	  currDir = newDir;
	} // while !passEast

      if (env.getParam(SG::STOREITERATIONS))
	soln.push_back(iter); // Important to do this before updating the threat point and minIC of the actions

      // Recompute the error level
      errorLevel = 0;
      {
	// Rather heavy handed, but do this for now.

	  auto dir0 = directions.cbegin(), dir1 = newDirections.cbegin();
	  auto lvl0 = levels.cbegin(), lvl1 = newLevels.cbegin();
	  while (dir1 != newDirections.cend()
		 && lvl1 != newLevels.cend())
	    {
	      dir0 = directions.cbegin();
	      lvl0 = levels.cbegin();
	      double minDist = numeric_limits<double>::max();
	      while (dir0 != directions.cend()
		     && lvl0 != levels.cend() )
		{
		  double tmp = 0;
		  for (int state = 0; state < numStates; state++)
		    tmp = max(tmp,abs((*lvl0)[state]-(*lvl1)[state]));
		  
		  minDist = min(minDist,SGPoint::distance(*dir0,*dir1)+tmp);
		  
		  dir0++;
		  lvl0++;
		}
	      errorLevel = max(errorLevel,minDist);
	      
	      dir1++;
	      lvl1++;
	    }
	}
      
      // Print summary of iteration
      cout << "Iter: " << numIter
	   << ", errorLvl: " << scientific << errorLevel
	   << ", remaining actions: ( ";
      for (int state = 0; state < numStates; state++)
	cout << actions[state].size() << " ";
      cout << ")"
	   << ", numDirections = " << newDirections.size()
	   << endl;
      
      findFeasibleTuple(feasibleTuple,actions);
      
      
      // Update the the threat tuple, directions, levels
      threatTuple = newThreatTuple;
      directions = newDirections;
      levels = newLevels;
      
      // Recalculate minimum IC continuation payoffs
      for (int state = 0; state < numStates; state++)
	{
	  for (auto ait = actions[state].begin();
	       ait != actions[state].end();
	       ++ait)
	    {
	      ait->calculateMinIC(game,updateICFlags,threatTuple);

	      // Trim the actions
	      auto dir = directions.cbegin();
	      auto lvl = levels.cbegin();
	      while (dir != directions.cend()
		     && lvl != levels.cend())
		{
		  // Trim the action
		  double expLevel = 0;
		  for (int sp = 0; sp < numStates; sp++)
		    expLevel += probabilities[state][ait->getAction()][sp]
		      * (*lvl)[sp];

		  ait->trim(*dir,expLevel);

		  dir++;
		  lvl++;
		} // for dir, lvl
	      ait->updateTrim();

	      // Delete the action if not supportable
	      if (!(ait->supportable(feasibleTuple.expectation(probabilities[state]
							       [ait->getAction()]))))
		{
		  actions[state].erase(ait++);
		  continue;
		}
	    } // for ait
	  
	} // for state

      numIter++;
    } // while

  cout << "Converged!" << endl;
  
} // solve_endogenous



void SGSolver_V4::optimizePolicy(SGTuple & pivot,
				 vector<SGActionIter> & actionTuple,
				 vector<SG::Regime> & regimeTuple,
				 const SGPoint currDir,
				 const vector<list<SGAction_V2> > & actions,
				 const SGTuple & feasibleTuple) const
{
  // Do policy iteration to find the optimal pivot.
  
  double pivotError = 1.0;
  int numPolicyIters = 0;
  // Iterate until convergence
  SGTuple newPivot(numStates);

  vector<SGActionIter> newActionTuple(actionTuple);
  vector<SG::Regime> newRegimeTuple(regimeTuple);

  vector<bool> bestAPSNotBinding(numStates,false);
  vector<SGPoint> bestBindingPayoffs(numStates);
  
  // policy iteration
  do
    {
      pivotError = 0;

      // Look in each state for improvements
      for (int state = 0; state < numStates; state++)
	{
	  double bestLevel = -numeric_limits<double>::max();
	  
	  for (auto ait = actions[state].begin();
	       ait != actions[state].end();
	       ++ait)
	    {
	      // Procedure to find an improvement to the policy
	      // function
	      
	      SGPoint nonBindingPayoff = (1-delta)*payoffs[state]
		[ait->getAction()]
		+ delta * pivot.expectation(probabilities[state][ait->getAction()]);

	      bool APSNotBinding = false;
	      SGPoint bestAPSPayoff;

	      // Find which payoff is highest in current normal and
	      // break ties in favor of the clockwise 90 degree.
	      int bestBindingPlayer = -1;
	      int bestBindingPoint = -1;
	      double bestBindLvl = -numeric_limits<double>::max();
	      for (int p = 0; p < numPlayers; p++)
		{
		  for (int k = 0; k < ait->getPoints()[p].size(); k++)
		    {
		      double tmpLvl = ait->getPoints()[p][k]*currDir;
		      if (tmpLvl > bestBindLvl)
			{
			  bestBindLvl = tmpLvl;
			  bestBindingPlayer = p;
			  bestBindingPoint = k;
			}
		    } // point
		} // player

	      if (bestBindingPlayer < 0 // didn't find a binding payoff
		  || (ait->getBndryDirs()[bestBindingPlayer][bestBindingPoint]
		      *currDir > 1e-8) // Can improve on the best
				       // binding payoff by moving in
				       // along the frontier
		  )
		{
		  APSNotBinding = true;
		}
	      else // Found a binding payoff
		bestAPSPayoff =  (1-delta)*payoffs[state][ait->getAction()]
		  + delta * ait->getPoints()[bestBindingPlayer][bestBindingPoint];

	      if ( APSNotBinding // NB bestAPSPayoff has only been
				    // set if bestAPSNotBinding ==
				    // false
		   || (bestAPSPayoff*currDir > nonBindingPayoff*currDir -1e-7) ) 
		{
		  // ok to use non-binding payoff
		  if (nonBindingPayoff*currDir > bestLevel)
		    {
	      	      bestLevel = nonBindingPayoff*currDir;

		      bestAPSNotBinding[state] = APSNotBinding;
		      if (!APSNotBinding)
			bestBindingPayoffs[state] = bestAPSPayoff;
		      
		      newActionTuple[state] = ait;
	      	      newRegimeTuple[state] = SG::NonBinding;
	      	      newPivot[state] = nonBindingPayoff;
		    }
		}
	      else if (bestAPSPayoff*currDir < nonBindingPayoff*currDir +1e-7)
		{
		  if (bestAPSPayoff*currDir > bestLevel)
		    {
	      	      bestLevel = bestAPSPayoff*currDir;
		      newActionTuple[state] = ait;
	      	      newRegimeTuple[state] = SG::Binding;
	      	      newPivot[state] = bestAPSPayoff;
		    }
		}
	    } // ait

	  pivotError = max(pivotError,abs(bestLevel-pivot[state]*currDir));
	} // state

      pivot = newPivot;
      actionTuple = newActionTuple;
      regimeTuple = newRegimeTuple;

      // Inner loop to fix regimes
      bool anyViolation = false;
      do
	{
	  // Do Bellman iteration to find new fixed point
	  policyToPayoffs(pivot,actionTuple,regimeTuple);

	  anyViolation = false;

	  // Check if any reversals between best binding and non-binding

	  // First determine the maximum gap
	  vector<double> gaps(numStates,0);
	  double maxGap = 0;
	  for (int state = 0; state < numStates; state++)
	    {
	      if (!bestAPSNotBinding[state]
		  && regimeTuple[state] == SG::NonBinding) 
		{
		  gaps[state] = pivot[state]*currDir - bestBindingPayoffs[state]*currDir;
		  if (gaps[state] > maxGap)
		    {
		      anyViolation = true;
		      maxGap = gaps[state];
		    }
		}
	    }
	  
	  for (int state = 0; state < numStates; state++)
	    {
	      if (!bestAPSNotBinding[state]
		  && regimeTuple[state] == SG::NonBinding
		  && gaps[state] >= delta*maxGap) 
		{
		  pivot[state] = bestBindingPayoffs[state];
		  regimeTuple[state] = SG::Binding;
		}
	    }
	} while (anyViolation);

      // numPolicyIters++;
    } while (pivotError > env.getParam(SG::POLICYITERTOL)
	     && ++numPolicyIters < env.getParam(SG::MAXPOLICYITERATIONS));

  if (numPolicyIters >= env.getParam(SG::MAXPOLICYITERATIONS))
    cout << "WARNING: Maximum policy iterations reached." << endl;

} // optimizePolicy

double SGSolver_V4::sensitivity(const SGTuple & pivot,
				const vector<SGActionIter> & actionTuple,
				const vector<SG::Regime> & regimeTuple,
				const SGPoint currDir,
				const vector<list<SGAction_V2> > & actions) const
{
  SGPoint normDir = currDir.getNormal();
  
  double nonBindingIndiffLvl = -1;
  double bindingIndiffLvl = -1;
  double bestLevel = numeric_limits<double>::max()-1.0;
  
  // Look in each state for improvements
  for (int state = 0; state < numStates; state++)
    {

      for (auto ait = actions[state].begin();
	   ait != actions[state].end();
	   ++ait)
	{
	  // Find the smallest weight on normDir such that this action
	  // improves in that direction. For each of the binding 

	  SGPoint nonBindingPayoff = (1-delta)*payoffs[state]
	    [ait->getAction()]
	    + delta * pivot.expectation(probabilities[state][ait->getAction()]);

	  // Calculate the lvl at which indifferent to the pivot
	  // pivot[state]*(currDir+tmp*normDir)<=nonBindingPayoff*(currDir+tmp*normDir);
	  // (pivot[state]-nonBindingPayoff)*currDir<=-tmp*normDir*(pivot[state]-nonBindingPayoff)
	  double denom = normDir*(nonBindingPayoff-pivot[state]);
	  double numer = (pivot[state]-nonBindingPayoff)*currDir;
	  if (SGPoint::distance(pivot[state],nonBindingPayoff) > 1e-6
	      && abs(denom) > 1e-10)
	    {
	      nonBindingIndiffLvl = numer/denom;

	      if (nonBindingIndiffLvl < bestLevel
		  && nonBindingIndiffLvl > -1e-6)
		{
		  SGPoint indiffDir = currDir + normDir * nonBindingIndiffLvl;

		  // See if a binding payoff is higher in the
		  // indifference direction
		  double bestBindLvl = -numeric_limits<double>::max();
		  int bestBindingPlayer = -1;
		  int bestBindingPoint = 0;
		  for (int p = 0; p < numPlayers; p++)
		    {
		      for (int k = 0; k < ait->getPoints()[p].size(); k++)
			{
			  double tmpLvl = ait->getPoints()[p][k]*indiffDir;
			  if (tmpLvl > bestBindLvl
			      || (tmpLvl > bestBindLvl-1e-8
				  && ait->getPoints()[p][k]*normDir >= 0))
			    {
			      bestBindLvl = tmpLvl;
			      bestBindingPlayer = p;
			      bestBindingPoint = k;
			    }
			} // point
		    } // player

		  bool bestAPSNotBinding = false;
		  if (bestBindingPlayer < 0 // didn't find a binding payoff
		      || (ait->getBndryDirs()[bestBindingPlayer][bestBindingPoint]
			  *indiffDir > 1e-6) // Can improve on the
		      // best binding payoff by
		      // moving in along the
		      // frontier
		      )
		    {
		      bestAPSNotBinding = true;
		    }


		  if ( bestAPSNotBinding // NB bestAPSPayoff has only been
		       // set if bestAPSNotBinding ==
		       // true
		       || bestBindLvl > nonBindingPayoff*indiffDir - 1e-10) 
		    {
		      // If we get to here, non-binding regime is
		      // available in the indifferent direction, and
		      // this direction is smaller than the best level
		      // found so far.

		      if ( (ait != actionTuple[state] && denom> 1e-6)
			   || (ait == actionTuple[state]
			       && denom < -1e-6
			       && regimeTuple[state] == SG::Binding) )
			bestLevel = nonBindingIndiffLvl;
		    }
		} // Non-binding indifference level is smaller than best level
	    } // Positive level of indifference


	      // Now check the binding directions
	  for (int p = 0; p < numPlayers; p++)
	    {
	      for (int k = 0; k < ait->getPoints()[p].size(); k++)
		{
		  SGPoint bindingPayoff = (1-delta)*payoffs[state]
		    [ait->getAction()]
		    + delta * ait->getPoints()[p][k];
		  double denom = normDir*(bindingPayoff-pivot[state]);
		  double numer = (pivot[state]-bindingPayoff)*currDir;
		  if (SGPoint::distance(pivot[state],bindingPayoff)>1e-6
		      && abs(denom) > 1e-10)
		    {
		      bindingIndiffLvl = numer/denom;

		      if (bindingIndiffLvl < bestLevel
			  && bindingIndiffLvl > -1e-6)
			{
			  SGPoint indiffDir = currDir + normDir * bindingIndiffLvl;

			  if (nonBindingPayoff*indiffDir
			      >= bindingPayoff*indiffDir-1e-6)
			    {
			      if ( (ait != actionTuple[state]
				    && denom > 1e-6 )
				   || (ait == actionTuple[state]
				       && (regimeTuple[state]==SG::NonBinding
					   && denom < -1e-6 )
				       || (regimeTuple[state]==SG::Binding
					   && denom > 1e-6) ) )
				bestLevel = bindingIndiffLvl;
			    } // Binding payoff is available
			} // Smaller than the current bestLvl
		    } // Denominator is positive
		} // point
	    } // player
	} // ait

    } // state

  return std::max(bestLevel,0.0);

} // sensitivity


void SGSolver_V4::findFeasibleTuple(SGTuple & feasibleTuple,
				    const vector<list<SGAction_V2> > & actions) const
{
  // Update the APS-feasible tuple

  // These are just in case we cannot find binding APS payofs. NB not
  // the same as the actionTuple and regimeTuple that determine the
  // pivot.
  vector<SGActionIter> actionTuple(numStates);
  vector< SG::Regime > regimeTuple(numStates,SG::Binding);

  bool anyNonBinding = false;
  for (int state = 0; state < numStates; state++)
    {
      // Search for an action with feasible binding continuations
      // and just pick one. If we don't find any action with
      // feasible binding continuations, it either means (i) the
      // game has no pure strategy SPNE, or (ii) any feasible
      // payoff tuple (within the set of remaining actions) is an
      // APS payoff.
      bool foundFeasiblePlayer = false;
      for (auto ait = actions[state].begin();
	   ait != actions[state].end();
	   ait++)
	{
	  int feasiblePlayer = -1;
	  for (int player = 0; player < numPlayers; player++)
	    {
	      if (ait->getPoints()[player].size()>0)
		{
		  feasiblePlayer = player;
		  break;
		}
	    }
	  if (feasiblePlayer >= 0)
	    {
	      feasibleTuple[state] = (1-delta)*payoffs[state][ait->getAction()]
		+delta*ait->getPoints()[feasiblePlayer][0];
	      foundFeasiblePlayer = true;
	      break;
	    }
	} // for ait
      if (!foundFeasiblePlayer)
	{
	  regimeTuple[state] = SG::NonBinding;
	  actionTuple[state] = actions[state].begin();
	  anyNonBinding = true;
	}
    } // for state

  bool notAllIC = anyNonBinding;
  while (notAllIC)
    {
      // Have to do Bellman iteration to find the new pivot.
      policyToPayoffs(feasibleTuple,actionTuple,regimeTuple);
      notAllIC = false;
      
      // Check if resulting tuple is IC.
      for (int state = 0; state < numStates; state++)
	{
	  if (!(feasibleTuple.expectation(probabilities[state]
					  [actionTuple[state]->getAction()])
		>= actionTuple[state]->getMinICPayoffs()))
	    {
	      notAllIC = true;
	      // Try advancing the action and recomputing
	      if ((++actionTuple[state])==actions[state].end())
		throw(SGException(SG::NOFEASIBLETUPLE));
	    }
	} // for state
    } // if anyNonBinding

} // findFeasibleTuple

void SGSolver_V4::policyToPayoffs(SGTuple & pivot,
				  const vector<SGActionIter>  & actionTuple,
				  const vector<SG::Regime> & regimeTuple) const
{
  // Do Bellman iteration to find new fixed point
  int updatePivotPasses = 0;
  double bellmanPivotGap = 0;
  SGTuple newPivot(pivot);

  do
    {
      for (int state = 0; state < numStates; state++)
	{
	  if (regimeTuple[state] == SG::NonBinding)
	    newPivot[state] = (1-delta)*payoffs[state]
	      [actionTuple[state]->getAction()]
	      + delta*pivot.expectation(probabilities[state]
					[actionTuple[state]->getAction()]);
	}
      bellmanPivotGap = SGTuple::distance(newPivot,pivot);
      pivot = newPivot;
    } while (bellmanPivotGap > env.getParam(SG::UPDATEPIVOTTOL)
	     && (++updatePivotPasses < env.getParam(SG::MAXUPDATEPIVOTPASSES) ));

  if (updatePivotPasses == env.getParam(SG::MAXUPDATEPIVOTPASSES) )
    cout << "WARNING: Maximum pivot update passes reached." << endl;
  
} // policyToPayoffs
