/*
 * Name:    boundTightening.cpp
 * Author:  Pietro Belotti
 * Purpose: tighten bounds prior to convexification cuts
 *
 * (C) Pietro Belotti, all rights reserved. 
 * This file is licensed under the Common Public License.
 */

#include <CglCutGenerator.hpp>
#include <CouenneCutGenerator.hpp>
#include <CouenneProblem.hpp>
#include "BonAuxInfos.hpp"

// max # bound tightening iterations
#define MAX_BT_ITER 10


/// procedure to strengthen variable bounds. Return false if problem
/// turns out to be infeasible with given bounds, true otherwise.

bool CouenneCutGenerator::boundTightening (const OsiSolverInterface *psi,
					   OsiCuts &cs, 
					   t_chg_bounds *chg_bds, 
					   Bonmin::BabInfo * babInfo) const {

  int objInd = problem_ -> Obj (0) -> Body () -> Index ();

  /////////////////////// MIP bound management /////////////////////////////////

  if ((psi) && (objInd >= 0) && babInfo && (babInfo -> babPtr ())) {

    CouNumber UB      = babInfo  -> babPtr () -> model (). getObjValue(),
              LB      = babInfo  -> babPtr () -> model (). getBestPossibleObjValue (),
              primal0 = problem_ -> Ub (objInd), 
              dual0   = problem_ -> Lb (objInd);

    // Bonmin assumes minimization. Hence, primal (dual) is an UPPER
    // (LOWER) bound.
    
    if ((UB < COUENNE_INFINITY) && 
	(UB < primal0 - COUENNE_EPS)) { // update primal bound (MIP)

      //      printf ("updating upper %g <-- %g\n", primal0, primal);
      problem_ -> Ub (objInd) = UB;
      chg_bds [objInd]. upper = CHANGED;
    }
    
    if ((LB   > - COUENNE_INFINITY) && 
	(LB   > dual0 + COUENNE_EPS)) { // update dual bound
      problem_ -> Lb (objInd) = LB;
      chg_bds [objInd]. lower = CHANGED;
    }

    //////////////////////// Reduced cost bound tightening //////////////////////

    // do it only if a linear convexification is in place already

    if (!firstcall_) {
      /*
    CouNumber 
      LB = si.getObjValue (), 
      UB = (babInfo && (babInfo -> babPtr ())) ? 
              babInfo  -> babPtr () -> model (). getObjValue() : 
              problem_ -> Ub (objInd);
      */

      if ((LB > -COUENNE_INFINITY) && (UB < COUENNE_INFINITY)) {
	int ncols = psi ->getNumCols ();

	for (int i=0; i<ncols; i++) {

	  CouNumber 
	    x  = psi -> getColSolution () [i],
	    rc = psi -> getReducedCost () [i],
	    dx = problem_ -> Ub (i) - x;

	  if ((rc > COUENNE_EPS) && (dx*rc > (UB-LB))) {
	    // can improve bound on variable w_i
	    problem_ -> Ub (i) = x + (UB-LB) / rc;
	    chg_bds [i].upper = CHANGED;
	  }
	}
      }
    }
  }

  //////////////////////// Bound propagation and implied bounds ////////////////////

  int   ntightened = 0,
      nbwtightened = 0,
      niter = 0;

  bool first = true;

  do {

    // propagate bounds to auxiliary variables

    //    if ((nbwtightened > 0) || (ntightened > 0))
    ntightened = ((nbwtightened > 0) || first) ? problem_ -> tightenBounds (chg_bds) : 0;

    //    printf ("#### propagate ---> %d\n", ntightened);

    // implied bounds. Call also at the beginning, as some common
    // expression may have non-propagated bounds

    // if last call didn't signal infeasibility
    nbwtightened = ((ntightened > 0) || first) ? problem_ -> impliedBounds (chg_bds) : 0;

    //    printf ("#### implied ---> %d\n", nbwtightened);

    if (first)
      first = false;

    if ((ntightened < 0) || (nbwtightened < 0)) {

      // set infeasibility through a cut 1 <= x0 <= -1

      if (!firstcall_) {

	OsiColCut *infeascut = new OsiColCut;

	if (infeascut) {
	  int i=0;
	  double upper = -1., lower = +1.;
	  infeascut -> setLbs (1, &i, &lower);
	  infeascut -> setUbs (1, &i, &upper);
	  cs.insert (infeascut);
	  delete infeascut;
	}
      }

      if (babInfo) // set infeasibility to true in order to skip NLP heuristic
	babInfo -> setInfeasibleNode ();

      return false;
    }

  } while (((ntightened > 0) || (nbwtightened > 0)) && (niter++ < MAX_BT_ITER));
  // continue if EITHER procedures gave (positive) results, as
  // expression structure is not a tree.

  // TODO: limit should depend on number of constraints, that is,
  // bound transmission should be documented and the cycle should stop
  // as soon as no new constraint subgraph has benefited from bound
  // transmission.

  return true;
}