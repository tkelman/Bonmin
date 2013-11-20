// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CbcHeuristicDive.hpp"
#include "CbcStrategy.hpp"
#include  "CoinTime.hpp"

//#define DIVE_FIX_BINARY_VARIABLES

// Default Constructor
CbcHeuristicDive::CbcHeuristicDive() 
  :CbcHeuristic()
{
  // matrix and row copy will automatically be empty
  downLocks_ =NULL;
  upLocks_ = NULL;
  percentageToFix_ = 0.2;
  maxIterations_ = 100;
  maxTime_ = 60;
}

// Constructor from model
CbcHeuristicDive::CbcHeuristicDive(CbcModel & model)
  :CbcHeuristic(model)
{
  downLocks_ =NULL;
  upLocks_ = NULL;
  // Get a copy of original matrix
  assert(model.solver());
  // model may have empty matrix - wait until setModel
  const CoinPackedMatrix * matrix = model.solver()->getMatrixByCol();
  if (matrix) {
    matrix_ = *matrix;
    matrixByRow_ = *model.solver()->getMatrixByRow();
    validate();
  }
  percentageToFix_ = 0.2;
  maxIterations_ = 100;
  maxTime_ = 60;
}

// Destructor 
CbcHeuristicDive::~CbcHeuristicDive ()
{
  delete [] downLocks_;
  delete [] upLocks_;
}

// Create C++ lines to get to current state
void 
CbcHeuristicDive::generateCpp( FILE * fp, const char * heuristic) 
{
  // hard coded as CbcHeuristic virtual
  CbcHeuristic::generateCpp(fp,heuristic);
  if (percentageToFix_!=0.2)
    fprintf(fp,"3  %s.setPercentageToFix(%.f);\n",heuristic,percentageToFix_);
  else
    fprintf(fp,"4  %s.setPercentageToFix(%.f);\n",heuristic,percentageToFix_);
  if (maxIterations_!=100)
    fprintf(fp,"3  %s.setMaxIterations(%d);\n",heuristic,maxIterations_);
  else
    fprintf(fp,"4  %s.setMaxIterations(%d);\n",heuristic,maxIterations_);
  if (maxTime_!=60)
    fprintf(fp,"3  %s.setMaxTime(%.2f);\n",heuristic,maxTime_);
  else
    fprintf(fp,"4  %s.setMaxTime(%.2f);\n",heuristic,maxTime_);
}

// Copy constructor 
CbcHeuristicDive::CbcHeuristicDive(const CbcHeuristicDive & rhs)
:
  CbcHeuristic(rhs),
  matrix_(rhs.matrix_),
  matrixByRow_(rhs.matrixByRow_),
  percentageToFix_(rhs.percentageToFix_),
  maxIterations_(rhs.maxIterations_),
  maxTime_(rhs.maxTime_)
{
  if (rhs.downLocks_) {
    int numberIntegers = model_->numberIntegers();
    downLocks_ = CoinCopyOfArray(rhs.downLocks_,numberIntegers);
    upLocks_ = CoinCopyOfArray(rhs.upLocks_,numberIntegers);
  } else {
    downLocks_ = NULL;
    upLocks_ = NULL;
  }
}

// Assignment operator 
CbcHeuristicDive & 
CbcHeuristicDive::operator=( const CbcHeuristicDive& rhs)
{
  if (this!=&rhs) {
    CbcHeuristic::operator=(rhs);
    matrix_ = rhs.matrix_;
    matrixByRow_ = rhs.matrixByRow_;
    percentageToFix_ = rhs.percentageToFix_;
    maxIterations_ = rhs.maxIterations_;
    maxTime_ = rhs.maxTime_;
    delete [] downLocks_;
    delete [] upLocks_;
    if (rhs.downLocks_) {
      int numberIntegers = model_->numberIntegers();
      downLocks_ = CoinCopyOfArray(rhs.downLocks_,numberIntegers);
      upLocks_ = CoinCopyOfArray(rhs.upLocks_,numberIntegers);
    } else {
      downLocks_ = NULL;
      upLocks_ = NULL;
    }
  }
  return *this;
}

// Resets stuff if model changes
void 
CbcHeuristicDive::resetModel(CbcModel * model)
{
  model_=model;
  assert(model_->solver());
  // Get a copy of original matrix
  const CoinPackedMatrix * matrix = model_->solver()->getMatrixByCol();
  // model may have empty matrix - wait until setModel
  if (matrix) {
    matrix_ = *matrix;
    matrixByRow_ = *model->solver()->getMatrixByRow();
    validate();
  }
}

// update model
void CbcHeuristicDive::setModel(CbcModel * model)
{
  model_ = model;
  assert(model_->solver());
  // Get a copy of original matrix
  const CoinPackedMatrix * matrix = model_->solver()->getMatrixByCol();
  if (matrix) {
    matrix_ = *matrix;
    matrixByRow_ = *model->solver()->getMatrixByRow();
    // make sure model okay for heuristic
    validate();
  }
}

bool CbcHeuristicDive::canHeuristicRun()
{
  return shouldHeurRun_randomChoice();
}

struct PseudoReducedCost {
  int var;
  double pseudoRedCost;
};

inline bool compareBinaryVars(const PseudoReducedCost obj1,
			      const PseudoReducedCost obj2)
{
  return obj1.pseudoRedCost > obj2.pseudoRedCost;
}

// See if dive fractional will give better solution
// Sets value of solution
// Returns 1 if solution, 0 if not
int
CbcHeuristicDive::solution(double & solutionValue,
			   double * betterSolution)
{
  ++numCouldRun_;

  // test if the heuristic can run
  if(!canHeuristicRun())
    return 0;

#if 0
  // See if to do
  if (!when()||(when()%10==1&&model_->phase()!=1)||
      (when()%10==2&&(model_->phase()!=2&&model_->phase()!=3)))
    return 0; // switched off
#endif

  double time1 = CoinCpuTime();

  OsiSolverInterface * solver = model_->solver()->clone();
  const double * lower = solver->getColLower();
  const double * upper = solver->getColUpper();
  const double * rowLower = solver->getRowLower();
  const double * rowUpper = solver->getRowUpper();
  const double * solution = solver->getColSolution();
  const double * objective = solver->getObjCoefficients();
  double integerTolerance = model_->getDblParam(CbcModel::CbcIntegerTolerance);
  double primalTolerance;
  solver->getDblParam(OsiPrimalTolerance,primalTolerance);

  int numberRows = matrix_.getNumRows();
  assert (numberRows<=solver->getNumRows());
  int numberIntegers = model_->numberIntegers();
  const int * integerVariable = model_->integerVariable();
  double direction = solver->getObjSense(); // 1 for min, -1 for max
  double newSolutionValue = direction*solver->getObjValue();
  int returnCode = 0;
  // Column copy
  const double * element = matrix_.getElements();
  const int * row = matrix_.getIndices();
  const CoinBigIndex * columnStart = matrix_.getVectorStarts();
  const int * columnLength = matrix_.getVectorLengths();
  // Row copy
  const double * elementByRow = matrixByRow_.getElements();
  const int * column = matrixByRow_.getIndices();
  const CoinBigIndex * rowStart = matrixByRow_.getVectorStarts();
  const int * rowLength = matrixByRow_.getVectorLengths();

  // Get solution array for heuristic solution
  int numberColumns = solver->getNumCols();
  double * newSolution = new double [numberColumns];
  memcpy(newSolution,solution,numberColumns*sizeof(double));

  // vectors to store the latest variables fixed at their bounds
  int* columnFixed = new int [numberIntegers];
  double* originalBound = new double [numberIntegers];
  bool * fixedAtLowerBound = new bool [numberIntegers];
  PseudoReducedCost * candidate = NULL;
  if(binVarIndex_.size())
    candidate = new PseudoReducedCost [binVarIndex_.size()];

  const int maxNumberAtBoundToFix = (int) floor(percentageToFix_ * numberIntegers);

  // count how many fractional variables
  int numberFractionalVariables = 0;
  for (int i=0; i<numberIntegers; i++) {
    int iColumn = integerVariable[i];
    double value=newSolution[iColumn];
    if (fabs(floor(value+0.5)-value)>integerTolerance) {
      numberFractionalVariables++;
    }
  }

  const double* reducedCost = solver->getReducedCost();

  int iteration = 0;
  while(numberFractionalVariables) {
    iteration++;

    // select a fractional variable to bound
    int bestColumn;
    int bestRound; // -1 rounds down, +1 rounds up
    selectVariableToBranch(solver, newSolution, bestColumn, bestRound);

    bool canRoundSolution = true;
    if(bestColumn != -1)
      canRoundSolution = false;
      
    // fix binary variables based on pseudo reduced cost
    int numberAtBoundFixed = 0;
#if 0
    // This version uses generalized upper bounds. It doesn't seem to be working.
    if(binVarIndex_.size()) {
      int cnt = 0;
      for (int j=0; j<(int)binVarIndex_.size(); j++) {
	int iColumn1 = binVarIndex_[j];
	double value = newSolution[iColumn1];
	double maxPseudoReducedCost = 0.0;
	if(fabs(value)<=integerTolerance &&
	   lower[iColumn1] != upper[iColumn1]) {
	  //	  std::cout<<"iColumn1 = "<<iColumn1<<", value = "<<value<<std::endl;
	  int iRow = vbRowIndex_[j];
	  for (int k=rowStart[iRow];k<rowStart[iRow]+rowLength[iRow];k++) {
	    int iColumn2 = column[k];
	    //	    std::cout<<"iColumn2 = "<<iColumn2<<std::endl;
	    if(iColumn1 != iColumn2) {
	      double pseudoReducedCost = fabs(reducedCost[iColumn2] *
					      elementByRow[iColumn2] / 
					      elementByRow[iColumn1]);
	      //	      std::cout<<"reducedCost["<<iColumn2<<"] = "
	      //		       <<reducedCost[iColumn2]
	      //		       <<", elementByRow["<<iColumn2<<"] = "<<elementByRow[iColumn2]
	      //		       <<", elementByRow["<<iColumn1<<"] = "<<elementByRow[iColumn1]
	      //		       <<", pseudoRedCost = "<<pseudoReducedCost
	      //		       <<std::endl;
	      if(pseudoReducedCost > maxPseudoReducedCost)
		maxPseudoReducedCost = pseudoReducedCost;
	    }
	  }
	  //	  std::cout<<", maxPseudoRedCost = "<<maxPseudoReducedCost<<std::endl;
	  candidate[cnt].var = iColumn1;
	  candidate[cnt++].pseudoRedCost = maxPseudoReducedCost;
	}
      }
      //      std::cout<<"candidates for rounding = "<<cnt<<std::endl;
      std::sort(candidate, candidate+cnt, compareBinaryVars);
      for (int i=0; i<cnt; i++) {
	int iColumn = candidate[i].var;
	if (numberAtBoundFixed < maxNumberAtBoundToFix) {
	  columnFixed[numberAtBoundFixed] = iColumn;
	  originalBound[numberAtBoundFixed] = upper[iColumn];
	  fixedAtLowerBound[numberAtBoundFixed] = true;
	  solver->setColUpper(iColumn, lower[iColumn]);
	  numberAtBoundFixed++;
	  if(numberAtBoundFixed == maxNumberAtBoundToFix)
	    break;
	}
      }
    }
#else
// THIS ONLY USES variable upper bound constraints with 1 continuous variable
    if(binVarIndex_.size()) {
      int cnt = 0;
      for (int j=0; j<(int)binVarIndex_.size(); j++) {
	int iColumn = binVarIndex_[j];
	double value = newSolution[iColumn];
	double pseudoReducedCost = 0.0;
	if(fabs(value)<=integerTolerance &&
	   lower[iColumn] != upper[iColumn]) {
	  //	  std::cout<<"iColumn = "<<iColumn<<", value = "<<value<<std::endl;
	  int iRow = vbRowIndex_[j];
	  assert(rowLength[iRow]==2);
	  int k=rowStart[iRow];
	  if(iColumn == column[k]) {
	    pseudoReducedCost = fabs(reducedCost[column[k+1]] *
				     elementByRow[k+1] / 
				     elementByRow[k]);
	    //	    std::cout<<"reducedCost["<<column[k+1]<<"] = "
	    //		     <<reducedCost[column[k+1]]
	    //		     <<", elementByRow["<<k+1<<"] = "<<elementByRow[k+1]
	    //		     <<", elementByRow["<<k<<"] = "<<elementByRow[k];
	  }
	  else {
	    pseudoReducedCost = fabs(reducedCost[column[k]] *
				     elementByRow[k] / 
				     elementByRow[k+1]);
	    //	    std::cout<<"reducedCost["<<column[k]<<"] = "
	    //		     <<reducedCost[column[k]]
	    //		     <<", elementByRow["<<k<<"] = "<<elementByRow[k]
	    //		     <<", elementByRow["<<k+1<<"] = "<<elementByRow[k+1];
	  }
	  //	  std::cout<<", pseudoRedCost = "<<pseudoReducedCost<<std::endl;
	  candidate[cnt].var = iColumn;
	  candidate[cnt++].pseudoRedCost = pseudoReducedCost;
	}
      }
      //      std::cout<<"candidates for rounding = "<<cnt<<std::endl;
      std::sort(candidate, candidate+cnt, compareBinaryVars);
      for (int i=0; i<cnt; i++) {
	int iColumn = candidate[i].var;
	if (numberAtBoundFixed < maxNumberAtBoundToFix) {
	  columnFixed[numberAtBoundFixed] = iColumn;
	  originalBound[numberAtBoundFixed] = upper[iColumn];
	  fixedAtLowerBound[numberAtBoundFixed] = true;
	  solver->setColUpper(iColumn, lower[iColumn]);
	  numberAtBoundFixed++;
	  if(numberAtBoundFixed == maxNumberAtBoundToFix)
	    break;
	}
      }
    }
#endif
    //    std::cout<<"numberAtBoundFixed = "<<numberAtBoundFixed<<std::endl;

    // fix other integer variables that are at there bounds
    for (int i=0; i<numberIntegers; i++) {
      int iColumn = integerVariable[i];
      double value=newSolution[iColumn];
      if(fabs(floor(value+0.5)-value)<=integerTolerance && 
	 numberAtBoundFixed < maxNumberAtBoundToFix) {
	// fix the variable at one of its bounds
	if (fabs(lower[iColumn]-value)<=integerTolerance &&
	    lower[iColumn] != upper[iColumn]) {
	  columnFixed[numberAtBoundFixed] = iColumn;
	  originalBound[numberAtBoundFixed] = upper[iColumn];
	  fixedAtLowerBound[numberAtBoundFixed] = true;
	  solver->setColUpper(iColumn, lower[iColumn]);
	  numberAtBoundFixed++;
	}
	else if(fabs(upper[iColumn]-value)<=integerTolerance &&
	    lower[iColumn] != upper[iColumn]) {
	  columnFixed[numberAtBoundFixed] = iColumn;
	  originalBound[numberAtBoundFixed] = lower[iColumn];
	  fixedAtLowerBound[numberAtBoundFixed] = false;
	  solver->setColLower(iColumn, upper[iColumn]);
	  numberAtBoundFixed++;
	}
	if(numberAtBoundFixed == maxNumberAtBoundToFix)
	  break;
      }
    }

    if(canRoundSolution) {
      // Round all the fractional variables
      for (int i=0; i<numberIntegers; i++) {
	int iColumn = integerVariable[i];
	double value=newSolution[iColumn];
	if (fabs(floor(value+0.5)-value)>integerTolerance) {
	  if(downLocks_[i]==0 || upLocks_[i]==0) {
	    if(downLocks_[i]==0 && upLocks_[i]==0) {
	      if(direction * objective[iColumn] >= 0.0)
		newSolution[iColumn] = floor(value);
	      else
		newSolution[iColumn] = ceil(value);
	    }
	    else if(downLocks_[i]==0)
	      newSolution[iColumn] = floor(value);
	    else
	      newSolution[iColumn] = ceil(value);
	  }
	  else
	    break;
	}
      }
      break;
    }


    double originalBoundBestColumn;
    if(bestColumn >= 0) {
      if(bestRound < 0) {
	originalBoundBestColumn = upper[bestColumn];
	solver->setColUpper(bestColumn, floor(newSolution[bestColumn]));
      }
      else {
	originalBoundBestColumn = lower[bestColumn];
	solver->setColLower(bestColumn, ceil(newSolution[bestColumn]));
      }
    } else {
      break;
    }
    int originalBestRound = bestRound;
    while (1) {

      solver->resolve();

      if(!solver->isProvenOptimal()) {
	if(numberAtBoundFixed > 0) {
	  // Remove the bound fix for variables that were at bounds
	  for(int i=0; i<numberAtBoundFixed; i++) {
	    int iColFixed = columnFixed[i];
	    if(fixedAtLowerBound[i])
	      solver->setColUpper(iColFixed, originalBound[i]);
	    else
	      solver->setColLower(iColFixed, originalBound[i]);
	  }
	  numberAtBoundFixed = 0;
	}
	else if(bestRound == originalBestRound) {
	  bestRound *= (-1);
	  if(bestRound < 0) {
	    solver->setColLower(bestColumn, originalBoundBestColumn);
	    solver->setColUpper(bestColumn, floor(newSolution[bestColumn]));
	  }
	  else {
	    solver->setColLower(bestColumn, ceil(newSolution[bestColumn]));
	    solver->setColUpper(bestColumn, originalBoundBestColumn);
	  }
	}
	else
	  break;
      }
      else
	break;
    }

    if(!solver->isProvenOptimal())
      break;

    if(iteration > maxIterations_) {
      break;
    }

    if(CoinCpuTime()-time1 > maxTime_) {
      break;
    }

    memcpy(newSolution,solution,numberColumns*sizeof(double));
    numberFractionalVariables = 0;
    for (int i=0; i<numberIntegers; i++) {
      int iColumn = integerVariable[i];
      double value=newSolution[iColumn];
      if (fabs(floor(value+0.5)-value)>integerTolerance) {
	numberFractionalVariables++;
      }
    }

  }


  double * rowActivity = new double[numberRows];
  memset(rowActivity,0,numberRows*sizeof(double));

  // re-compute new solution value
  double objOffset=0.0;
  solver->getDblParam(OsiObjOffset,objOffset);
  newSolutionValue = -objOffset;
  for (int i=0 ; i<numberColumns ; i++ )
    newSolutionValue += objective[i]*newSolution[i];
  newSolutionValue *= direction;
    //printf("new solution value %g %g\n",newSolutionValue,solutionValue);
  if (newSolutionValue<solutionValue) {
    // paranoid check
    memset(rowActivity,0,numberRows*sizeof(double));
    for (int i=0;i<numberColumns;i++) {
      int j;
      double value = newSolution[i];
      if (value) {
	for (j=columnStart[i];
	     j<columnStart[i]+columnLength[i];j++) {
	  int iRow=row[j];
	  rowActivity[iRow] += value*element[j];
	}
      }
    }
    // check was approximately feasible
    bool feasible=true;
    for (int i=0;i<numberRows;i++) {
      if(rowActivity[i]<rowLower[i]) {
	if (rowActivity[i]<rowLower[i]-1000.0*primalTolerance)
	  feasible = false;
      } else if(rowActivity[i]>rowUpper[i]) {
	if (rowActivity[i]>rowUpper[i]+1000.0*primalTolerance)
	  feasible = false;
      }
    }
    for (int i=0; i<numberIntegers; i++) {
      int iColumn = integerVariable[i];
      double value=newSolution[iColumn];
      if (fabs(floor(value+0.5)-value)>integerTolerance) {
	feasible = false;
	break;
      }
    }
    if (feasible) {
      // new solution
      memcpy(betterSolution,newSolution,numberColumns*sizeof(double));
      solutionValue = newSolutionValue;
      //printf("** Solution of %g found by CbcHeuristicDive\n",newSolutionValue);
      returnCode=1;
    } else {
      // Can easily happen
      //printf("Debug CbcHeuristicDive giving bad solution\n");
    }
  }

  delete [] newSolution;
  delete [] columnFixed;
  delete [] originalBound;
  delete [] fixedAtLowerBound;
  delete [] candidate;
  delete [] rowActivity;
  delete solver;
  return returnCode;
}

// Validate model i.e. sets when_ to 0 if necessary (may be NULL)
void 
CbcHeuristicDive::validate() 
{
  if (model_&&when()<10) {
    if (model_->numberIntegers()!=
        model_->numberObjects())
      setWhen(0);
  }

  int numberIntegers = model_->numberIntegers();
  const int * integerVariable = model_->integerVariable();
  delete [] downLocks_;
  delete [] upLocks_;
  downLocks_ = new unsigned short [numberIntegers];
  upLocks_ = new unsigned short [numberIntegers];
  // Column copy
  const double * element = matrix_.getElements();
  const int * row = matrix_.getIndices();
  const CoinBigIndex * columnStart = matrix_.getVectorStarts();
  const int * columnLength = matrix_.getVectorLengths();
  const double * rowLower = model_->solver()->getRowLower();
  const double * rowUpper = model_->solver()->getRowUpper();
  for (int i=0;i<numberIntegers;i++) {
    int iColumn = integerVariable[i];
    int down=0;
    int up=0;
    if (columnLength[iColumn]>65535) {
      setWhen(0);
      break; // unlikely to work
    }
    for (CoinBigIndex j=columnStart[iColumn];
	 j<columnStart[iColumn]+columnLength[iColumn];j++) {
      int iRow=row[j];
      if (rowLower[iRow]>-1.0e20&&rowUpper[iRow]<1.0e20) {
	up++;
	down++;
      } else if (element[j]>0.0) {
	if (rowUpper[iRow]<1.0e20)
	  up++;
	else
	  down++;
      } else {
	if (rowLower[iRow]>-1.0e20)
	  up++;
	else
	  down++;
      }
    }
    downLocks_[i] = (unsigned short) down;
    upLocks_[i] = (unsigned short) up;
  }

#ifdef DIVE_FIX_BINARY_VARIABLES
  selectBinaryVariables();
#endif
}

#if 0
// This version uses generalized upper bounds. It doesn't seem to be working.

// Select candidate binary variables for fixing
void
CbcHeuristicDive::selectBinaryVariables()
{
  // Row copy
  const double * elementByRow = matrixByRow_.getElements();
  const int * column = matrixByRow_.getIndices();
  const CoinBigIndex * rowStart = matrixByRow_.getVectorStarts();
  const int * rowLength = matrixByRow_.getVectorLengths();

  const int numberRows = matrixByRow_.getNumRows();
  const int numberCols = matrixByRow_.getNumCols();

  const double * lower = model_->solver()->getColLower();
  const double * upper = model_->solver()->getColUpper();
  const double * rowLower = model_->solver()->getRowLower();
  const double * rowUpper = model_->solver()->getRowUpper();

  //  const char * integerType = model_->integerType();
  

  //  const int numberIntegers = model_->numberIntegers();
  //  const int * integerVariable = model_->integerVariable();
  const double * objective = model_->solver()->getObjCoefficients();

  // vector to store the row number of variable bound rows
  int* rowIndexes = new int [numberCols];
  memset(rowIndexes, -1, numberCols*sizeof(int));

  for(int i=0; i<numberRows; i++) {
    int binVar = -1;
    int numIntegers = 0;
    int numContinuous = 0;
    for (int k=rowStart[i];k<rowStart[i]+rowLength[i];k++) {
      int iColumn = column[k];
      if(model_->solver()->isInteger(iColumn)) {
	numIntegers++;
	if(numIntegers > 1)
	  break;
	if(lower[iColumn] == 0.0 && upper[iColumn] == 1.0 &&
	   objective[iColumn] == 0.0)
	  binVar = iColumn;
      }
      else
	numContinuous++;
    }
    if(numIntegers == 1 && binVar >= 0 && numContinuous > 0 &&
       ((rowLower[i] == 0.0 && rowUpper[i] > 1.0e30) ||
	(rowLower[i] < -1.0e30 && rowUpper[i] == 0))) {
      if(rowIndexes[binVar] == -1)
	rowIndexes[binVar] = i;
      else if(rowIndexes[binVar] >= 0)
	rowIndexes[binVar] = -2;
    }
  }

  for(int j=0; j<numberCols; j++) {
    if(rowIndexes[j] >= 0) {
      binVarIndex_.push_back(j);
      vbRowIndex_.push_back(rowIndexes[j]);
    }
  }

  std::cout<<"number vub Binary = "<<binVarIndex_.size()<<std::endl;

  delete [] rowIndexes;
    
}

#else
// THIS ONLY USES variable upper bound constraints with 1 continuous variable

// Select candidate binary variables for fixing
void
CbcHeuristicDive::selectBinaryVariables()
{
  // Row copy
  const double * elementByRow = matrixByRow_.getElements();
  const int * column = matrixByRow_.getIndices();
  const CoinBigIndex * rowStart = matrixByRow_.getVectorStarts();
  const int * rowLength = matrixByRow_.getVectorLengths();

  const int numberRows = matrixByRow_.getNumRows();
  const int numberCols = matrixByRow_.getNumCols();

  const double * lower = model_->solver()->getColLower();
  const double * upper = model_->solver()->getColUpper();
  const double * rowLower = model_->solver()->getRowLower();
  const double * rowUpper = model_->solver()->getRowUpper();

  //  const char * integerType = model_->integerType();
  

  //  const int numberIntegers = model_->numberIntegers();
  //  const int * integerVariable = model_->integerVariable();
  const double * objective = model_->solver()->getObjCoefficients();

  // vector to store the row number of variable bound rows
  int* rowIndexes = new int [numberCols];
  memset(rowIndexes, -1, numberCols*sizeof(int));

  for(int i=0; i<numberRows; i++) {
    int binVar = -1;
    int numContinuous = 0;
    if(rowLength[i] == 2) {
      int k = rowStart[i];
      int iColumn1 = column[k++];
      int iColumn2 = column[k];
      if(model_->solver()->isInteger(iColumn1) &&
	 lower[iColumn1] == 0.0 && upper[iColumn1] == 1.0 &&
	 objective[iColumn1] == 0.0 &&
	 model_->solver()->isContinuous(iColumn2))
	binVar = iColumn1;
      else if(model_->solver()->isInteger(iColumn2) &&
	      lower[iColumn2] == 0.0 && upper[iColumn2] == 1.0 &&
	      objective[iColumn2] == 0.0 &&
	      model_->solver()->isContinuous(iColumn1))
	binVar = iColumn2;
    }
    if(binVar >= 0 &&
       ((rowLower[i] == 0.0 && rowUpper[i] > 1.0e30) ||
	(rowLower[i] < -1.0e30 && rowUpper[i] == 0))) {
      if(rowIndexes[binVar] == -1)
	rowIndexes[binVar] = i;
      else if(rowIndexes[binVar] >= 0)
	rowIndexes[binVar] = -2;
    }
  }

  for(int j=0; j<numberCols; j++) {
    if(rowIndexes[j] >= 0) {
      binVarIndex_.push_back(j);
      vbRowIndex_.push_back(rowIndexes[j]);
    }
  }

  std::cout<<"number vub Binary = "<<binVarIndex_.size()<<std::endl;

  delete [] rowIndexes;
    
}
#endif
