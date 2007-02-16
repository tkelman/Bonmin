/*
 * Name:    exprCos.cpp
 * Author:  Pietro Belotti
 * Purpose: methods for of cosine 
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#include <exprCos.h>
#include <exprSin.h>
#include <exprOpp.h>
#include <exprMul.h>
#include <exprClone.h>
#include <math.h>


// return an expression -sin (argument), the derivative of cos (argument)

expression *exprCos::differentiate (int index) {

  expression **arglist = new expression * [2];

  arglist [0] = new exprOpp (new exprSin (new exprClone (argument_)));
  arglist [1] = argument_ -> differentiate (index);

  return new exprMul (arglist, 2);
}


// I/O

void exprCos::print (std::ostream& out) const
  {exprUnary::print (out, "cos", PRE);}
