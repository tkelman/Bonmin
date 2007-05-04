/*
 * Name:    boundTightening.cpp
 * Author:  Pietro Belotti
 * Purpose: tighten bounds prior to convexification cuts
 *
 * (C) Pietro Belotti, all rights reserved. 
 * This file is licensed under the Common Public License.
 */

#include <CglCutGenerator.hpp>
#include <CouenneCutGenerator.h>
#include <CouenneProblem.h>
#include "BonAuxInfos.hpp"


/// procedure to strengthen variable bounds

bool CouenneCutGenerator::boundTightening (const OsiSolverInterface &si,
					   OsiCuts &cs, 
					   char *chg_bds, 
					   Bonmin::BabInfo * babInfo) const {

  int objInd = problem_ -> Obj (0) -> Body () -> Index ();

  //////////////////////// Reduced cost bound tightening //////////////////////

  // do it only if a linear convexification is in place already

  if (0)
  if ((objInd >= 0) && !firstcall_) {

    CouNumber 
      LB = si.getObjValue (), 
      UB = (babInfo && (babInfo -> babPtr ())) ? 
              babInfo  -> babPtr () -> model (). getObjValue() : 
              problem_ -> Ub (objInd);

    if ((LB > -COUENNE_INFINITY) && (UB < COUENNE_INFINITY)) {
      int ncols = si.getNumCols ();

      for (int i=0; i<ncols; i++) {

	CouNumber 
	  x  = si.getColSolution () [i],
	  rc = si.getReducedCost () [i],
	  dx = problem_ -> Ub (i) - x;

	if ((rc > COUENNE_EPS) && (dx*rc > (UB-LB))) {
	  // can improve bound on variable w_i
	  problem_ -> Ub (i) = x + (UB-LB) / rc;
	  chg_bds [i] = 1;
	}
      }
    }
  }

  /////////////////////// MIP bound management /////////////////////////////////

  if (objInd < 0) objInd = 0; 

  if (babInfo && (babInfo -> babPtr ())) {

    CouNumber primal  = babInfo  -> babPtr () -> model (). getObjValue(),
              dual    = babInfo  -> babPtr () -> model (). getBestPossibleObjValue (),
              primal0 = problem_ -> Ub (objInd), 
              dual0   = problem_ -> Lb (objInd);

    // Bonmin assumes minimization. Hence, primal (dual) is an UPPER
    // (LOWER) bound.

    if ((primal <   COUENNE_INFINITY - 1) && (primal < primal0)) { // update primal bound (MIP)
      problem_ -> Ub (objInd) = primal;
      chg_bds [objInd] = 1;
    }

    if ((dual   > - COUENNE_INFINITY + 1) && (dual   > dual0)) { // update dual bound
      problem_ -> Lb (objInd) = dual;
      chg_bds [objInd] = 1;
    }
  }

  //////////////////////// Bound tightening ///////////////////////////////////

  int   ntightened = 0,
      nbwtightened = 0,
      niter = 0;

  do {

    // propagate bounds to auxiliary variables

    ntightened = problem_ -> tightenBounds (chg_bds);

    // implied bounds. Call also at the beginning, as some common
    // expression may have non-propagated bounds

    if (ntightened >= 0) // if last call didn't signal infeasibility
      nbwtightened = problem_ -> impliedBounds (chg_bds);

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
  } while (ntightened || nbwtightened && (niter++ < 10));
  // need to check if EITHER procedures gave results, as expression
  // structure is not a tree any longer.

  return true;
}
