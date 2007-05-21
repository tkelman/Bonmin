/*
 * Name:    exprOpp.h
 * Author:  Pietro Belotti
 * Purpose: definition of the opposite -f(x) of a function
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPROPP_H
#define COUENNE_EXPROPP_H

#include <exprUnary.h>


// operator opp: returns the opposite of a number

inline CouNumber opp (register CouNumber arg) 
{return - arg;}


// class opposite 

class exprOpp: public exprUnary {

 public:

  // Constructors, destructor
  exprOpp  (expression *al): 
    exprUnary (al) {} //< non-leaf expression, with argument list

  // cloning method
  expression *clone () const
    {return new exprOpp (argument_ -> clone ());}

  /// the operator's function
  inline unary_function F () {return opp;}

  /// print operator
  std::string printOp () const
    {return "-";}

  // I/O
  //  void print (std::ostream&) const;

  // differentiation
  expression *differentiate (int index); 

  // get a measure of "how linear" the expression is (see CouenneTypes.h)
  inline int Linearity ()
    {return argument_ -> Linearity ();}

  // Get lower and upper bound of an expression (if any)
  void getBounds (expression *&, expression *&);

  // generate equality between *this and *w
  void generateCuts (exprAux *w, const OsiSolverInterface &si, 
		     OsiCuts &cs, const CouenneCutGenerator *cg);

  ///
  virtual enum expr_type code () {return COU_EXPROPP;}

  /// implied bound processing
  bool impliedBound (int, CouNumber *, CouNumber *, char *);
};

#endif
