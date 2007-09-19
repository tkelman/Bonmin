/*
 * Name:    exprUnary.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of the class for univariate functions
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRUNARY_H
#define COUENNE_EXPRUNARY_H

#include <iostream>

#include <expression.hpp>
#include <CouenneTypes.h>
#include <exprOp.hpp>


/// zero function (used by default by exprUnary)
inline CouNumber zero_fun (CouNumber x) 
{return 0;}


/// expression class for unary functions (sin, log, etc.)
///
/// univariate operator-type expression: requires single argument. All
/// unary functions are derived from this base class, which has a lot
/// of common methods that need not be re-implemented by any
/// univariate class.

class exprUnary: public expression {

 protected:

  /// single argument taken by this expression
  expression *argument_;

 public:

  /// node type
  virtual inline enum nodeType Type () 
    {return UNARY;}

  /// Constructor
  exprUnary  (expression *argument): 
    argument_ (argument)        //< non-leaf expression, with argument list
   {}

  /// the operator itself (e.g. sin, log...)
  virtual inline unary_function F () 
    {return zero_fun;}

  /// Destructor
  ~exprUnary () 
    {if (argument_) delete argument_;}

  /// return number of arguments
  inline int nArgs () const
    {return 1;}

  /// return argument (when applicable, i.e., with univariate functions)
  virtual inline expression *Argument () const
    {return argument_;}

  /// return pointer to argument (when applicable, i.e., with univariate functions)
  virtual inline expression **ArgPtr () 
    {return &argument_;}

  /// print this expression to iostream
  virtual void print (std::ostream &out = std::cout, 
		      bool = false, CouenneProblem * = NULL) const;

  /// print position (PRE, INSIDE, POST)
  virtual enum pos printPos () const
    {return PRE;}

  /// print operator
  virtual std::string printOp () const
    {return "?";}

  /// compute value of unary operator
  virtual inline CouNumber operator () ()
    {return (currValue_ = (F ()) ((*argument_) ()));}

  /// dependence on variable set
  inline int dependsOn (int *list, int n) 
    {return (argument_ -> dependsOn (list, n));}

  /// simplification
  expression *simplify ();

  /// get a measure of "how linear" the expression is (see CouenneTypes.h)
  /// for general univariate functions, return nonlinear.
  virtual inline int Linearity ()
    {return NONLINEAR;}

  /// reduce expression in standard form, creating additional aux
  /// variables (and constraints)
  virtual exprAux *standardize (CouenneProblem *);

  /// return an index to the variable's argument that is better fixed
  /// in a branching rule for solving a nonconvexity gap
  virtual expression *getFixVar () 
    {return argument_;}

  /// type of operator
  virtual enum expr_type code () 
    {return COU_EXPRUNARY;}

  /// compare two unary functions
  virtual int compare (exprUnary &); 

  /// used in rank-based branching variable choice
  virtual int rank (CouenneProblem *p)
    {return (argument_ -> rank (p));} 

  /// fill in dependence structure
  virtual void fillDepSet (std::set <DepNode *, compNode> *dep, DepGraph *g) 
    {argument_ -> fillDepSet (dep, g);}

  /// replace variable with other
  virtual void replace (exprVar *, exprVar *);
};

#endif