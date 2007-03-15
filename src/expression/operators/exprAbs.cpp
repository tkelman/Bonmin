/*
 * Name:    exprAbs.cpp
 * Author:  Pietro Belotti
 * Purpose: definition of the absulute value of a function
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#include <exprAbs.h>
#include <exprClone.h>
#include <exprMin.h>
#include <exprMax.h>
#include <exprOpp.h>


/// find lower and upper bound of a given expression

void exprAbs::getBounds (expression *&lb, expression *&ub) {

  // get bounds of the argument
  expression *lba, *uba;

  argument_ -> getBounds (lba, uba);

  // lower bound = max (0, lb, -ub)

  expression **all = new expression * [6];
  all [0] = new exprConst (0);   all [1] = new exprConst (0);
  all [2] = new exprOpp (uba);   all [3] = new exprOpp (new exprClone (uba));
  all [4] = lba;                 all [5] = new exprClone (lba);

  lb = new exprMax (all, 6);

  // upper bound = max (|lb|, |ub|)

  ub = new exprMax (new exprAbs (new exprClone (lba)), 
		    new exprAbs (new exprClone (uba)));
}


/// differentiation

expression *exprAbs::differentiate (int index) {

  expression **arglist = new expression * [4];
  expression  *diffarg = argument_ -> differentiate (index);

  arglist [0] = new exprConst (0);
  arglist [1] = new exprClone (diffarg);
  arglist [2] = new exprOpp (new exprClone (argument_));
  arglist [3] = new exprOpp (diffarg);

  return new exprMin (arglist, 4);
}


/// printing

void exprAbs::print (std::ostream& out) const {
  exprUnary::print (out, "abs", PRE);
  /*
  out << "|";
  argument_ -> print (out);
  out << "|";
  */
}


/// implied bound processing for expression w = |x|, upon change in
/// lower- and/or upper bound of w, whose index is wind

bool exprAbs::impliedBound (int wind, CouNumber *l, CouNumber *u, char *chg) {

  int index = argument_ -> Index ();

  CouNumber *xl = l + index, wl = l [wind],
            *xu = u + index, wu = u [wind];

  // for w >= b, we can only improve xlb if it is at least -b
  //                                 xub             most   b

  bool tighter = false;

  if (wl >= 0) {

    tighter = updateBound (-1, xl, -wl);
    tighter = updateBound (+1, xu,  wl) || tighter;
  }

  // w <= u. If u the problem is infeasible, but we only fix x to zero
  // since we cannot return infeasibility here

  if (wu <= -0.) 
    wu = 0.;

  tighter = updateBound (-1, xl, -wu) || tighter;
  tighter = updateBound (+1, xu,  wu) || tighter;

  if (tighter)
    chg [index] = 1;

  return tighter;
}
