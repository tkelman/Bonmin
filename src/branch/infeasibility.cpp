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

const CouNumber weiMin = 0.8;
const CouNumber weiMax = 0.3;
const CouNumber weiSum = 1.0; // at least 1 so top level aux are avoided
const CouNumber weiAvg = 0.0;


/// compute infeasibility of this variable, |w - f(x)| (where w is
/// the auxiliary variable defined as w = f(x))
/// TODO: suggest way
double CouenneVarObject::infeasibility (const OsiBranchingInformation *info, int &way) const {

  assert (reference_);

  const std::set <int> &dependence = problem_ -> Dependence () [reference_ -> Index ()];

  CouNumber retval;

  if (dependence.size () == 0) { // this is a top level auxiliary, not appearing as independent

    //retval = fabs ((*reference_) () - (*(reference_ -> Image ())) ());
    const CouenneObject &obj = problem_ -> Objects () [reference_ -> Index ()];
    retval =  (obj. Reference ()) ? obj.infeasibility (info, way) : 0.;
  }
  else {

    CouNumber 
      infsum = 0.,
      infmax = 0.,
      infmin = COIN_DBL_MAX;

    for (std::set <int>::const_iterator i = dependence.begin ();
	 i != dependence.end (); ++i) {

      // *i is the index of an auxiliary that depends on reference_

      const CouenneObject &obj = problem_ -> Objects () [*i];
      CouNumber infeas = (obj. Reference ()) ? obj.infeasibility (info, way) : 0.;

      if (infeas > infmax) infmax = infeas;
      if (infeas < infmin) infmin = infeas;
      infsum += infeas;
    }

    retval = 
      weiSum * infsum + 
      weiAvg * (infsum / CoinMax (1., (CouNumber) dependence.size ())) + 
      weiMin * infmin + 
      weiMax * infmax;
  }

  if (jnlst_->ProduceOutput(J_MATRIX, J_BRANCHING)) {
    printf ("infeasVar %-10g [", retval); 
    reference_             -> print (); 
    if (dependence.size () == 0) { // if no list, print image
      printf (" := ");
      reference_ -> Image () -> print ();
    }
    printf ("]\n");
  }

  return ((retval < CoinMin (COUENNE_EPS, feas_tolerance_)) ? 0. : retval);
}
