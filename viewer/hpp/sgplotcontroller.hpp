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

#ifndef SGPLOTCONTROLLER_HPP
#define SGPLOTCONTROLLER_HPP

#include <QObject>
#include <QComboBox>
#include <QScrollBar>
#include "sgsolution.hpp"

//! Handles the plot settings for SGSolutionHandler
/*! This class intermediates between the controllers (iterSlider,
    stateCombo, actionCombo) and the plotting methods in the
    SGSSolutionHandler (i.e., plotSolution(...)).

    See also \ref viewersolutionsec.

  \ingroup viewer
*/
class SGPlotController : public QObject
{
  Q_OBJECT;
public:
  //! Plot mode for the detailPlot
  /*! Indicates whether to plot all of the test directions
   (Directions) or just plot the way the current pivot is generated
   (Generation). */
  enum PlotMode
    {
      Directions, /*!< Plot the test directions. */
      Generation /*!< Plot how the payoffs are generated. */
    };
  //! Solution mode
  /*! Indicates whether or not to plot all of the iterations from a
      user-defined start point to the current iteration (Progress) or
      just plot all of the iterations on the last revolution
      (Final). */
  enum SolutionMode
    {
      Progress, /*!< Plot the progress of the algorithm. */
      Final /*!< Plot the last revolution (the true correspondence) */
    };
  
private:
  //! Current state
  int state;
  //! Current action
  int action;
  //! Index of the current action within the list of actions in
  //! currentIter->actions[state]
  int actionIndex;
  //! The current iteration
  int iteration;

  //! The current solution object
  SGSolution * soln;
  
  //! The current plot mode
  PlotMode plotMode;
  
  //! The current solution mode
  SolutionMode mode;
  
  //! Points to the first SGIteration object from which to plot.
  list<SGIteration>::const_iterator startIter;
  //! Points to the last SGIteration object to which to plot.
  list<SGIteration>::const_iterator endIter;
  //! Points to the current SGiteration.
  list<SGIteration>::const_iterator currentIter;
  //! Points to the SGIteration that starts the last revolution.
  list<SGIteration>::const_iterator startOfLastRev;

  //! Indicates when an SGSolution object has been loaded
  bool solnLoaded;

  //! Points to the stateCombo QComboBox selector.
  QComboBox * stateCombo;
  //! Points to the actionCombo QComboBox selector.
  QComboBox * actionCombo;
  //! Points to the solutionMode QComboBox selector.
  QComboBox * solutionModeCombo;
  //! Points to the iterSlider QScrollBar for selecting the current iteration
  QScrollBar * iterSlider;
  //! Points to the startSlider QScrollBar for selecting the start
  //! iteration when the solution mode is Progress.
  QScrollBar * startSlider;
public:
  //! Constructor
  SGPlotController(QComboBox * _stateCombo,
		   QComboBox *_actionCombo,
		   QScrollBar * _iterSlider,
		   QScrollBar * _startSlider,
		   QComboBox * _solutionModeCombo);

  //! Access method for the current state.
  int getState() const { return state; }
  //! Access method for the current action.
  int getAction() const { return action; }
  //! Access method for the current action index.
  int getActionIndex() const { return actionIndex; }
  //! Access method for the current iteration number.
  int getIteration() const { return iteration; }
  //! Access method for the current plot mode.
  PlotMode getPlotMode() const { return plotMode; }
  //! Access method for the current solution mode.
  SolutionMode getMode () const { return mode; }
  //! Access method for the current SGIteration pointer.
  const SGIteration & getCurrentIter() const { return *currentIter; }
  //! Access method for the start SGIteration pointer.
  const SGIteration & getStartIter() const { return *startIter; }
  //! Access method for the end SGIteration pointer.
  const SGIteration & getEndIter() const { return *endIter; }
  //! Access method for the SGIteration pointer that starts the last revolution.
  const SGIteration & getStartOfLastRev() const { return *startOfLastRev; }
  //! Returns the position of the startSlider.
  int getStartSliderPosition() const { return startSlider->sliderPosition(); }
  //! Returns true if a solution has been loaded.
  bool hasSolution() const { return solnLoaded; }
  //! Returns the current solution.
  const SGSolution * getSolution() const { return soln; }
  //! Sets the solution.
  void setSolution(SGSolution * newSoln);
  //! Sets the current state.
  bool setState(int newState);
  //! Sets the plot mode.
  void setPlotMode(PlotMode newMode) { plotMode = newMode; }
  //! Sets the current action index.
  bool setActionIndex(int newActionIndex);
  //! Sets the current action.
  bool setAction(int newAction);
  //! Sets the current iteration.
  bool setIteration(int newIter);
  //! Increments the current iteration.
  void moveForwards()
  {
    if (!solnLoaded)
      return;
      
    iterSlider->setSliderPosition(std::max(iterSlider->minimum(),
					   iterSlider->value()-1));
    // iterSliderUpdate(iterSlider->value());
  }
  //! Decrements the current iteration.
  void moveBackwards()
  {
    if (!solnLoaded)
      return;
      
    iterSlider->setSliderPosition(std::min(iterSlider->maximum(),
					   iterSlider->value()+1));
    // iterSliderUpdate(iterSlider->value());
  }
  //! Sets the current iteration to be the one where the pivot in the
  //! given state is closest to point.
  void setCurrentIteration(SGPoint point, int state);
  //! Synchronizes sliders with controls.
  /*! Sets startIter and currentIter to be equal to the values
      indicated in the respective sliders. */
  void synchronizeSliders();

signals:
  //! Signal to the state and action models that the solution changed.
  void solutionChanged();
  //! Signal to the solutionHandler that the action changed.
  void actionChanged();
  //! Signal to solutionHandler that the iteration changed.
  void iterationChanged();

    
public slots:
  //! Toggles the solution mode
  void changeMode(int newMode);
  //! Slot when the iter and start sliders are moved.
  void iterSliderUpdate(int value);
  //! Slot used to set up the sliders when changing between Progress
  //! and Final modes.
  void setSliderRanges(int start, int end);
  //! Decrements the action.
  void prevAction();
  //! Increments the action.
  void nextAction();
  //! Sets the action
  void changeAction(int newAction);

  


}; // SGPlotController



#endif
