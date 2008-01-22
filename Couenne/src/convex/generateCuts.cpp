/*
 * Name:    generateCuts.cpp
 * Author:  Pietro Belotti
 * Purpose: the generateCuts() method of the convexification class CouenneCutGenerator
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#include "BonAuxInfos.hpp"
#include "CglCutGenerator.hpp"

#include "CouenneCutGenerator.hpp"
#include "CouenneProblem.hpp"
#include "CouenneSolverInterface.hpp"

// exception
#define INFEASIBLE 1

// fictitious bound for initial unbounded lp relaxations
#define LARGE_BOUND 9.999e12

// minimum #bound changed in obbt to generate further cuts
#define THRES_NBD_CHANGED 1

// maximum number of obbt iterations
#define MAX_OBBT_ITER 1

#define LARGE_TOL (LARGE_BOUND / 1e6)

// set and lift bound for auxiliary variable associated with objective
// function
void fictitiousBound (OsiCuts &cs,
		      CouenneProblem *p, 
		      bool action) {     // true before convexifying, false afterwards

  // set trivial dual bound to objective function, if there is none

  int ind_obj = p -> Obj (0) -> Body () -> Index ();

  if (ind_obj < 0) return;

  // we have a single variable objective function

  int sense = (p -> Obj (0) -> Sense () == MINIMIZE) ? -1 : 1;

  if (action)
    if (sense<0) {if (p -> Lb (ind_obj) < - LARGE_BOUND) p -> Lb (ind_obj) = - LARGE_BOUND;}
    else         {if (p -> Ub (ind_obj) >   LARGE_BOUND) p -> Ub (ind_obj) =   LARGE_BOUND;}
  else
    if (sense>0) {if (fabs (p->Ub(ind_obj)-LARGE_BOUND)<LARGE_TOL) p->Ub(ind_obj) = COUENNE_INFINITY;}
    else         {if (fabs (p->Lb(ind_obj)+LARGE_BOUND)<LARGE_TOL) p->Lb(ind_obj) =-COUENNE_INFINITY;}
}


// translate changed bound sparse array into a dense one
void sparse2dense (int ncols, t_chg_bounds *chg_bds, int *&changed, int &nchanged) {

  // convert sparse chg_bds in something handier
  // AW: replacd "malloc" here by "realloc"; otherwise this is a memory leak
  //     In general, I don't think it is worth to do a realloc here, it is probably more expensive than not using it.  The memory is free anyway when generateCuts is left
  changed  = (int *) realloc (changed, ncols * sizeof (int));
  nchanged = 0;

  for (register int i=ncols, j=0; i--; j++, chg_bds++)
    if (chg_bds -> lower() != t_chg_bounds::UNCHANGED ||
	chg_bds -> upper() != t_chg_bounds::UNCHANGED ) {
      *changed++ = j;
      nchanged++;
    }

  changed = (int *) realloc (changed - nchanged, nchanged * sizeof (int));
}


/// get new bounds from parents' bounds + branching rules
void updateBranchInfo (const OsiSolverInterface &si, CouenneProblem *p, 
		       t_chg_bounds *chg, const CglTreeInfo &info);

/// a convexifier cut generator

void CouenneCutGenerator::generateCuts (const OsiSolverInterface &si,
					OsiCuts &cs, 
					const CglTreeInfo info) const {
  int nInitCuts = cs.sizeRowCuts ();

  /*  static int count = 0;
  char fname [20];
  sprintf (fname, "relax_%d", count++);
  si.writeLp (fname);*/

  jnlst_ -> Printf (J_DETAILED, J_CONVEXIFYING,
		    ":::::::::: level = %d, pass = %d, intree=%d\n",// Bounds:\n", 
		    info.level, info.pass, info.inTree);

  Bonmin::BabInfo * babInfo = dynamic_cast <Bonmin::BabInfo *> (si.getAuxiliaryInfo ());

  if (babInfo)
    babInfo -> setFeasibleNode ();

  double now   = CoinCpuTime ();
  int    ncols = problem_ -> nVars ();

  // This vector contains variables whose bounds have changed due to
  // branching, reduced cost fixing, or bound tightening below. To be
  // used with malloc/realloc/free

  t_chg_bounds *chg_bds = new t_chg_bounds [ncols];

  if (jnlst_ -> ProduceOutput (J_VECTOR, J_CONVEXIFYING)) {
    jnlst_ -> Printf(J_VECTOR, J_CONVEXIFYING,"=============================\n");
    for (int i = 0; i < problem_ -> nVars (); i++)
      jnlst_->Printf(J_VECTOR, J_CONVEXIFYING,"%4d %+20.8f [%+20.8f,%+20.8f]\n", i,
		     problem_ -> X  (i),
		     problem_ -> Lb (i),
		     problem_ -> Ub (i));
    jnlst_->Printf(J_VECTOR, J_CONVEXIFYING,"=============================\n");
  }

  if (firstcall_) {

    //////////////////////// FIRST CONVEXIFICATION //////////////////////////////////////

    // initialize auxiliary variables and bounds according to originals from NLP
    problem_ -> initAuxs (const_cast <CouNumber *> (nlp_ -> getColSolution ()), 
			  const_cast <CouNumber *> (nlp_ -> getColLower    ()),
			  const_cast <CouNumber *> (nlp_ -> getColUpper    ()));

    // OsiSolverInterface is empty yet, no information can be obtained
    // on variables or bounds -- and none is needed since our
    // constructor populated *problem_ with variables and bounds. We
    // only need to update the auxiliary variables and bounds with
    // their current value.

    // For each auxiliary variable replacing the original (nonlinear)
    // constraints, check if corresponding bounds are violated, and
    // add cut to cs

    int nnlc = problem_ -> nCons ();

    for (int i=0; i<nnlc; i++) {

      // for each constraint
      CouenneConstraint *con = problem_ -> Con (i);

      // (which has an aux as its body)
      int index = con -> Body () -> Index ();

      if ((index >= 0) && (con -> Body () -> Type () == AUX)) {

	// get the auxiliary that is at the lhs
	exprAux *conaux = dynamic_cast <exprAux *> (problem_ -> Var (index));

	if (conaux &&
	    (conaux -> Image ()) && 
	    (conaux -> Image () -> Linearity () <= LINEAR)) {

	  // the auxiliary w of constraint w <= b is associated with a
	  // linear expression w = ax: add constraint ax <= b
	  conaux -> Image () -> generateCuts (conaux, si, cs, this, chg_bds, 
					      conaux -> Index (), 
					      (*(con -> Lb ())) (), 
					      (*(con -> Ub ())) ());

	  // take it from the list of the variables to be linearized
	  //conaux -> decreaseMult (); // !!!
	}

	// also, add constraint w <= b

	// if there exists violation, add constraint
	CouNumber l = con -> Lb () -> Value (),	
	          u = con -> Ub () -> Value ();

	// tighten bounds in Couenne's problem representation
	problem_ -> Lb (index) = CoinMax (l, problem_ -> Lb (index));
	problem_ -> Ub (index) = CoinMin (u, problem_ -> Ub (index));

      } else { // body is more than just a variable, but it should be
	       // linear. If so, generate equivalent linear cut

	assert (false);	// TODO
      }
    }

    if (jnlst_ -> ProduceOutput (J_DETAILED, J_CONVEXIFYING)) {
      if (cs.sizeRowCuts ()) {
	jnlst_ -> Printf (J_DETAILED, J_CONVEXIFYING,"Couenne: constraint row cuts\n");
	for (int i=0; i<cs.sizeRowCuts (); i++) 
	  cs.rowCutPtr (i) -> print ();
      }
      if (cs.sizeColCuts ()) {
	jnlst_ -> Printf (J_DETAILED, J_CONVEXIFYING,"Couenne: constraint col cuts\n");
	for (int i=0; i<cs.sizeColCuts (); i++) 
	  cs.colCutPtr (i) -> print ();
      }
    }
  } else updateBranchInfo (si, problem_, chg_bds, info); // info.depth >= 0 || info.pass >= 0

  fictitiousBound (cs, problem_, false);

  int *changed = NULL, nchanged;

  //////////////////////// Bound tightening ///////////////////////////////////////////

  // do bound tightening only at first pass of cutting plane in a node
  // of BB tree (info.pass == 0) or if first call (creation of RLT,
  // info.pass == -1)

  problem_ -> installCutOff ();

  try {

    if (problem_ -> doFBBT () &&
	(info.pass <= 0) &&
	(! (problem_ -> boundTightening (chg_bds, babInfo))))
      throw INFEASIBLE;

    // Reduced Cost BT
    if (problem_ -> doFBBT () && !firstcall_)
      problem_ -> redCostBT (&si, chg_bds, babInfo);

    //////////////////////// GENERATE CONVEXIFICATION CUTS //////////////////////////////

    sparse2dense (ncols, chg_bds, changed, nchanged);

    double *nlpSol;

    //--------------------------------------------

    if (babInfo && ((nlpSol = const_cast <double *> (babInfo -> nlpSolution ())))) {

      if (problem_ -> doABT () && (info.pass <= 0)) {
	if (! (problem_ -> aggressiveBT (chg_bds, babInfo)))
	  throw INFEASIBLE;

	sparse2dense (ncols, chg_bds, changed, nchanged);
      }

      // obtain solution just found by nlp solver

      // Auxiliaries should be correct. solution should be the one found
      // at the node even if it is not as good as the best known.

      // save violation flag and disregard it while adding cut at NLP
      // point (which are not violated by the current, NLP, solution)
      bool save_av = addviolated_;
      addviolated_ = false;

      // update problem current point with NLP solution
      problem_ -> update (nlpSol, NULL, NULL, problem_ -> nOrig ());
      genRowCuts (si, cs, nchanged, changed, info, chg_bds, true);  // add cuts

      // restore LP point
      problem_ -> update (si. getColSolution (), NULL, NULL);
      addviolated_ = save_av;     // restore previous value

      //    if (!firstcall_) // keep solution if called from extractLinearRelaxation()
      babInfo -> setHasNlpSolution (false); // reset it after use 
    }
    else genRowCuts (si, cs, nchanged, changed, info, chg_bds);

    //---------------------------------------------

    // change tightened bounds through OsiCuts
    if (nchanged)
      genColCuts (si, cs, nchanged, changed);

    // OBBT ////////////////////////////////////////////////////////////////////////////////

    int logObbtLev = problem_ -> logObbtLev ();

    // to be carried out if:
    if (problem_ -> doOBBT () &&        // relative flag is checked
	(logObbtLev != 0) &&            // relative frequency parameter is nonzero
	!firstcall_ &&                  // not first call (there is no LP to work with)
	(info.pass == 0) &&             // at first round of cuts
	((logObbtLev < 0) ||            // always if logObbtLev = -1
	 (info.level <= logObbtLev) ||  // at all levels up to the COU_OBBT_CUTOFF_LEVEL-th,
	 // and then with probability inversely proportional to the level
	 (CoinDrand48 () < pow (2., (double) logObbtLev - (info.level + 1))))) {

      CouenneSolverInterface *csi = dynamic_cast <CouenneSolverInterface *> (si.clone (true));

      csi -> setupForRepeatedUse ();

      int nImprov, nIter = 0;

      while ((nIter++ < MAX_OBBT_ITER) &&
	     ((nImprov = problem_ -> obbt (csi, cs, chg_bds, babInfo)) > 0)) 

	if (nImprov >= THRES_NBD_CHANGED) {

	  /// OBBT has given good results, add convexification with
	  /// improved bounds

	  sparse2dense (ncols, chg_bds, changed, nchanged);
	  genColCuts (*csi, cs, nchanged, changed);

	  int nCurCuts = cs.sizeRowCuts ();
	  genRowCuts (*csi, cs, nchanged, changed, info, chg_bds);

	  if (nCurCuts == cs.sizeRowCuts ()) 
	    break; // repeat only if new cuts available
	}

      delete csi;

      if (nImprov < 0)
	jnlst_->Printf(J_DETAILED, J_CONVEXIFYING,
		       "### infeasible node after OBBT\n");

      if (nImprov < 0)
	throw INFEASIBLE;
    }
  
    {
      int ncuts;
      if (firstcall_ && ((ncuts = cs.sizeRowCuts ()) >= 1))
	jnlst_->Printf(J_SUMMARY, J_CONVEXIFYING,
		       "Couenne: %d initial row cuts\n", ncuts);
    }
  }

  // end of OBBT //////////////////////////////////////////////////////////////////////

  catch (int exception) {

    if ((exception == INFEASIBLE) && (!firstcall_)) {

      OsiColCut *infeascut = new OsiColCut;

      if (infeascut) {
	int i=0;
	double upper = -1., lower = +1.;
	infeascut -> setLbs (1, &i, &lower);
	infeascut -> setUbs (1, &i, &upper);
	cs.insert (infeascut);
	delete infeascut;
      }
    }

    if (babInfo) // set infeasibility to true in order to skip NLP heuristic
      babInfo -> setInfeasibleNode ();
  }

  delete [] chg_bds;
  free (changed);

  if (firstcall_) {

    fictitiousBound (cs, problem_, true);
    firstcall_  = false;
    ntotalcuts_ = nrootcuts_ = cs.sizeRowCuts ();
  }
  else ntotalcuts_ += (cs.sizeRowCuts () - nInitCuts);

  septime_ += CoinCpuTime () - now;
}