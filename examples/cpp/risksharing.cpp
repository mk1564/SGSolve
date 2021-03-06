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

//! Kocherlakota style risk sharing model
#include "sgrisksharing.hpp"
#include "sgsimulator.hpp"

int main()
{
  double delta = 0.85;
  int numEndowments = 5;
  int c2e = 25;
  int numSims = 1e3;
  int numSimIters = 1e5;
  RiskSharingGame::EndowmentMode endowmentMode = RiskSharingGame::Consumption;

  {
    double persistence = 0;
    RiskSharingGame rsg(delta,numEndowments,
			c2e,persistence,endowmentMode);
    SGEnv env;
    env.setParam(SG::STOREITERATIONS,1);
    env.setParam(SG::STOREACTIONS,false);
    SGGame game(rsg);
    SGSolver_PencilSharpening solver(env,game);
    solver.solve();

  }

  return 0;

  stringstream nashname, lrpname, prename, name, gamename, solnname;
  prename << "_ne="
	  << numEndowments
	  << "_c2e="
	  << c2e
	  << "_d="
	  << setprecision(3) << delta;
  switch (endowmentMode)
    {
    case RiskSharingGame::Consumption:
      prename << "_cmode=C";
      break;
    case RiskSharingGame::Endowment:
      prename << "_cmode=E";
      break;
    }

  
  lrpname << "./logs/rsg_lrp" << prename.str() << ".log";
  nashname << "./logs/rsg_nash" << prename.str() << ".log";
  ofstream ofs_lrp(lrpname.str().c_str(),std::ofstream::out);
  ofstream ofs_nash(nashname.str().c_str(),std::ofstream::out);

  SGSolver_PencilSharpening * solver;
  
  for (double persistence = 0.0; persistence < 0.125; persistence += 0.25)
    {
      try
	{
	  cout << "Starting computation with p="
	       << setprecision(3) << persistence << "." << endl;
	  
	  RiskSharingGame rsg(delta,numEndowments,
			      c2e,persistence,endowmentMode);
	  SGGame game(rsg);

	  name.str(""); gamename.str(""); solnname.str("");
	  name << "_ne="
	       << numEndowments
	       << "_c2e="
	       << c2e
	       << "_d="
	       << setprecision(3) << delta
	       << "_p="
	       << setprecision(3) << persistence;
	  switch (endowmentMode)
	    {
	    case RiskSharingGame::Consumption:
	      name << "_cmode=C";
	      break;
	    case RiskSharingGame::Endowment:
	      name << "_cmode=E";
	      break;
	    }

	  gamename << "./games/rsg"
		   << name.str()
		   << ".sgm";
	  solnname << "./solutions/rsg"
		   << name.str()
		   << ".sln";

	  SGSolution_PencilSharpening soln;
	  try
	    {
	      cout << "Trying to load: " << solnname.str() << endl;
		
	      SGSolution_PencilSharpening::load(soln,solnname.str().c_str());
	    }
	  catch (std::exception & ep)
	    {
	      cout << "Caught the following exception:" << endl
		   << ep.what() << endl;
	      SGGame::save(game,"./games/risksharing.sgm");

	      SGEnv env;
	      env.setParam(SG::STOREITERATIONS,1);
	      env.setParam(SG::STOREACTIONS,false);
	      solver = new SGSolver_PencilSharpening(env,game);

	      solver->solve();
	      soln = solver->getSolution();
	      SGSolution_PencilSharpening::save(soln,"./solutions/risksharing.sln");
	      delete solver;
	    }

	  // Locate the socially efficient and inefficient equilibria
	  int midPoint = (numEndowments-1)/2;
	  SGPoint northWest(1.0,1.0);
	  double bestLevel = -numeric_limits<double>::max(),
	    worstLevel = numeric_limits<double>::max();
	  list<SGIteration_PencilSharpening>::const_iterator bestIter, worstIter;
	  for (list<SGIteration_PencilSharpening>::const_iterator iter
		 = soln.getIterations().begin();
	       iter != soln.getIterations().end();
	       ++iter)
	    {
	      double tempLevel = northWest*(iter->getPivot()[midPoint]);
	      if (tempLevel < worstLevel)
		{
		  worstLevel = tempLevel;
		  worstIter = iter;
		}
	      if (tempLevel > bestLevel)
		{
		  bestLevel = tempLevel;
		  bestIter = iter;
		}
	    } // for iter

	  ofs_lrp << persistence << " ";

	  SGSimulator sim(soln);
	  sim.initialize();

	  sim.simulate(numSims,numSimIters,midPoint,bestIter->getIteration());
	  SGPoint lrp = sim.getLongRunPayoffs();
	  ofs_lrp << setprecision(6) << lrp[0] << " " << lrp[1] << " ";
	  cout << "Best long run payoffs: ("
	       << setprecision(6) << lrp[0] << ","
	       << lrp[1] << ")" << endl;
	    
	  sim.simulate(numSims,numSimIters,midPoint,worstIter->getIteration());
	  lrp = sim.getLongRunPayoffs();
	  ofs_lrp << setprecision(6) << lrp[0] << " " << lrp[1] << endl;
	  cout << "Autarky payoffs: ("
	       << setprecision(6) << lrp[0] << ","
	       << lrp[1] << ")" << endl;

	  // Calculate nash payoffs, where threat point is autarky
	  const SGTuple & threatTuple = soln.getIterations().back().getThreatTuple();
	  for (int e = 0; e < numEndowments; e++)
	    {
	      double nashObj = -numeric_limits<double>::max();
	      SGPoint nashPayoffs;
	      for (list<SGIteration_PencilSharpening>::const_iterator iter
		     = soln.getIterations().begin();
		   iter != soln.getIterations().end();
		   ++iter)
		{
		  double tempObj = ( iter->getPivot()[e][0] - threatTuple[e][0] )
		    * ( iter->getPivot()[e][1] - threatTuple[e][1] );
		  if (tempObj > nashObj)
		    {
		      nashObj = tempObj;
		      nashPayoffs = iter->getPivot()[e];
		    }
		} // for iter
	      ofs_nash << nashPayoffs[0] << " " << nashPayoffs[1] << " ";
	    } // for e
	  ofs_nash << endl;
	  
	}
      catch (SGException & e)
	{
	  if (e.getType() == SG::NO_ADMISSIBLE_DIRECTION)
	    continue;
	  else
	    throw;
	}
      catch (std::exception & e)
	{
	  SGSolution_PencilSharpening soln = solver->getSolution();
	  SGSolution_PencilSharpening::save(soln,solnname.str().c_str());
	  delete solver;
	  cout << "SGException caught. Received following error: "
	       << endl
	       << e.what()
	       << endl;
	  break;
	}
    } // for persistence
  ofs_lrp.close();
  ofs_nash.close();

  return 0;
}
