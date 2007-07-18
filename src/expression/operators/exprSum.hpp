/*
 * Name:    exprSum.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of sum expressions
 *
 * (C) Pietro Belotti 2006. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRSUM_H
#define COUENNE_EXPRSUM_H

#include <exprOp.hpp>


/// class sum 

class exprSum: public exprOp {

 public:

  /// Constructors, destructor
  exprSum  (expression **, int);

  /// Constructor with two elements
  exprSum (expression *, expression *);

  /// cloning method
  virtual expression *clone () const
    {return new exprSum (clonearglist (), nargs_);}

  /// print operator
  std::string printOp () const
    {return "+";}

  /// function for the evaluation of the expression
  virtual CouNumber operator () ();

  /// differentiation
  virtual expression *differentiate (int index); 

  /// simplification
  virtual expression *simplify ();

  /// get a measure of "how linear" the expression is:
  virtual int Linearity ();

  /// Get lower and upper bound of an expression (if any)
  virtual void getBounds (expression *&, expression *&);

  /// reduce expression in standard form, creating additional aux
  /// variables (and constraints)
  virtual exprAux *standardize (CouenneProblem *p);

  /// special version for linear constraints
  virtual void generateCuts (exprAux *, const OsiSolverInterface &, 
			     OsiCuts &, const CouenneCutGenerator *,
			     t_chg_bounds * = NULL, int = -1, 
			     CouNumber = -COUENNE_INFINITY, 
			     CouNumber =  COUENNE_INFINITY);

  /// code for comparison
  virtual enum expr_type code () 
    {return COU_EXPRSUM;}

  /// implied bound processing
  virtual bool impliedBound (int, CouNumber *, CouNumber *, t_chg_bounds *);

  /// compute best variable to branch on (nonsense here, as there is
  /// no nonlinear infeasibility)
  //virtual expression *getFixVar () 
  //{printf ("### Warning: called empty exprSum::getFixVar\n"); return *arglist_;}
};


/// compute sum

inline CouNumber exprSum::operator () () {

  register CouNumber ret = 0;

  expression **al = arglist_;

  for (register int n = nargs_; n--;)
    ret += (**al++) ();

  return (currValue_ = ret);
}

#endif
