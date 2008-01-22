/*
 * Name:    impliedBounds.cpp
 * Author:  Pietro Belotti
 * Purpose: backward implied bound search
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include "CouenneProblem.hpp"

/// Bound tightening for auxiliary variables

int CouenneProblem::impliedBounds (t_chg_bounds *chg_bds) const {

  int nchg = 0; //< number of bounds changed for propagation

  /*if (Jnlst()->ProduceOutput(Ipopt::J_VECTOR, J_BOUNDTIGHTENING)) {  
    Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,"=====================implied\n");
    int j=0;
    for (int i=0; i < nVars (); i++) 
      if (variables_ [i] -> Multiplicity () > 0) {
	Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,
			"x_%03d [%+10g %+10g] ", i, 
			expression::Lbound (i),
			expression::Ubound (i));
	if (!(++j % 6)) Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,"\n");
      }
    if (j % 6) Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,"\n");
    }*/

  for (int ii = nVars (); ii--;) {

    int i = numbering_ [ii];

    if (variables_ [i] -> Type () == AUX) {

      if (lb_ [i] > ub_ [i] + COUENNE_EPS) {
	Jnlst()->Printf(Ipopt::J_DETAILED, J_BOUNDTIGHTENING,
			"implied bounds: w_%d has infeasible bounds [%g,%g]\n", 
			i, lb_ [i], ub_ [i]);
	return -1;
      }

      //    if ((auxiliaries_ [i] -> Image () -> code () == COU_EXPRSUM) ||
      //	(auxiliaries_ [i] -> Image () -> code () == COU_EXPRGROUP))

      /*if (auxiliaries_ [i] -> Image () -> Argument () || 
	  auxiliaries_ [i] -> Image () -> ArgList  ()) {

	expression *arg = auxiliaries_ [i] -> Image () -> Argument ();
	if (!arg)   arg = auxiliaries_ [i] -> Image () -> ArgList  () [0];

	printf (":::: ");
	  arg -> print (std::cout);
	  if (arg -> Index () >= 0) {
	  int ind = arg -> Index ();
	  printf (" in [%g,%g]", 
	  expression::Lbound (ind), 
	  expression::Ubound (ind));
	  }
	  printf ("\n");
      }*/

      // TODO: also test if this expression, or any of its indep
      // variables, have changed. If not, skip

      /*CouNumber 
	l0 = lb_ [i], 
	u0 = ub_ [i];*/

      if (variables_ [i] -> Image () -> impliedBound 
	  (variables_ [i] -> Index (), lb_, ub_, chg_bds)) {

	/*if (Jnlst()->ProduceOutput(Ipopt::J_VECTOR, J_BOUNDTIGHTENING)) {
	  // todo: send all output through journalist
	  Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,
			  "impli %2d [%15.8g, %15.8g] -> [%15.8g, %15.8g]: ",
			  i, l0, u0, lb_ [i], ub_ [i]);

	  variables_ [i]             -> print (std::cout);
	  Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING," := ");
	  variables_ [i] -> Image () -> print (std::cout);
	  Jnlst()->Printf(Ipopt::J_VECTOR, J_BOUNDTIGHTENING,"\n");
	  }*/

	/*for (int i=0; i<nVars (); i++) 
	  printf ("%4d. [%20.8f, %20.8f]\n", i,
		  expression::Lbound (i),
		  expression::Ubound (i));*/

	/*
	if (optimum_ && 
	    ((optimum_ [i+nvar] < lb_ [i+nvar] - COUENNE_EPS) ||
	     (optimum_ [i+nvar] > ub_ [i+nvar] + COUENNE_EPS)))
	  printf ("#### implied b_%d [%g,%g] cuts optimum %g: [%g --> %g, %g <-- %g]\n", 
		  i+nvar, expression::Lbound (i+nvar), expression::Ubound (i+nvar), 
		  optimum_ [i+nvar], l0, lb_ [i+nvar], ub_ [i+nvar], u0);

	//printf ("impli %2d ", nvar+i);

	if (auxiliaries_ [i] -> Image () -> Argument () || 
	    auxiliaries_ [i] -> Image () -> ArgList ()) {

	  expression *arg = auxiliaries_ [i] -> Image () -> Argument ();

	  if (!arg) {
	    for (int k=0; k < auxiliaries_ [i] -> Image () -> nArgs (); k++) {
	      arg =  auxiliaries_ [i] -> Image () -> ArgList () [k];
	      printf (" ");
	      arg -> print (std::cout);
	      if (arg -> Index () >= 0) {
		int ind = arg -> Index ();
		printf (" in [%g,%g]", 
			expression::Lbound (ind), 
			expression::Ubound (ind));
	      }	    
	    }
	  } else {
	    printf (" ");
	    arg -> print (std::cout);
	    if (arg -> Index () >= 0) {
	      int ind = arg -> Index ();
	      printf (" in [%g,%g]", 
		      expression::Lbound (ind), 
		      expression::Ubound (ind));
	    }
	  }
	} else printf (" [no args]");
	printf ("\n");
	*/

	nchg++;
      }
    }
  }

  /*for (int i=0; i < nAuxs (); i++) {

    printf (" [%g, %g]", 
    expression::Lbound (i+nvar),
    expression::Ubound (i+nvar));

    auxiliaries_ [i] -> print (std::cout);
    printf (" := ");
    auxiliaries_ [i] -> Image () -> print (std::cout); fflush (stdout);
    printf ("\n");
  }*/

  return nchg;
}