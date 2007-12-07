/*
 * Name:    exprSub.cpp
 * Author:  Pietro Belotti
 * Purpose: convexification methods for the Subtraction class
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include "CouenneTypes.hpp"
#include "CouenneCutGenerator.hpp"
#include "exprSub.hpp"
#include "exprOpp.hpp"

// generate equality between *this and *w
void exprSub::generateCuts (exprAux *w, const OsiSolverInterface &si, 
			    OsiCuts &cs, const CouenneCutGenerator *cg,
			    t_chg_bounds *chg, 
			    int wind, CouNumber lb, CouNumber ub) {

  if (!(cg -> isFirst ()))
    return;

  // only add one cut at the beginning

  expression *x = arglist_ [0];
  expression *y = arglist_ [1];

  int wi = w -> Index ();
  int xi = x -> Index ();
  int yi = y -> Index ();

  if (wind >= 0) wi = -1; // do not insert w's index if specified as input
  else lb = ub = 0;

  if (xi < 0) {
    CouNumber x0 = x -> Value ();
    lb -= x0;
    ub -= x0;
  }

  if (yi < 0) {
    CouNumber y0 = y -> Value ();
    lb += y0;
    ub += y0;
  }

  cg -> createCut (cs, lb, ub, wi, -1., xi, 1., yi, -1., true);
}