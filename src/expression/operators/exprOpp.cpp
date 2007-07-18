/*
 * Name:    exprOpp.cpp
 * Author:  Pietro Belotti
 * Purpose: definition of the opposite -f(x) of a function
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#include <exprOpp.hpp>


// find bounds of -x given bounds on x

void exprOpp::getBounds (expression *&lb, expression *&ub) {

  expression *lba, *uba;
  argument_ -> getBounds (lba, uba);

  lb = new exprOpp (uba);
  ub = new exprOpp (lba);
}


// differentiation

inline expression *exprOpp::differentiate (int index) 
{return new exprOpp (argument_ -> differentiate (index));}


/// implied bound processing for expression w = -x, upon change in
/// lower- and/or upper bound of w, whose index is wind

bool exprOpp::impliedBound (int wind, CouNumber *l, CouNumber *u, t_chg_bounds *chg) {

  int ind = argument_ -> Index ();
  bool res = false;

  if (updateBound (-1, l + ind, - u [wind])) {res = true; chg [ind].lower = CHANGED;}
  if (updateBound ( 1, u + ind, - l [wind])) {res = true; chg [ind].upper = CHANGED;}
  return res;
}


/// simplification

expression *exprOpp::simplify () {

  if (argument_ -> code () == COU_EXPROPP) {
    expression *ret = argument_ -> Argument () -> clone ();
    delete argument_;
    return ret;
  }

  return NULL;
}

