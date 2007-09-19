/*
 * Name:    exprClone.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of the clone class (different from exprCopy in
 *          that evaluation are propagated)
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRCLONE_H
#define COUENNE_EXPRCLONE_H

#include <iostream>

#include <CouenneTypes.h>
#include <exprCopy.hpp>


/// expression clone (points to another expression) 

class exprClone: public exprCopy {

 public:

  /// Constructor
  exprClone  (expression *copy): 
    exprCopy (copy) {}

  /// copy constructor
  exprClone (const exprClone &e):
    exprCopy (e) {}

  /// cloning method
  exprClone *clone () const
  {return new exprClone (*this);}

  /// I/O
  void print (std::ostream &out = std::cout, 
	      bool descend      = false, 
	      CouenneProblem *p = NULL) const
    {copy_ -> Original () -> print (out, descend, p);}

  /// value
  inline CouNumber Value () const 
    {return copy_ -> Value ();}

  /// null function for evaluating the expression
  inline CouNumber operator () () 
    {return (currValue_ = (*copy_) ());}
};

#endif