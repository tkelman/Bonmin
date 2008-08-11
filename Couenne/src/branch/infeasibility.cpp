/*
 * Name:    infeasibility.cpp
 * Authors: Pietro Belotti, Carnegie Mellon University
 * Purpose: Compute infeasibility of a variable, looking at all expressions it appears in
 *
 * (C) Carnegie-Mellon University, 2008.
 * This file is licensed under the Common Public License (CPL)
 */

#include "CoinHelperFunctions.hpp"

#include "CouenneProblem.hpp"
#include "CouenneVarObject.hpp"

/// weights for computing infeasibility
const CouNumber weiMin = 0.8; // for minimum of infeasibilities of nonlinear objects
const CouNumber weiMax = 1.3; //     maximum
const CouNumber weiSum = 0.1; //     sum
const CouNumber weiAvg = 0.0; //     average


/// compute infeasibility of this variable, |w - f(x)| (where w is
/// the auxiliary variable defined as w = f(x))
/// TODO: suggest way
double CouenneVarObject::infeasibility (const OsiBranchingInformation *info, int &way) const {

  assert (reference_);
  int index = reference_ -> Index ();

  if (info -> lower_ [index] >= 
      info -> upper_ [index] - CoinMin (COUENNE_EPS, feas_tolerance_))
    return 0.;

  problem_ -> domain () -> push 
    (problem_ -> nVars (),
     info -> solution_, 
     info -> lower_, 
     info -> upper_);

  const std::set <int> &dependence = problem_ -> Dependence () [index];

  //////////////////////////////////////////////
  CouNumber retval = checkInfeasibility (info);
  //////////////////////////////////////////////

  if (//(retval > CoinMin (COUENNE_EPS, feas_tolerance_)) &&
      (jnlst_ -> ProduceOutput (J_MATRIX, J_BRANCHING))) {

    printf ("infeasVar %-10g [", retval);

    reference_             -> print (); 
    if (dependence.size () == 0) { // if no list, print image
      printf (" := ");
      reference_ -> Image () -> print ();
    }
    printf ("]\n");
  }

  // add term to stop branching on very tiny intervals

  // Compute the up and down estimates here
  // TODO: We probably only have to do that if pseudo costs option has
  // been chosen

  CouNumber brkPt = computeBranchingPoint (info, way);

  if (pseudoMultType_ != PROJECTDIST)
    setEstimates (info, &retval, &brkPt);

  if (jnlst_ -> ProduceOutput (J_MATRIX, J_BRANCHING)) {
    printf("index = %d up = %e down = %e bounds [%e,%e] brpt = %e\n", 
	   index, upEstimate_, downEstimate_, 
	   info -> lower_ [index],
	   info -> upper_ [index], brkPt);
  }

  problem_ -> domain () -> pop ();

  return ((retval < CoinMin (COUENNE_EPS, feas_tolerance_)) ? 0. : (retval));
}


/// compute infeasibility of this variable, |w - f(x)|, where w is
/// the auxiliary variable defined as w = f(x)
double CouenneVarObject::checkInfeasibility (const OsiBranchingInformation * info) const {

  int index = reference_ -> Index ();

  const std::set <int> &dependence = problem_ -> Dependence () [index];

  if (dependence.size () == 0) { // this is a top level auxiliary,
				 // nowhere an independent

    const CouenneObject &obj = problem_ -> Objects () [reference_ -> Index ()];

    return (obj. Reference ()) ? weiSum * obj.checkInfeasibility (info) : 0.;

  } else {

    CouNumber 
      infsum = 0.,
      infmax = 0.,
      infmin = COIN_DBL_MAX;

    for (std::set <int>::const_iterator i = dependence.begin ();
	 i != dependence.end (); ++i) {

      // *i is the index of an auxiliary that depends on reference_

      const CouenneObject &obj = problem_ -> Objects () [*i];
      CouNumber infeas = (obj. Reference ()) ? obj.checkInfeasibility (info) : 0.;

      if (infeas > infmax) infmax = infeas;
      if (infeas < infmin) infmin = infeas;
      infsum += infeas;
    }

    return 
      // to avoid very small intervals -- removed as now scale w/ norm of gradient
      //(1. - exp (info -> lower_ [index] - info -> upper_ [index])) *
      // to consider maximum, minimum, and sum/avg of the infeasibilities
      (weiSum * infsum + 
       weiAvg * (infsum / CoinMax (1., (CouNumber) dependence.size ())) + 
       weiMin * infmin + 
       weiMax * infmax);
  }
}