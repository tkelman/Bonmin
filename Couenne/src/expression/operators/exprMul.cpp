/*
 * Name:    exprMul.cpp
 * Author:  Pietro Belotti
 * Purpose: definition of multiplications
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include <stdlib.h>
#include <assert.h>

#include "CoinHelperFunctions.hpp"
#include "exprMul.hpp"
#include "exprSum.hpp"
#include "exprConst.hpp"
#include "exprClone.hpp"
#include "CouennePrecisions.hpp"


/// Constructors, destructor
exprMul::exprMul  (expression **al, int n): 
  exprOp (al, n) { //< non-leaf expression, with argument list

  // commutative operator, sort elements
  qsort (arglist_, nargs_, sizeof (expression*), compareExpr);
}


/// constructor with only two factors
exprMul::exprMul (expression *arg0, expression *arg1):
  exprOp (arg0, arg1) {

  if (compareExpr (arglist_, arglist_ + 1) > 0) {

    register expression
           *swap = arglist_ [0];
    arglist_ [0] = arglist_ [1];
    arglist_ [1] = swap;
  }
}


/// simplify multiplications
expression *exprMul::simplify () {

  exprOp:: simplify ();

  if (nargs_ == 1) {

    expression *ret = arglist_ [0];
    arglist_ [0] = NULL;
    return ret;
  }

  CouNumber prod = 1;

  bool found_one = false;

  for (register int i=0; i<nargs_; i++) {

    // check for null operands in multiplications

    if (arglist_ [i] -> Type () == CONST) {

      found_one = true;

      CouNumber c = arglist_ [i] -> Value ();
      prod *= c;

      if (fabs (c) < COUENNE_EPS_SIMPL) {

	for (int j=0; j<nargs_; j++)
	  if (arglist_ [j]) {
	    delete arglist_ [j];
	    arglist_ [j] = NULL;
	  }

	return new exprConst (0.);
      }

      // check for nonzero constants in multiplications

      delete arglist_ [i];
      arglist_ [i] = NULL;
    }
  }
  /*
  if (found_one && shrink_arglist (prod, 1))
    return new exprConst (arglist_ [0] -> Value ());
  */
  if (found_one && shrink_arglist (prod, 1)) {
    expression *ret = arglist_ [0];
    arglist_ [0] = NULL;
    return ret;
  }
  else return NULL;
}


// differentiate product of expressions
expression *exprMul::differentiate (int index) {

  expression **als = new expression * [nargs_];
  int nonconst = 0;

  for (int i = 0; i < nargs_; i++) 

    if (arglist_ [i] -> dependsOn (index)) {

      expression **alm = new expression * [nargs_];

      alm [i] = arglist_ [i] -> differentiate (index);

      for (int j = 0; j < nargs_; j++) 
	if (i!=j)
	  alm [j] = new exprClone (arglist_ [j]);

      als [nonconst++] = new exprMul (alm, nargs_);
    }

  if (nonconst) 
    return new exprSum (als, nonconst);
  else {
    delete [] als;
    return new exprConst (0.);
  }
}


// get a measure of "how linear" the expression is:
//
// ZERO      = 0: constant 0
// CONSTANT  = 1: a constant
// LINEAR    = 2: linear
// QUADRATIC = 3: quadratic
// NONLINER  = 4: nonlinear non-quadratic

int exprMul::Linearity () {

  int lin0 = arglist_ [0] -> Linearity ();

  if (lin0 >= NONLINEAR) return NONLINEAR;
  if (lin0 == ZERO)      return ZERO;

  for (register int i=1; i<nargs_; i++) {

    int lin = arglist_ [i] -> Linearity ();

    switch (lin) {
    case NONLINEAR: return NONLINEAR;
    case ZERO:      return ZERO;
    case LINEAR:    lin0++; break;
    case QUADRATIC: lin0 += 2; break;
    default: break;
    }

    if (lin0 >= NONLINEAR) return NONLINEAR;
  }
  return lin0;
}


/// compute $y^{lv}$ and  $y^{uv}$ for Violation Transfer algorithm
void exprMul::closestFeasible (expression *varind,
			       expression *vardep,
			       CouNumber &left,
			       CouNumber &right) const {

  expression *varoth = arglist_ [0]; // suppose $w = cy$;

  if (varoth -> Index () == varind -> Index ())
    varoth = arglist_ [1]; // actually no, it's $w = x*c$

  assert (varoth -> Index () >= 0);

  CouNumber
    x = (*varind) (),
    y = (*vardep) (),
    c = (*varoth) ();

  if (c < 0.)
    if (y < c*x) {assert (y/c >= right); right = y/c;}
    else         {assert (y/c <= left);  left  = y/c;}
  else if (c > 0.)
    if (y < c*x) {assert (y/c <= left);  left  = y/c;}
    else         {assert (y/c >= right); right = y/c;}
  else left = - (right = COIN_DBL_MAX);
}


/// return l-2 norm of gradient at given point
CouNumber exprMul::gradientNorm (const double *x) {

  int 
    ind0 = arglist_ [0] -> Index (),
    ind1 = arglist_ [1] -> Index ();

  CouNumber 
    x0 = (ind0 < 0) ? arglist_ [0] -> Value () : x [ind0],
    x1 = (ind1 < 0) ? arglist_ [1] -> Value () : x [ind1];

  if (ind0 < 0)
    if (ind1 < 0) return 0.;                   // c*d
    else          return fabs (x0);            // c*y
  else 
    if (ind1 < 0) return fabs (x1);            // x*d
    else          return sqrt (x0*x0 + x1*x1); // x*y
}

