/*
 * Name:    exprLog.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of logarithm
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRLOG_HPP
#define COUENNE_EXPRLOG_HPP

#include "exprInv.hpp"
#include "expression.hpp"


/// class logarithm

class exprLog: public exprUnary {

 public:

  /// Constructors, destructor
  exprLog  (expression *al): 
    exprUnary (al) {} // non-leaf expression, with argument list

  /// cloning method
  expression *clone (const std::vector <exprVar *> *variables = NULL) const
    {return new exprLog (argument_ -> clone (variables));}

  /// the operator's function
  inline unary_function F () {return log;}

  /// print operator
  std::string printOp () const
    {return "log";}

  /// differentiation
  expression *differentiate (int index); 

  /// Get lower and upper bound of an expression (if any)
  void getBounds (expression *&, expression *&);

  /// generate equality between *this and *w
  void generateCuts (expression *w, const OsiSolverInterface &si, 
		     OsiCuts &cs, const CouenneCutGenerator *cg, 
		     t_chg_bounds * = NULL, int = -1, 
		     CouNumber = -COUENNE_INFINITY, 
		     CouNumber =  COUENNE_INFINITY);

  /// code for comparisons
  virtual enum expr_type code () 
  {return COU_EXPRLOG;}

  /// implied bound processing
  bool impliedBound (int, CouNumber *, CouNumber *, t_chg_bounds *);

  /// set up branching object by evaluating many branching points for
  /// each expression's arguments
  virtual CouNumber selectBranch (const CouenneObject *obj, 
				  const OsiBranchingInformation *info,
				  expression * &var, 
				  double * &brpts, 
				  int &way);
};

#endif