/*
 * Name:    exprExp.cpp
 * Author:  Pietro Belotti
 * Purpose: definition of the exponential
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include <exprExp.hpp>
#include <exprClone.hpp>
#include <exprMul.hpp>


// differentiation
expression *exprExp::differentiate (int index) {

  return new exprMul (new exprExp (new exprClone (argument_)),
		      argument_ -> differentiate (index));
}


// Get lower and upper bound of an expression (if any)
void exprExp::getBounds (expression *&lb, expression *&ub) {

  expression *lba, *uba;
  argument_ -> getBounds (lba, uba);

  lb = new exprExp (lba);
  ub = new exprExp (uba);
}


/// implied bound processing for expression w = exp(x), upon change in
/// lower- and/or upper bound of w, whose index is wind
bool exprExp::impliedBound (int wind, CouNumber *l, CouNumber *u, t_chg_bounds *chg) {

  bool resU, resL = resU = false;
  int ind = argument_ -> Index ();

  CouNumber b;

  if ((b = l [wind]) >= COUENNE_EPS) // lower bound
    resL = updateBound (-1, l + ind, log (b));

  if ((b = u [wind]) >= COUENNE_EPS) // upper bound
    resU = updateBound ( 1, u + ind, log (b));
  else if (b < - COUENNE_EPS) {
    // make it infeasible
    resU = updateBound ( 1, u + ind, -1) || resU;
    resL = updateBound (-1, l + ind,  1) || resL;
  }

  if (resL) chg [ind].lower = CHANGED;
  if (resU) chg [ind].upper = CHANGED;

  return (resL || resU);
}
