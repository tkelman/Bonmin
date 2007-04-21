/*
 * Name:    expression.cpp
 * Author:  Pietro Belotti
 * Purpose: methods of the expression class
 *
 * This file is licensed under the Common Public License (CPL)
 */

#include <string>
#include <string.h>

#include <CouenneCutGenerator.h>
#include <CouenneProblem.h>

#include <CouenneTypes.h>
#include <expression.h>
#include <exprAux.h>
#include <exprOp.h>
#include <exprUnary.h>
#include <exprVar.h>
#include <exprIVar.h>
#include <exprBound.h>


// print unary expression

void exprUnary::print (std::ostream      &out = std::cout, 
		       const std::string &op  = "unknown", 
		       enum pos           pos = PRE)       const 
{
  if (pos == PRE)  out << op;
  out << "(";
  argument_ -> print (out);
  out << ")";
  if (pos == POST) out << op;
}


/// comparison when looking for duplicates
int exprUnary::compare (exprUnary  &e1) { 

  int c0 = code (),
      c1 = e1. code ();

  if      (c0 < c1) return -1;
  else if (c0 > c1) return  1;
  else // have to compare arguments 
    return argument_ -> compare (*(e1.argument_));
}


// Create standard formulation of this expression, by:
//
// - creating auxiliary w variables and corresponding expressions
// - returning linear counterpart as new constraint (to replace 
//   current one)

exprAux *exprUnary::standardize (CouenneProblem *p) {

  exprAux *subst;

  if ((subst = argument_ -> standardize (p)))
    argument_ = new exprClone (subst);

  return p -> addAuxiliary (this);
}
