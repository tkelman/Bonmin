/*
 * Name:    CouenneChooseStrong.cpp
 * Authors: Andreas Waechter, IBM Corp.
 * Purpose: Strong branching objects for Couenne
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include "CouenneChooseStrong.hpp"
#include "CoinTime.hpp"
#include "CouenneProblem.hpp"

namespace Bonmin {

  CouenneChooseStrong::CouenneChooseStrong(BabSetupBase &b, CouenneProblem* p) :
    BonChooseVariable(b, b.continuousSolver()),
    problem_(p)
  {}


  CouenneChooseStrong::CouenneChooseStrong(const CouenneChooseStrong& rhs) :
    BonChooseVariable(rhs),
    problem_(rhs.problem_)
  {}

  CouenneChooseStrong::~CouenneChooseStrong()
  {}

  OsiChooseVariable *
  CouenneChooseStrong::clone() const
  {
    return new CouenneChooseStrong(*this);
  }

  CouenneChooseStrong&
  CouenneChooseStrong::operator=(const CouenneChooseStrong & rhs)
  {
    if (this != &rhs) {
      BonChooseVariable::operator=(rhs);
      problem_ = rhs.problem_;
    }
    return *this;
  }

  int
  CouenneChooseStrong::doStrongBranching( OsiSolverInterface * solver, 
					  OsiBranchingInformation *info,
					  int numberToDo, int returnCriterion)
  {

    // update problem with point/bounds contained in info
    problem_ -> domain () -> push 
      (problem_ -> nVars (),
       info -> solution_, 
       info -> lower_, 
       info -> upper_);

    // Might be faster to extend branch() to return bounds changed
    double * saveLower = NULL;
    double * saveUpper = NULL;
    int numberColumns = solver->getNumCols();
    solver->markHotStart();
    const double * lower = info->lower_;
    const double * upper = info->upper_;
    saveLower = CoinCopyOfArray(info->lower_,numberColumns);
    saveUpper = CoinCopyOfArray(info->upper_,numberColumns);
    int returnCode=0;
    double timeStart = CoinCpuTime();
    int iDo = 0;
    for (;iDo<numberToDo;iDo++) {
      HotInfo * result = results_() + iDo;
      // For now just 2 way
      OsiBranchingObject * branch = result->branchingObject();
      assert (branch->numberBranches()==2);
      /*
        Try the first direction.  Each subsequent call to branch() performs the
        specified branch and advances the branch object state to the next branch
        alternative.)
      */

      int 
	status0 = -1, 
	status1 = -1;

      OsiSolverInterface * thisSolver = solver; 
      if (branch->boundBranch()) {
        // ordinary
        if (branch->branch(solver) > COUENNE_INFINITY)
	  status0 = 1;
        // maybe we should check bounds for stupidities here?
        solver->solveFromHotStart() ;
      } else {
        // adding cuts or something 
        thisSolver = solver->clone();
        if (branch->branch(thisSolver) > COUENNE_INFINITY)
	  status0 = 1;
	else {
	  // set hot start iterations
	  int limit;
	  thisSolver->getIntParam(OsiMaxNumIterationHotStart,limit);
	  thisSolver->setIntParam(OsiMaxNumIteration,limit); 
	  thisSolver->resolve();
	}
      }
      // can check if we got solution
      // status is 0 finished, 1 infeasible and 2 unfinished and 3 is solution

      if (status0 < 0) {
	status0 = result->updateInformation(thisSolver,info,this);
	numberStrongIterations_ += thisSolver->getIterationCount();
      }

      if (status0==3) {
        // new solution already saved
        if (trustStrongForSolution_) {
  	info->cutoff_ = goodObjectiveValue_;
  	status0=0;
        }
      }
      if (solver!=thisSolver)
        delete thisSolver;
      // Restore bounds
      for (int j=0;j<numberColumns;j++) {
        if (saveLower[j] != lower[j])
  	solver->setColLower(j,saveLower[j]);
        if (saveUpper[j] != upper[j])
  	solver->setColUpper(j,saveUpper[j]);
      }
      /*
        Try the next direction
      */
      thisSolver = solver; 
      if (branch->boundBranch()) {
        // ordinary
        if (branch->branch(solver) > COUENNE_INFINITY)
	  status1 = 1;
        else solver->solveFromHotStart();  // maybe we should check bounds for stupidities here?
      } else {
        // adding cuts or something 
        thisSolver = solver->clone();
        if (branch->branch(thisSolver) > COUENNE_INFINITY)
	  status1 = 1;
	else {
        // set hot start iterations
	  int limit;
	  thisSolver->getIntParam(OsiMaxNumIterationHotStart,limit);
	  thisSolver->setIntParam(OsiMaxNumIteration,limit); 
	  thisSolver->resolve();
	}
      }
      // can check if we got solution
      // status is 0 finished, 1 infeasible and 2 unfinished and 3 is solution

      if (status1 < 0) {
	status1 = result->updateInformation(thisSolver,info,this);
	numberStrongDone_++;
      }

      //printf ("statuses = %d,%d\n", status0, status1);

      numberStrongIterations_ += thisSolver->getIterationCount();
      if (status1==3) {
        // new solution already saved
        if (trustStrongForSolution_) {
  	info->cutoff_ = goodObjectiveValue_;
  	status1=0;
        }
      }
      if (solver!=thisSolver)
        delete thisSolver;
      // Restore bounds
      for (int j=0;j<numberColumns;j++) {
        if (saveLower[j] != lower[j])
  	solver->setColLower(j,saveLower[j]);
        if (saveUpper[j] != upper[j])
  	solver->setColUpper(j,saveUpper[j]);
      }
      /*
        End of evaluation for this candidate object. Possibilities are:
        * Both sides below cutoff; this variable is a candidate for branching.
        * Both sides infeasible or above the objective cutoff: no further action
        here. Break from the evaluation loop and assume the node will be purged
        by the caller.
        * One side below cutoff: Install the branch (i.e., fix the variable). Possibly break
        from the evaluation loop and assume the node will be reoptimised by the
        caller.
      */
      if (status0==1&&status1==1) {
        // infeasible
        returnCode=-1;
        break; // exit loop
      } else if (status0==1||status1==1) {
        numberStrongFixed_++;
        if (!returnCriterion) {
  	returnCode=1;
        } else {
  	returnCode=2;
  	break;
        }
      }
      bool hitMaxTime = ( CoinCpuTime()-timeStart > info->timeRemaining_);
      if (hitMaxTime) {
        returnCode=3;
        break;
      }
    }
    if(iDo < numberToDo) iDo++;
    assert(iDo <= (int) results_.size());
    results_.resize(iDo);
    delete [] saveLower;
    delete [] saveUpper;
    // Delete the snapshot
    solver->unmarkHotStart();

    // discard current point/bounds from problem
    problem_ -> domain () -> pop ();

    //printf ("retcode = %d\n", returnCode);

    return returnCode;
  }
}