/*
 * Name:    exprMin.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of $\f(x_{\argmin_{i\in I} y_i})$ 
 *
 * (C) Pietro Belotti 2006. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRMIN_H
#define COUENNE_EXPRMIN_H

#include <exprOp.hpp>


/// class for minima

class exprMin: public exprOp {

 public:

  /// Constructor
  exprMin  (expression **al, int n): 
    exprOp (al, n) {}
 
  /// Constructor with only two arguments
  exprMin  (expression *el0, expression *el1):
    exprOp (new expression * [4], 4) {
    arglist_ [0] = el0; arglist_ [1] = new exprClone (el0);
    arglist_ [2] = el1; arglist_ [3] = new exprClone (el1);
  }

  /// cloning method
  exprMin *clone () const
    {return new exprMin (clonearglist (), nargs_);}

  /// print operator
  std::string printOp () const
    {return "min";}

  /// print operator
  enum pos printPos () const
    {return PRE;}

  /// function for the evaluation of the expression
  CouNumber operator () ();

  /// differentiation
  inline expression *differentiate (int) 
    {return NULL;} 

  /// simplification
  inline expression *simplify () 
    {return NULL;}

  /// get a measure of "how linear" the expression is (see CouenneTypes.h)
  virtual inline int Linearity () 
    {return NONLINEAR;}

  // Get lower and upper bound of an expression (if any)
  //  void getBounds (expression *&, expression *&);

  /// reduce expression in standard form, creating additional aux
  /// variables (and constraints)
  virtual inline exprAux *standardize (CouenneProblem *, bool addAux = true)
    {return NULL;}

  /// generate equality between *this and *w
  void generateCuts (exprAux *w, const OsiSolverInterface &si, 
		     OsiCuts &cs, const CouenneCutGenerator *cg, 
		     t_chg_bounds * = NULL, int = -1, 
		     CouNumber = -COUENNE_INFINITY, 
		     CouNumber =  COUENNE_INFINITY);

  /// code for comparisons
  virtual enum expr_type code () 
  {return COU_EXPRMIN;}
};


/// compute maximum

inline CouNumber exprMin::operator () () {

  CouNumber best_val = (*(arglist_ [0])) ();
  int best_ind = 0;

  for (int ind = 2; ind < nargs_; ind += 2) {

    register CouNumber val = (*(arglist_ [ind])) ();

    if (val < best_val) {
      best_ind = ind;
      best_val = val;
    }
  }

  best_val = (*(arglist_ [best_ind + 1])) ();

  return (currValue_ = best_val);
}

#endif
