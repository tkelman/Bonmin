// (C) Copyright International Business Machines Corporation (IBM) 2006, 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// Authors :
// Pietro Belloti, Carnegie Mellon University
// Pierre Bonami, International Business Machines Corporation
//
// Date : 12/19/2006


#include "BonCouenneInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include <CouenneProblem.h>

namespace Bonmin {

/** Default constructor. */
CouenneInterface::CouenneInterface():
  AmplInterface(),
  couenneCg_(NULL)
{}

/** Copy constructor. */
CouenneInterface::CouenneInterface(const CouenneInterface &other):
  AmplInterface(other),
  couenneCg_(NULL)
  {
  }

/** virutal copy constructor. */
CouenneInterface * CouenneInterface::clone(bool CopyData){
  return new CouenneInterface(*this);
}

/** Destructor. */
CouenneInterface::~CouenneInterface(){
  if(couenneCg_) delete couenneCg_;
}

void 
CouenneInterface::readAmplNlFile(char **& amplArgs, Bonmin::BasicSetup & b){
  readAmplNlFile(amplArgs, b.journalist(), b.options(), b.roptions());
}


void 
CouenneInterface::readAmplNlFile(char **& argv, Ipopt::SmartPtr<Ipopt::Journalist> journalist,
                         Ipopt::SmartPtr<Ipopt::OptionsList> options,
                                 Ipopt::SmartPtr<Ipopt::RegisteredOptions> roptions){
  AmplInterface::readAmplNlFile(argv, journalist, options, roptions);
  aslfg_ = readASLfg (argv);
  int addOnlyViolatedOa;
  options->GetEnumValue("add_only_violated_oa", addOnlyViolatedOa,"bonmin.");
  couenneCg_ = new CouenneCutGenerator 
    (this, aslfg_, true, CURRENT_ONLY,1);
}

/** \name Overloaded methods to build outer approximations */
  //@{
  /** \brief Extract a linear relaxation of the MINLP.
   *
   * Solve the continuous relaxation and takes first-order
   * outer-approximation constraints at the optimum.  Then put
   * everything in an OsiSolverInterface.
   *
   * The OsiSolverInterface si is empty and has to be populated with
   * the initial linear relaxation.
   */

void 
CouenneInterface::extractLinearRelaxation (OsiSolverInterface &si, bool getObj, bool solveNlp) {

  if (solveNlp)
    initialSolve ();

   // Check that couenneCg_ has been built. 
   if (couenneCg_ == NULL)
     throw CoinError 
       ("No couenne generator has been built, probably ampl .nl file was not properly read",
	"extractLinearRelaxation", "Bonmin::CouenneInterface");

   int numcols     = getNumCols ();             // number of original variables
   int numcolsconv = couenneCg_ -> getnvars (); // number of original+auxiliary variables

   const double *lb = getColLower ();
   const double *ub = getColUpper ();

   // add original variables to the new problem
   for (register int i=0; i<numcols; i++)
     si.addCol (0, NULL, NULL, lb [i], ub [i], 0);

   // get initial relaxation
   OsiCuts cs;
   couenneCg_ -> generateCuts (si, cs);

   // store all (original + auxiliary) bounds in the relaxation
   CouNumber * colLower = new CouNumber [numcolsconv];
   CouNumber * colUpper = new CouNumber [numcolsconv];

   CouNumber *ll = couenneCg_ -> Problem () -> Lb ();
   CouNumber *uu = couenneCg_ -> Problem () -> Ub ();

   // overwrite original bounds, could be improved within generateCuts
   for (register int i=numcolsconv; i--;) {
     colLower [i] = ll [i];
     colUpper [i] = uu [i];
   }

   int numrowsconv = cs.sizeRowCuts ();

   // create matrix and other stuff
   CoinBigIndex * start = new CoinBigIndex [numrowsconv + 1];

   int    * length   = new int    [numrowsconv];
   double * rowLower = new double [numrowsconv];
   double * rowUpper = new double [numrowsconv];

   start[0] = 0;

   /* fill the four arrays. */
   for(int i = 0 ; i < numrowsconv ; i++)
   {
     OsiRowCut * cut = cs.rowCutPtr (i);

     const CoinPackedVector &v = cut->row();
     start[i+1] = start[i] + v.getNumElements();

     length[i] = v.getNumElements();

     rowLower[i] = cut->lb();
     rowUpper[i] = cut->ub();
   }
   
   /* Now fill the elements arrays. */
   int * ind = new int[start[numrowsconv]];
   double * elem = new double[start[numrowsconv]];
   for(int i = 0 ; i < numrowsconv ; i++) {

     OsiRowCut * cut = cs.rowCutPtr (i);

     const CoinPackedVector &v = cut->row();

     if(v.getNumElements() != length[i])
       std::cout<<"Empty row"<<std::endl;

     CoinCopyN(v.getIndices(), length[i], ind + start[i]);
     CoinCopyN(v.getElements(), length[i], elem + start[i]);
   }

   // Ok everything done now create interface
   CoinPackedMatrix A;
   A.assignMatrix(false, numcolsconv, numrowsconv,
                  start[numrowsconv], elem, ind,
                  start, length);

   // Objective function
   double * obj = new double[numcolsconv];
   CoinFillN(obj,numcolsconv,0.);

   // some instances have no (or null) objective function, check it here
   if (couenneCg_ -> Problem () -> nObjs () > 0)
     obj [couenneCg_ -> Problem () -> Obj (0) -> Body () -> Index ()] = 1;

   // Finally, load interface si with the initial LP relaxation
   si.loadProblem (A, colLower, colUpper, obj, rowLower, rowUpper);
  
   delete [] rowLower; 
   delete [] rowUpper;
   delete [] colLower;
   delete [] colUpper;
   delete [] obj;
   //   delete [] x0;

   for(int i = 0 ; i < numcols ; i++)
   {
     if(isInteger(i)){
       si.setInteger(i);
     }
   }
 
   //si.writeMpsNative("toto",NULL,NULL,1);
   si.writeLp("toto");
  app_->enableWarmStart();
  setColSolution(problem()->x_sol());
  setRowPrice(problem()->duals_sol());
}

/** Get the outer approximation constraints at the currently stored optimal point.
   (Only get outer-approximations of nonlinear constraints of the problem.)*/
/*
void 
CouenneInterface::getOuterApproximation(OsiCuts &cs, bool getObj, 
					const double * x2, bool global){

  
   int numcols = getNumCols();
   int numcolsconv = couenneCg_->getnvars();

   CouNumber * x0 = new CouNumber[numcolsconv];
   CouNumber * colLower = new CouNumber[numcolsconv];
   CouNumber * colUpper = new CouNumber[numcolsconv];
   assert(numcolsconv >= numcols);
   
   CoinCopyN(couenneCg_->X(), numcolsconv, x0);
   CoinCopyN(couenneCg_->Lb(), numcolsconv, colLower);
   CoinCopyN(couenneCg_->Ub(), numcolsconv, colUpper);

   // Feed in the initial relaxation point
   CoinCopyN(getColSolution(), numcols, x0);
   couenneCg_ -> updateAuxs (x0,colLower, colUpper);
   getOuterApproximation(cs, x0, getObj, x2, global);

   delete [] colLower;
   delete [] colUpper;
   delete [] x0;

}
*/
/*
void 
CouenneInterface::getOuterApproximation(OsiCuts &cs, const double * x, bool getObj, const double * x2, bool global){
   // Check that couenneCg_ has been built. 

   if(couenneCg_ == NULL){
     throw CoinError("No couenne generator has been built,"
                     " probably ampl .nl file was not properly"
                     " read",
                     "extractLinearRelaxation", 
                     "Bonmin::CouenneInterface");
   }
   int numcols = getNumCols();
   int numcolsconv = couenneCg_->getnvars();
  
   CouNumber * x0 = new CouNumber[numcolsconv];
   CouNumber * colLower = new CouNumber[numcolsconv];
   CouNumber * colUpper = new CouNumber[numcolsconv];
   assert(numcolsconv >= numcols);
   
   // Feed in the current state
   CoinCopyN(x, numcolsconv, x0);
   CoinCopyN(getColLower(), numcolsconv, colLower);
   CoinCopyN(getColUpper(), numcolsconv, colUpper);

   couenneCg_->updateConv(x0, colLower, colUpper);


   int ncuts = couenneCg_->getncuts();
   for(int i = 0 ; i < ncuts ; i++)
   {
     cs.insert(*couenneCg_->getCut(i)); 
   }
   
   delete [] colLower;
   delete [] colUpper;
   delete [] x0;
}
*/


/** To set some application specific defaults. */
void CouenneInterface::setAppDefaultOptions(Ipopt::SmartPtr<Ipopt::OptionsList> Options){
  Options->SetStringValue("bonmin.algorithm", "B-Couenne", true, true);
  Options->SetIntegerValue("bonmin.filmint_ecp_cuts", 1, true, true);
}
} /** End Bonmin namespace. */
