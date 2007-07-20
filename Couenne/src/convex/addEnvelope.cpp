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
#include <CouenneCutGenerator.hpp>

void CouenneCutGenerator::addEnvelope (OsiCuts &cs, int sign,
				       unary_function f,      // function to be linearized
				       unary_function fprime, // derivative of f
				       int w_ind, int x_ind, 
				       CouNumber x, CouNumber l, CouNumber u,
				       t_chg_bounds *chg,
				       bool is_global) const {
  CouNumber opp_slope = - fprime (x);

  // TODO: remove check of !firstcall_ if point is available already

  // if bounds are very close, convexify with a single line

  bool cLeft  = !chg || (chg [x_ind].lower != UNCHANGED) || firstcall_,
       cRight = !chg || (chg [x_ind].upper != UNCHANGED) || firstcall_;

  if (fabs (u - l) < COUENNE_EPS) {

    CouNumber x0 = 0.5 * (u+l), fp0 = fprime (x0);
    if (cLeft || cRight) createCut (cs, f(x0) - fp0 * x0, 0, w_ind, 1., x_ind, - fp0);
    return;
  }

  // Add tangent in any case

  if (((!firstcall_) || ((x >= l) && (x <= u)))
      && (fabs (opp_slope) < COUENNE_INFINITY))
    createCut (cs, f (x) + opp_slope * x, sign, w_ind, 1., 
	       x_ind, opp_slope, -1, 0., is_global);

    //      addTangent (cs, w_ind, x_ind, x, f (x), fprime (x), sign);

  if ((convtype_ == UNIFORM_GRID) || firstcall_) {

    if (cLeft || cRight) {
      // now add tangent at each sampling point

      CouNumber sample = l, 
	step   = (u-l) / nSamples_;

      //    printf ("[%.4f %.4f], step = %.4f, %d samples\n", 
      //	    l, u, step, nSamples_);

      for (int i = 0; i <= nSamples_; i++) {

	opp_slope = - fprime (sample);

	if ((fabs (opp_slope) < COUENNE_INFINITY) && 
	    (fabs (sample-x) > COUENNE_EPS)) // do not add twice cut at current point
	  createCut (cs, f (sample) + opp_slope * sample, sign, 
		     w_ind, 1.,
		     x_ind, opp_slope, 
		     -1, 0., is_global);

	//	printf ("  Uniform %d: ", i); cut -> print ();

	sample += step;
      }
    }
  }
  else if (convtype_ != CURRENT_ONLY) {

    CouNumber sample = x;

    if (fabs (opp_slope) < COUENNE_INFINITY)
      createCut (cs, f (x) + opp_slope * x, sign, 
		 w_ind, 1.,
		 x_ind, opp_slope, 
		 -1, 0.,
		 is_global);
      //      printf ("  Current tangent: "); cut -> print ();

    for (int i = 0; i <= nSamples_/2; i++) {

      sample += (x-l) / nSamples_;
      opp_slope = - fprime (sample);

      if (fabs (opp_slope) < COUENNE_INFINITY)
	createCut (cs, f (sample) + opp_slope * sample, sign, 
		   w_ind, 1.,
		   x_ind, opp_slope, 
		   -1, 0.,
		   is_global);
	//	printf ("  neighbour -%d: ", i); cut -> print ();
    }

    sample = x;

    for (int i = 0; i <= nSamples_/2; i++) {

      sample += (u-x) / nSamples_;
      opp_slope = - fprime (sample);

      createCut (cs, f (sample) + opp_slope * sample, sign, 
		 w_ind, 1.,
		 x_ind, opp_slope, 
		 -1, 0.,
		 is_global);
	//	printf ("  neighbour  %d: ", i); cut -> print ();
    }
  }
}