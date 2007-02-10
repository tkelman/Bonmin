/*
 * Name:    conv-exprOpp.cpp
 * Author:  Pietro Belotti
 * Purpose: methods to convexify opposite of expressions
 *
 * This file is licensed under the Common Public License (CPL)
 */

#include <CouenneTypes.h>
#include <exprOpp.h>
#include <exprConst.h>

#include <CouenneProblem.h>
#include <CouenneCutGenerator.h>


// generate convexification cut for constraint w = - x

void exprOpp::generateCuts (exprAux *w, const OsiSolverInterface &si, 
			    OsiCuts &cs, const CouenneCutGenerator *cg) {
  // easy... 

  OsiRowCut *cut;

  if (cg -> isFirst () && 
      (cut = cg -> createCut (0., 0, w->Index (), 1., argument_->Index (), 1.)))
    cs.insert (cut);
}
