/*
 * Name:    CouenneVarObject.cpp
 * Authors: Pietro Belotti, Carnegie Mellon University
 * Purpose: Base object for variables (to be used in branching)
 *
 * (C) Carnegie-Mellon University, 2008.
 * This file is licensed under the Common Public License (CPL)
 */

#include "CoinHelperFunctions.hpp"

#include "CouenneProblem.hpp"
#include "CouenneVarObject.hpp"
#include "CouenneBranchingObject.hpp"

//#define DEBUG

/// Constructor with information for branching point selection strategy
CouenneVarObject::CouenneVarObject (exprVar *ref, 
				    CouenneProblem *p,
				    Bonmin::BabSetupBase *base, 
				    JnlstPtr jnlst):

  // do not set variable (expression), so that no expression-dependent
  // strategy is chosen
  CouenneObject (NULL, base, jnlst),
  problem_      (p) {

  reference_ = ref;

  if (jnlst_ -> ProduceOutput (J_SUMMARY, J_BRANCHING)) {

    printf ("created Variable Object: "); 
    reference_ -> print (); 

    printf (" with %s strategy [clamp=%g, alpha=%g]\n", 
	    (strategy_ == LP_CLAMPED)   ? "lp-clamped" : 
	    (strategy_ == LP_CENTRAL)   ? "lp-central" : 
	    (strategy_ == BALANCED)     ? "balanced"   : 
	    (strategy_ == MIN_AREA)     ? "min-area"   : 
	    (strategy_ == MID_INTERVAL) ? "mid-point"  : 
	    (strategy_ == NO_BRANCH)    ? "no-branching (null infeasibility)" : 
	                                  "no strategy",
	    lp_clamp_, alpha_);
  }
}

/// Copy constructor
CouenneVarObject::CouenneVarObject (const CouenneVarObject &src):
  CouenneObject (src),
  problem_      (src.problem_) {}


/// apply the branching rule
OsiBranchingObject* CouenneVarObject::createBranch (OsiSolverInterface *si, 
						    const OsiBranchingInformation *info, 
						    int way) const {

  // a nonlinear constraint w = f(x) is violated. The infeasibility is
  // given by something more elaborate than |w-f(x)|, that is, it is
  // the minimum, among the two branching nodes, of the distance from
  // the current optimum (w,x) and the optimum obtained after
  // convexifying the two subproblems. We call selectBranch for the
  // purpose, and save the output parameter into the branching point
  // that should be used later in createBranch.

  problem_ -> domain () -> push 
    (problem_ -> nVars (),
     info -> solution_,
     info -> lower_,
     info -> upper_); // have to alloc+copy

  int bestWay;
  CouNumber bestPt = computeBranchingPoint (info, bestWay);

  ///////////////////////////////////////////

#ifdef DEBUG
  printf (":::: creating branching\n");
#endif

  CouenneBranchingObject *brObj = new CouenneBranchingObject 
    (jnlst_, reference_, bestWay ? TWO_RIGHT : TWO_LEFT, bestPt, doFBBT_, doConvCuts_);

  problem_ -> domain () -> pop ();

  return brObj;
}

CouNumber
CouenneVarObject::computeBranchingPoint(const OsiBranchingInformation *info,
					int& bestWay) const
{

#ifdef DEBUG
  printf ("---------- computeBRPT for "); reference_ -> print (); 
  printf (" [%g,%g]\n", 
	  info -> lower_ [reference_ -> Index ()],
	  info -> upper_ [reference_ -> Index ()]);
#endif

  expression *brVar = NULL; // branching variable

  CouNumber
    bestPt,
    *brPts  = NULL, // branching point(s)
    *brDist = NULL, // distances from current LP point to each
		    // new convexification (usually two)
    maxdist = - COIN_DBL_MAX;

  bool chosen = false;

  bestWay = TWO_LEFT;
  int whichWay = TWO_LEFT,
    index = reference_ -> Index ();

  std::set <int> deplist = problem_ -> Dependence () [index];

  for (std::set <int>::iterator i = deplist.begin (); i != deplist.end (); ++i) {

    CouenneObject obj = problem_ -> Objects () [*i];

    CouNumber improv = 0.;

    assert (obj. Reference ());

#ifdef DEBUG
    printf ("  ** "); 
    obj. Reference ()             -> print (); printf (" := ");
    obj. Reference () -> Image () -> print (); printf ("\n");
#endif

    if (obj. Reference ())
      improv = obj. Reference () -> Image ()
	-> selectBranch (&obj, info,                      // input parameters
			 brVar, brPts, brDist, whichWay); // result: who, where, distances, direction

#ifdef DEBUG
    printf ("  --> "); 
    if (brVar) brVar -> print (); 
    printf (" at %g, improv %g <%g>, indices = %d,%d\n", 
	    *brPts, improv, maxdist, index, brVar -> Index ());
#endif

    if (brVar &&
	(brVar -> Index () == index) &&    // it's us!
	(fabs (improv) > maxdist) &&       // this branching seems to induce a higher improvement
	(fabs (*brPts) < COU_MAX_COEFF)) { // and branching pt is limited

      chosen = true;
      bestPt = *brPts;
      maxdist = improv;
      bestWay = whichWay;
    }
  }

  // no hits on this VarObject's variable, that is, this variable was
  // never chosen 

  if (!chosen) {

    bestPt = info -> solution_ [index];

    CouNumber 
      l = info -> lower_ [index], 
      u = info -> upper_ [index];

    switch (strategy_) {

    case CouenneObject::LP_CLAMPED: {
      CouNumber width = lp_clamp_ * (u-l);
      bestPt = CoinMax (l + width, CoinMin (bestPt, u - width));
    } break;
    case CouenneObject::LP_CENTRAL: {
      CouNumber width = lp_clamp_ * (u-l);
      bestPt = ((bestPt < l + width) || (bestPt > u - width)) ? (l+u)/2 : bestPt;
    } break;
    case CouenneObject::MID_INTERVAL: 
    default:                          bestPt = midInterval (bestPt, l, u);

#ifdef DEBUG
      if (CoinMin (fabs (bestPt - l), fabs (bestPt - u)) < 1e-3) {
	printf (  "computed failsafe %g [%g,%g] for ", 
		bestPt, l,u);
	reference_ -> print ();
	printf ("\n");
      }
#endif 
      break;
    }

#ifdef DEBUG
    printf ("  ::: failsafe:  %g [%g,%g] for ", 
	    bestPt, 
	    info -> lower_ [index], 
	    info -> upper_ [index]); 
    reference_ -> print ();
    printf ("\n");
#endif

  } else {

#ifdef DEBUG
    if (CoinMin (fabs (bestPt - info -> lower_ [index]), 
		 fabs (bestPt - info -> upper_ [index])) < 1e-3) {
      printf ("  computed %g [%g,%g] for ", 
	      bestPt, 
	      info -> lower_ [index], 
	      info -> upper_ [index]); 
      reference_ -> print ();
      printf ("\n");
    }
#endif
  }

  if (brPts)
    free (brPts);

  return bestPt;
}
