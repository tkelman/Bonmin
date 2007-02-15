/*
 * Name:    exprPow.h
 * Author:  Pietro Belotti
 * Purpose: definition of powers
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRPOW_H
#define COUENNE_EXPRPOW_H

#include <math.h>

#include <exprOp.h>
#include <exprMul.h>
#include <exprClone.h>
#include <exprConst.h>


// Power of an expression (binary operator)

class exprPow: public exprOp {

 public:

  // Constructors, destructor
  exprPow  (expression **al, int n = 2): 
    exprOp (al, n) {} //< non-leaf expression, with argument list

  exprPow (expression *arg0, expression *arg1):
    exprOp (arg0, arg1) {}

  ~exprPow () {}

  // cloning method
  expression *clone () const
    {return new exprPow (clonearglist (), nargs_);}

  // String equivalent (for comparisons)
  const std::string name () const 
    {return "^" + exprOp::name();}

  // I/O
  void print (std::ostream&) const;

  // function for the evaluation of the expression
  CouNumber operator () ();

  // differentiation
  expression *differentiate (int index); 

  // simplification
  expression *simplify ();

  // get a measure of "how linear" the expression is
  int Linearity ();

  // Get lower and upper bound of an expression (if any)
  void getBounds (expression *&, expression *&);

  // reduce expression in standard form, creating additional aux
  // variables (and constraints)
  exprAux *standardize (CouenneProblem *p);

  // generate equality between *this and *w
  void generateCuts (exprAux *w, const OsiSolverInterface &si, 
		     OsiCuts &cs, const CouenneCutGenerator *cg);

  // return an index to the variable's argument that is better fixed
  // in a branching rule for solving a nonconvexity gap
  //   getFixVar () {return arglist_ [0];}
};


// compute power and check for integer-and-odd inverse exponent

inline CouNumber safe_pow (register CouNumber base, 
			   register CouNumber exponent) {

  register int rndexp;

  if ((base < 0) && 
      ((fabs (exponent - (rndexp = FELINE_round (exponent))) < COUENNE_EPS) ||
       ((fabs (exponent) > COUENNE_EPS) && 
	(fabs (1. / exponent - (rndexp = FELINE_round (1. / exponent))) < COUENNE_EPS)))
      && (rndexp % 2))
    return (- pow (- base, exponent));

  return (pow (base, exponent));
}


// compute power

inline CouNumber exprPow::operator () () {

  exprOp:: operator () ();

  register CouNumber exponent = *sp--;
  register CouNumber base     = *sp--;

  return (currValue_ = safe_pow (base, exponent));
}


// add upper/lower envelope to power in convex/concave areas

void addPowEnvelope (const CouenneCutGenerator *, OsiCuts &, int, int,
		     CouNumber, CouNumber, CouNumber, CouNumber, int);
#endif
