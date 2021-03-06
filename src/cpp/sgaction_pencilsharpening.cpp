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

// BAB 9-25-2012
#include "sgaction_pencilsharpening.hpp"

/* SGAction */

void SGAction_PencilSharpening::intersectRay(const SGPoint& pivot, 
			                      const SGPoint& direction)
{
  for (int player=0; player < points.size(); player++)
    intersectRaySegment(pivot,direction,points[player]);
}

void SGAction_PencilSharpening::trim(const SGPoint& pivot, 
		                      const SGPoint& direction)
{
  for (int player=0; player < points.size(); player++)
    intersectRaySegment(pivot,direction,trimmedPoints[player]);
}

void SGAction_PencilSharpening::updateTrim()
{
  points = trimmedPoints;
  for (int player = 0; player < 2; player++)
    {
      if (points[player].size() == 0)
	tuples[player] = vector<int>(0);
    }
}

double SGAction_PencilSharpening::distToTrimmed() const
{
  double dist = 0;
  for (int player = 0; player < 2; player++)
    {
      if (points[player].size() != trimmedPoints[player].size())
	return 1.0;

      for (int k = 0; k < points[player].size(); k++)
	{
	  for (int p = 0; p < numPlayers; p++)
	    dist = max(dist,abs(points[player][k][p]
				-trimmedPoints[player][k][p]));
	}
    }
  return dist;
}

void SGAction_PencilSharpening::intersectRaySegment(const SGPoint& pivot, 
				                     const SGPoint& direction,
				                     int player)
{
  intersectRaySegment(pivot,direction,points[player]);
  if (points[player].size() == 0)
    tuples[player] = vector<int>(0);
}

void SGAction_PencilSharpening::intersectRaySegment(const SGPoint& pivot, 
				                     const SGPoint& direction,
				                     SGTuple & segment)
{

  SGPoint normal = direction.getNormal();
  double level = pivot * normal;

  // First north south.
  if (segment.size() == 2)
    {
      // Two points of intersection for the ns IC constraint.
	      
      // Determine which of the two points is on the far side
      // of the clockwise ray.
      double l0 = normal * segment[0];
      double l1 = normal * segment[1];

      if (l0 > level + env.getParam(SG::ICTOL)
	  && l1 > level + env.getParam(SG::ICTOL))
	{
	  // Both lie above the ray.
	  segment.clear();
	}
      else if (l0 < level
	       && l1 < level)
	{
	  // Leave points alone.
	}
      else if (abs(l0 - l1)>env.getParam(SG::INTERSECTTOL))
	{
	  // Can take intersection.
	  double weightOn1 = (level - l0)/(l1 - l0);

	  if (weightOn1 > 1)
	    segment[0] = segment[1];
	  else if (weightOn1 < 0)
	    segment[1] = segment[0];
	  else
	    {
	      SGPoint intersection 
		= weightOn1 * segment[1] +
		(1.0 - weightOn1) * segment[0];
	      int replace1 = (l0 < l1);
	  
	      segment[replace1] = intersection;
	    }
	}
    }
  else
    {
      // No points of intersection. Do nothing.
      segment = SGTuple();
    }
}

void SGAction_PencilSharpening::calculateMinIC(const SGGame & game,
			                        const vector<bool> & update,
			                        const SGTuple & threatTuple)
{
  for (int player = 0;
       player < game.getNumPlayers();
       player++)
    {
      if (!update[player])
	continue;

      minIC[player] = SGAction_PencilSharpening::calculateMinIC(action,state,player,
					          game,threatTuple);
    }

} // calculateMinIC

double SGAction_PencilSharpening::calculateMinIC(int action,int state,int player,
				                  const SGGame & game,
				                  const SGTuple & threatTuple)
{
  vector<int> playersActions, playersDeviations;
  int deviation, deviationIndex;
  double currentGains;

  double minIC = -numeric_limits<double>::max();

  indexToVector(action,playersActions,
		game.getNumActions()[state]);
  playersDeviations = playersActions;
  
  for (deviation = 0;
       deviation < game.getNumActions()[state][player];
       deviation++)
    {
      playersDeviations[player] = deviation;

      deviationIndex = vectorToIndex(playersDeviations,
				     game.getNumActions()[state]);
      currentGains 
	= (1-game.getDelta())/game.getDelta() 
	* (game.getPayoffs()[state][deviationIndex][player] 
	   - game.getPayoffs()[state][action][player])
	+ threatTuple.expectation(game.getProbabilities()[state][deviationIndex], 
				  player);

      if (currentGains > minIC)
	minIC = currentGains;
    } // deviation

  return minIC;
}

void SGAction_PencilSharpening::calculateBindingContinuations(const vector<bool> & updatedThreatTuple,
					     const SGGame & game,
					     const vector<SGTuple> & extremeTuples,
					     const SGTuple & threatTuple,
					     const SGTuple & pivot,
					     const SGPoint & currentDirection,
					     int oldWest)
{
  // Calculates the IC intersection points. To be used after updating
  // the threat tuple.
  int numPlayers = 2;
  int tupleIndex;
  SGPoint intersection, point, nextPoint;

  reverse_iterator< vector<SGTuple>::const_iterator > tuple, nextTuple;
	      
  vector<SGTuple> newPoints(2);
  vector< vector<int> > newTuples(2,vector<int>(0,0));
  for (int player = 0; player < numPlayers; player++)
    {
      if (!updatedThreatTuple[player]
	  || game.getConstrained()[player])
	continue;

      tuples[player].clear(); 
      points[player].clear(); 
	      
      nextPoint = extremeTuples.back()
	.expectation(game.getProbabilities()
		     [state][action]);

      for (tuple = extremeTuples.rbegin(),
	     nextTuple = tuple+1,
	     tupleIndex = extremeTuples.size()-1; 
 	   tupleIndex > oldWest;
	   // nextTuple != extremeTuples.rend(); 
	   ++tuple,++nextTuple, --tupleIndex)
	{
	  point = nextPoint;
	  nextPoint = nextTuple->expectation(game.getProbabilities()
					     [state][action]);

	  double gap = point[player] - nextPoint[player];
	  if ( abs(gap) < env.getParam(SG::FLATTOL)
	       && abs(point[player] - minIC[player]) < env.getParam(SG::FLATTOL) )
	    {
	      // A flat.
	      newTuples[player].push_back(tupleIndex);
	      newTuples[player].push_back(tupleIndex - 1);
	      newPoints[player].push_back(point);
	      newPoints[player].push_back(nextPoint);
	    }
	  else if ( (point[player] <= minIC[player]
		     && minIC[player] < nextPoint[player])
		    || (point[player] >= minIC[player]
			&& minIC[player] > nextPoint[player]) )
	    {
	      // Points flank the minimum IC payoff
	      double alpha = (minIC[player] - nextPoint[player] ) / gap;
	      newTuples[player].push_back(tupleIndex);
	      newPoints[player].push_back((1-alpha)*nextPoint + alpha*point);
	    }

	  // Break when the payoff for this player is below
	  // but within env.getParam(SG::PASTTHREATTOL)/2.0 of the threat
	  // tuple
	  if ( tuple->strictlyLessThan(threatTuple,player) 
	       && !threatTuple
	       .strictlyLessThan(*tuple
				 +SGPoint(env.getParam(SG::PASTTHREATTOL)/2.0),
				 player) )
	    break;
	} // for point
    } // player
	
  for (int player = 0; player < numPlayers; player++)
    {	      
      if (updatedThreatTuple[player] 
	  && !game.getConstrained()[player])
	{
	  // Remove points that are not IC
	  int maxIndex, minIndex;
	  double maxOtherPayoff, minOtherPayoff;
	  newPoints[player].maxmin(1-player,maxOtherPayoff,maxIndex,
				   minOtherPayoff,minIndex);

	  if (maxOtherPayoff >= minIC[1-player])
	    {
	      points[player].push_back(newPoints[player][maxIndex]);
	      tuples[player].push_back(newTuples[player][maxIndex]);
	      if (minOtherPayoff < minIC[1-player])
		{
		  points[player].push_back(minIC);
		  tuples[player].push_back(-1);
		  corner = true;
		}
	      else
		{
		  points[player].push_back(newPoints[player][minIndex]);
		  tuples[player].push_back(newTuples[player][minIndex]);
		}

	      SGPoint expPivot 
		= pivot.expectation(game.getProbabilities()[state][getAction()]);
	      intersectRaySegment(expPivot,currentDirection,player);
	    }
	  // Otherwise, not IC.
	}
      else if (updatedThreatTuple[1-player]
	       && points[player].size()>0)
	{
	  if (points[player][0][1-player] >= minIC[1-player])
	    {
	      if (points[player][1][1-player] < minIC[1-player])
		{
		  points[player][1] = minIC;
		  tuples[player][1] = -1;
		}
	    }
	  else
	    {
	      points[player].clear();
	      tuples[player].clear();
	    }
	}
    } // for player

  for(int player= 0; player < numPlayers; player++)
    {
      assert((points[player].size()==0)
	     || (points[player].size()==2
		 && (points[player][0][1-player]
		     >= points[player][1][1-player]
		     -env.getParam(SG::PASTTHREATTOL))));
      assert(tuples[player].size() == points[player].size());
    }

} // calculateIntersections

