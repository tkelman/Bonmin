/*
 * File: addEnvelope.cpp
 * Author: Pietro Belotti, Carnegie Mellon University
 * Purpose: add generic envelope to convex function based on function and its derivative
 *
 * (C) Pietro Belotti, all rights reserved.
 * This code is distributed under the Common Public License.
 */


#include <OsiRowCut.hpp>
#include <CouennePrecisions.h>
#include <CouenneTypes.h>
#include <CouenneCutGenerator.h>

void CouenneCutGenerator::addEnvelope (OsiCuts &cs, int sign,
				       unary_function f,      // function to approximate
				       unary_function fprime, // derivative of the function 
				       int w_ind, int x_ind, 
				       CouNumber x, CouNumber l, CouNumber u,
				       bool is_global) const {
  OsiRowCut *cut;
  CouNumber opp_slope = - fprime (x);

  if ((convtype_ == UNIFORM_GRID) || firstcall_) {

    // now add tangent at each sampling point

    CouNumber sample = l, 
              step   = (u-l) / nSamples_;

    for (int i = 0; i <= nSamples_; i++) {

      opp_slope = - fprime (sample);

      if ((fabs (opp_slope) < COUENNE_INFINITY) && 
	  (cut = createCut (f (sample) + opp_slope * sample, sign, 
			    w_ind, CouNumber (1.),
			    x_ind, opp_slope, 
			    -1, 0.,
			    is_global))) {
	//	printf ("  Uniform %d: ", i); cut -> print ();
	cs.insert (cut);
      }

      sample += step;
    }
  } else if (convtype_ == CURRENT_ONLY) {
    if ((fabs (opp_slope) < COUENNE_INFINITY) &&
	((cut = createCut (f (x) + opp_slope * x, sign, w_ind, 1., 
			  x_ind, opp_slope, -1, 0., is_global))))
      cs.insert (cut);
      //      addTangent (cs, w_ind, x_ind, x, f (x), fprime (x), sign);
  }
  else {

    CouNumber sample = x;

    if ((fabs (opp_slope) < COUENNE_INFINITY) && 
	(cut = createCut (f (x) + opp_slope * x, sign, 
			  w_ind, CouNumber (1.),
			  x_ind, opp_slope, 
			  -1, 0.,
			  is_global))) {
      //      printf ("  Current tangent: "); cut -> print ();
      cs.insert (cut);
    }

    for (int i = 0; i <= nSamples_/2; i++) {

      sample += (x-l) / nSamples_;
      opp_slope = - fprime (sample);

      if ((fabs (opp_slope) < COUENNE_INFINITY) &&
	  (cut = createCut (f (sample) + opp_slope * sample, sign, 
			    w_ind, CouNumber (1.),
			    x_ind, opp_slope, 
			    -1, 0.,
			    is_global))) {
	//	printf ("  neighbour -%d: ", i); cut -> print ();
	cs.insert (cut);
      }
    }

    sample = x;

    for (int i = 0; i <= nSamples_/2; i++) {

      sample += (u-x) / nSamples_;
      opp_slope = - fprime (sample);

      if ((cut = createCut (f (sample) + opp_slope * sample, sign, 
			    w_ind, CouNumber (1.),
			    x_ind, opp_slope, 
			    -1, 0.,
			    is_global))) {
	//	printf ("  neighbour  %d: ", i); cut -> print ();
	cs.insert (cut);
      }
    }
  }
}
