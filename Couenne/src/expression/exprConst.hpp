/*
 * Name:    exprConst.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of the class exprConst
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRCONST_H
#define COUENNE_EXPRCONST_H

#include <iostream>

#include <CouenneTypes.h>
#include <expression.hpp>
#include <exprClone.hpp>


/// constant-type operator

class exprConst: public expression {

 public:

  /// node type
  inline enum nodeType Type () 
    {return CONST;}

  /// value of expression
  inline CouNumber Value () const 
    {return currValue_;}

  /// Constructor
  exprConst (CouNumber value)
    {currValue_ = value;}

  /// Copy constructor
  exprConst (const exprConst &e)
    {currValue_ = e.Value ();}

  /// Cloning method
  virtual exprConst *clone () const
    {return new exprConst (currValue_);}

  /// I/O
  void print (std::ostream &out = std::cout, 
	      bool = false, CouenneProblem * = NULL) const
    {out << currValue_;}

  /// return constant's value
  inline CouNumber operator() () 
    {return currValue_;}

  /// differentiation
  inline expression *differentiate (int) 
    {return new exprConst (0);}

  /// dependence on variable set
  inline int dependsOn (int *, int) 
    {return 0;}

  /// simplify
  inline expression *simplify () 
    {return NULL;}

  /// get a measure of "how linear" the expression is (see CouenneTypes.h)
  inline int Linearity ()
    {return ((fabs (currValue_) < COUENNE_EPS) ? ZERO: CONSTANT);}

  /// Get lower and upper bound of an expression (if any)
  inline void getBounds (expression *&lower, expression *&upper) {
    lower = new exprClone (this);
    upper = new exprClone (this);
  }

  /// Create standard formulation of this expression
  inline exprAux *standardize (CouenneProblem *)
    {return NULL;}

  /// generate convexification cut for constraint w = this
  void generateCuts (exprAux *, const OsiSolverInterface &, 
		     OsiCuts &, const CouenneCutGenerator *, 
		     t_chg_bounds * = NULL, int = -1, 
		     CouNumber = -COUENNE_INFINITY, 
		     CouNumber =  COUENNE_INFINITY);

  /// code for comparisons
  virtual enum expr_type code () {return COU_EXPRCONST;}

  /// used in rank-based branching variable choice
  virtual int rank (CouenneProblem *p)
    {return 0;} 
};

#endif