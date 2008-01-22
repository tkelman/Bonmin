/*
 * Name:    CouenneObject.hpp
 * Authors: Pierre Bonami, IBM Corp.
 *          Pietro Belotti, Carnegie Mellon University
 * Purpose: Object for auxiliary variables
 *
 * (C) Carnegie-Mellon University, 2006-07.
 * This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNEOBJECT_HPP
#define COUENNEOBJECT_HPP

#include "BonBabSetupBase.hpp"
#include "CoinFinite.hpp"
#include "OsiBranchingObject.hpp"

#include "exprVar.hpp"

#include "CouenneJournalist.hpp"


/// Define what kind of branching (two- or three-way) and where to
/// start from: left, (center,) or right. The last is to help
/// diversify branching through randomization, which may help when the
/// same variable is branched upon in several points of the BB tree.

enum {TWO_LEFT,                 TWO_RIGHT,   TWO_RAND,
      THREE_LEFT, THREE_CENTER, THREE_RIGHT, THREE_RAND, BRANCH_NONE};

//
class funtriplet;
CouNumber minMaxDelta (funtriplet *ft, CouNumber x, CouNumber y, CouNumber lb, CouNumber ub);
CouNumber maxHeight   (funtriplet *ft, CouNumber x, CouNumber y, CouNumber lb, CouNumber ub);

/// returns a point "inside enough" a given interval, or x if it is already
CouNumber midInterval (CouNumber x, CouNumber l, CouNumber u);

/// OsiObject for auxiliary variables $w=f(x)$. 
///
/// Associated with a multi-variate function $f(x)$ and a related
/// infeasibility $|w-f(x)|$, creates branches to help restoring
/// feasibility

class CouenneObject: public OsiObject {

public:

  /// strategy names
  enum brSelStrat {MID_INTERVAL, MIN_AREA, BALANCED};

  /// Constructor with information for branching point selection strategy
  CouenneObject (exprVar *ref, Bonmin::BabSetupBase *base,
		 JnlstPtr jnlst);

  /// Destructor
  ~CouenneObject () 
  {if (brPts_) free (brPts_);}

  /// Copy constructor
  CouenneObject (const CouenneObject &src);

  /// Cloning method
  virtual OsiObject * clone () const
  {return new CouenneObject (*this);}

  /// compute infeasibility of this variable, |w - f(x)| (where w is
  /// the auxiliary variable defined as w = f(x)
  virtual double infeasibility (const OsiBranchingInformation*, int &) const;

  /// fix (one of the) arguments of reference auxiliary variable 
  virtual double feasibleRegion (OsiSolverInterface*, 
				 const OsiBranchingInformation*) const;

  /// create CouenneBranchingObject or CouenneThreeWayBranchObj based
  /// on this object
  virtual OsiBranchingObject* createBranch (OsiSolverInterface*, 
					    const OsiBranchingInformation*, int) const;

  /// return reference auxiliary variable
  exprVar *Reference () const
  {return reference_;}

  /// return branching point selection strategy
  enum brSelStrat Strategy ()  const
  {return strategy_;}

  /// pick branching point based on current strategy
  CouNumber getBrPoint (funtriplet *ft, CouNumber x0, CouNumber y0, CouNumber l, CouNumber u) const {

    switch (strategy_) {

    case CouenneObject::MIN_AREA:     return maxHeight   (ft, x0, y0, l, u);
    case CouenneObject::BALANCED:     return minMaxDelta (ft, x0, y0, l, u);
    case CouenneObject::MID_INTERVAL: 
    default:                          return midInterval (    x0,     l, u);
    }
  }

protected:

  /// The (auxiliary) variable this branching object refers to. If the
  /// expression is w=f(x,y), this is w, as opposed to
  /// CouenneBranchingObject, where it would be either x or y.
  exprVar *reference_;

  /// index on the branching variable
  mutable expression *brVar_;

  /// where to branch. It is a vector in the event we want to use a
  /// ThreeWayBranching. Ends with a -COIN_DBL_MAX (not considered...)
  mutable CouNumber *brPts_;

  /// How to branch. This should be done automatically from
  /// ::infeasibility() and ::createBranch(), but for some reason
  /// obj->whichWay() in the last 20 lines of CbcNode.cpp returns 0,
  /// therefore we set our own whichWay_ within this object and use it
  /// between the two methods.
  mutable int whichWay_;

  /// Branching point selection strategy
  enum brSelStrat strategy_;

  /// SmartPointer to the Journalist
  JnlstPtr jnlst_;
};

#endif