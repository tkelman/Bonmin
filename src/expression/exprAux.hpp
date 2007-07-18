/*
 * Name:    exprAux.hpp
 * Author:  Pietro Belotti
 * Purpose: definition of the auxiliary variable class (used in
 *          standardization and convexification)
 *
 * (C) Pietro Belotti. This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNE_EXPRAUX_H
#define COUENNE_EXPRAUX_H

#include <iostream>
#include <exprVar.hpp>
#include <exprBound.hpp>
#include <exprMax.hpp>
#include <exprMin.hpp>
#include <CouenneTypes.h>
#include <CglCutGenerator.hpp>


/// expression base class

class exprAux: public exprVar {

 protected:

  /// The expression associated with this auxiliary variable
  expression *image_;

  /// bounds determined by the associated expression's bounds and the
  /// relative constraint's bound
  expression *lb_;
  expression *ub_;

  /// used in rank-based branching variable choice: original variables
  /// have rank 1; auxiliary w=f(x) has rank r(w) = r(x)+1; finally,
  /// auxiliary w=f(x1,x2...,xk) has rank r(w) = 1+max{r(xi):i=1..k}.
  int rank_;

  /// number of appearances of this aux in the formulation. The more
  /// times it occurs in the formulation, the more implication its
  /// branching has on other variables
  int multiplicity_;

 public:

  /// Node type
  inline enum nodeType Type () 
    {return AUX;}

  /// Constructor
  exprAux (expression *, int, int);

  /// Destructor
  ~exprAux () {
    delete image_; 
    delete lb_; 
    delete ub_;
  }

  /// copy constructor
  exprAux (const exprAux &e):
    exprVar       (e.varIndex_),
    image_        (e.image_ -> clone ()),
    //    lb_           (e.lb_    -> clone ()),
    //    ub_           (e.ub_    -> clone ()),
    rank_         (e.rank_),
    multiplicity_ (e.multiplicity_) {

    image_ -> getBounds (lb_, ub_);
    // getBounds (lb_, ub_);

    lb_ = new exprMax (lb_, new exprLowerBound (varIndex_));
    ub_ = new exprMin (ub_, new exprUpperBound (varIndex_));
  }

  /// cloning method
  virtual exprAux *clone () const
    {return new exprAux (*this);}

  /// Bound get
  expression *Lb () {return lb_;}
  expression *Ub () {return ub_;}

  /// I/O
  virtual void print (std::ostream & = std::cout, bool = false, CouenneProblem * = NULL) const;

  /// The expression associated with this auxiliary variable
  inline expression *Image () const
    {return image_;}

  /// Null function for evaluating the expression
  inline CouNumber operator () () 
    {return (currValue_ = expression::Variable (varIndex_));}

  /// Differentiation
  inline expression *differentiate (int index) 
    {return image_ -> differentiate (index);}

  /// Dependence on variable set: return number of times elements of
  /// indices occur in expression
  inline int dependsOn (int *indices, int num) 
    {return image_ -> dependsOn (indices, num);}

  /// Get a measure of "how linear" the expression is (see CouenneTypes.h)
  inline int Linearity ()
    {return LINEAR;}
    /*return image_ -> Linearity ();*/

  /// Get lower and upper bound of an expression (if any)
  inline void getBounds (expression *&lb, expression *&ub) {

    /// this replaces the previous 
    ///
    ///    image_ -> getBounds (lb0, ub0);
    ///
    /// which created large expression trees, now useless since all
    /// auxiliaries are standardized.

    lb = new exprLowerBound (varIndex_);
    ub = new exprUpperBound (varIndex_);
  }

  /// set bounds depending on both branching rules and propagated
  /// bounds. To be used after standardization
  inline void crossBounds () {

    expression *l0, *u0;

    image_ -> getBounds (l0, u0);

    //image_ -> getBounds (lb_, ub_);

    lb_ = new exprMax (lb_, l0);
    ub_ = new exprMin (ub_, u0);

    /*printf ("lower: ");   lb_ -> print ();
    printf (", upper: "); ub_ -> print ();
    printf ("\n");*/
  }

  /// generate cuts for expression associated with this auxiliary
  void generateCuts (const OsiSolverInterface &, 
		     OsiCuts &, const CouenneCutGenerator *, 
		     t_chg_bounds * = NULL, int = -1, 
		     CouNumber = -COUENNE_INFINITY, 
		     CouNumber =  COUENNE_INFINITY);

  /// used in rank-based branching variable choice
  virtual inline int rank (CouenneProblem *p = NULL)
    {return rank_;} 

  /// Tell this variable appears once more
  inline void increaseMult () {++multiplicity_;}

  /// Tell this variable appears once less (standardized within
  /// exprSum, for instance)
  inline void decreaseMult () {--multiplicity_;}

  /// How many times this variable appears 
  inline int Multiplicity () {return multiplicity_;}
};

/// allow to draw function within intervals and cuts introduced
void draw_cuts (OsiCuts &, const CouenneCutGenerator *, 
		int, expression *, expression *);

#endif