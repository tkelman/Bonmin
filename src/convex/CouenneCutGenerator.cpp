/*
 * Name: CouenneCglCutGenerator.cpp
 * Author: Pietro Belotti
 * Purpose: define a class of convexification procedures 
 *
 * (C) Carnegie-Mellon University, 2006-07. 
 * This file is licensed under the Common Public License (CPL)
 */


#include "OsiRowCut.hpp"
#include "BonOaDecBase.hpp"
#include "CglCutGenerator.hpp"

#include "CouennePrecisions.hpp"
#include "CouenneProblem.hpp"
#include "CouenneCutGenerator.hpp"


/// constructor
CouenneCutGenerator::CouenneCutGenerator (Bonmin::OsiTMINLPInterface *nlp,
					  Bonmin::BabSetupBase *base,
					  const struct ASL *asl, 
					  JnlstPtr jnlst):

  OaDecompositionBase (nlp, NULL, NULL, 0,0,0),

  firstcall_      (true),
  problem_        (NULL),
  nrootcuts_      (0),
  ntotalcuts_     (0),
  septime_        (0),
  objValue_       (- DBL_MAX),
  nlp_            (nlp),
  BabPtr_         (NULL),
  infeasNode_     (false),
  jnlst_          (jnlst) {

  base -> options () -> GetIntegerValue ("convexification_points", nSamples_, "couenne.");
  base -> options () -> GetIntegerValue ("log_num_obbt_per_level", logObbtLev_, "couenne.");

  std::string s;

  base -> options () -> GetStringValue ("convexification_type", s, "couenne.");
  if      (s == "current-point-only") convtype_ = CURRENT_ONLY;
  else if (s == "uniform-grid")       convtype_ = UNIFORM_GRID;
  else                                convtype_ = AROUND_CURPOINT;

  base -> options () -> GetStringValue ("feasibility_bt",  s, "couenne."); doFBBT_ = (s == "yes");
  base -> options () -> GetStringValue ("optimality_bt",   s, "couenne."); doOBBT_ = (s == "yes");
  base -> options () -> GetStringValue ("aggressive_fbbt", s, "couenne."); doABT_  = (s == "yes");

  base -> options () -> GetStringValue ("violated_cuts_only", s, "couenne."); 
  addviolated_ = (s == "yes");

  problem_ = new CouenneProblem (asl, jnlst_);
}


/// destructor
CouenneCutGenerator::~CouenneCutGenerator ()
  {delete problem_;}


/// copy constructor
CouenneCutGenerator::CouenneCutGenerator (const CouenneCutGenerator &src):

  OaDecompositionBase (src),

  firstcall_   (src. firstcall_),
  addviolated_ (src. addviolated_), 
  convtype_    (src. convtype_), 
  nSamples_    (src. nSamples_),
  problem_     (src. problem_ -> clone ()),
  nrootcuts_   (src. nrootcuts_),
  ntotalcuts_  (src. ntotalcuts_),
  septime_     (src. septime_),
  objValue_    (src. objValue_),
  nlp_         (src. nlp_),
  BabPtr_      (src. BabPtr_),
  infeasNode_  (src. infeasNode_),
  doFBBT_      (src. doFBBT_),
  doOBBT_      (src. doOBBT_),
  doABT_       (src. doABT_),
  logObbtLev_  (src. logObbtLev_),
  jnlst_       (src. jnlst_)  {}


#define MAX_SLOPE 1e3

/// add half-space through two points (x1,y1) and (x2,y2)
int CouenneCutGenerator::addSegment (OsiCuts &cs, int wi, int xi, 
				     CouNumber x1, CouNumber y1, 
				     CouNumber x2, CouNumber y2, int sign) const { 

  if (fabs (x2-x1) < COUENNE_EPS) {
    if (fabs (y2-y1) > MAX_SLOPE * COUENNE_EPS)
      jnlst_->Printf(J_WARNING, J_CONVEXIFYING,
		     "warning, discontinuity of %e over an interval of %e\n", y2-y1, x2-x1);
    else return createCut (cs, y2, (int) 0, wi, 1.);
  }

  CouNumber dx = x2-x1, dy = y2-y1;

  //  return createCut (cs, y1 + oppslope * x1, sign, wi, 1., xi, oppslope);
  return createCut (cs, y1*dx - dy*x1, (dx>0) ? sign : -sign, wi, dx, xi, -dy);
}


/// add tangent at (x,w) with given slope
int CouenneCutGenerator::addTangent (OsiCuts &cs, int wi, int xi, 
				     CouNumber x, CouNumber w, 
				     CouNumber slope, int sign) const
  {return createCut (cs, w - slope * x, sign, wi, 1., xi, - slope);}


/// total number of variables (original + auxiliary) of the problem
const int CouenneCutGenerator::getnvars () const
  {return problem_ -> nVars ();} 


/// Add list of options to be read from file
void CouenneCutGenerator::registerOptions (Ipopt::SmartPtr <Bonmin::RegisteredOptions> roptions) {

  roptions -> SetRegisteringCategory ("Couenne options", Bonmin::RegisteredOptions::CouenneCategory);

  roptions -> AddLowerBoundedIntegerOption
    ("convexification_cuts",
     "Specify the frequency (in terms of nodes) at which couenne ecp cuts are generated.",
     0,1,
     "A frequency of 0 amounts to never solve the NLP relaxation.");
    
  roptions -> AddStringOption2
    ("local_optimization_heuristic",
     "Do we search for local solutions of NLP's",
     "yes",
     "no","",
     "yes","");
    
  roptions -> AddLowerBoundedIntegerOption
    ("log_num_local_optimization_per_level",
     "Specify the logarithm of the number of local optimizations to perform" 
     " on average for each level of given depth of the tree.",
     -1,-1,"Solve as many nlp's at the nodes for each level of the tree. "
     "Nodes are randomly selected. If for a"
     "given level there are less nodes than this number nlp are solved for every nodes."
     "For example if parameter is 8, nlp's are solved for all node until level 8," 
     "then for half the node at level 9, 1/4 at level 10...."
     "Value -1 specify to perform at all nodes.");

  roptions -> AddStringOption3
    ("convexification_type",
     "Deterimnes in which point the linear over/under-estimator are generated",
     "current-point-only",
     "current-point-only","Only at current optimum of relaxation",
     "uniform-grid","Points chosen in a unform grid between the bounds of the problem",
     "around-current-point","At points around current optimum of relaxation");
    
  roptions -> AddLowerBoundedIntegerOption
    ("convexification_points",
     "Specify the number of points at which to convexify when convexification type"
     "is uniform-grid or arround-current-point.",
     0,1,
     "");

  roptions -> AddStringOption2 
    ("feasibility_bt",
     "Feasibility-based (cheap) bound tightening",
     "yes",
     "no","",
     "yes","");

  roptions -> AddStringOption2 
    ("optimality_bt",
     "optimality-based (expensive) bound tightening",
     "no",
     "no","",
     "yes","");

  roptions -> AddLowerBoundedIntegerOption
    ("log_num_obbt_per_level",
     "Specify the frequency (in terms of nodes) for optimality-based bound tightening.",
     -1,5,
     "If -1, apply at every node (expensive!). If 0, never apply.");

  roptions -> AddStringOption2 
    ("aggressive_fbbt",
     "Aggressive feasibility-based bound tightening (to use with NLP points)",
     "yes",
     "no","",
     "yes","");

  roptions -> AddStringOption3
    ("branch_pt_select",
     "Chooses branching point selection strategy",
     "mid-point",
     "balanced", "minimizes max distance from curve to convexification",
     "min-area", "minimizes total area of the two convexifications",
     "mid-point", "convex combination of current point and mid point",
     "");

  roptions -> AddStringOption2 
    ("violated_cuts_only",
     "Yes if only violated convexification cuts should be added",
     "yes",
     "no","",
     "yes","");


  roptions -> setOptionExtraInfo ("branch_pt_select", 15); // Why 15? TODO
}