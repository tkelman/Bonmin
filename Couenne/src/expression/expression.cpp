/*
 * Name:    expression.C
 * Author:  Pietro Belotti
 * Purpose: methods of the expression class
 *
 * This file is licensed under the Common Public License (CPL)
 */

#include <CouenneTypes.h>
#include <expression.h>
#include <exprAux.h>
#include <exprOp.h>
#include <exprUnary.h>
#include <exprVar.h>
#include <exprBound.h>

#include <CouenneProblem.h>


// static vectors for evaluation, see their description in
// expression.h

CouNumber  expression::stack [STACK_SIZE];
CouNumber *expression::sp = stack;

CouNumber *expression::variables_ = NULL;
CouNumber *expression::lbounds_   = NULL;
CouNumber *expression::ubounds_   = NULL;


// General N-ary function destructor

exprOp::~exprOp () {

  register expression *elem;

  if (arglist_) {
    for (register int i = nargs_; i--;)
      if ((elem = arglist_ [i]))
	delete elem;

    delete [] arglist_;
    arglist_ = NULL;
  }
}


// print expression

void exprOp::print (std::ostream &out = std::cout, const std::string &op = "unknown", 
		    enum pos pos = PRE) {

  if (pos == PRE)
    out << op;

  out << "("; 
  for (int i=0; i<nargs_; i++) {
    if (arglist_ [i])
      arglist_ [i] -> print (out); 

    if (i < nargs_ - 1) {
      if (pos == INSIDE) out << op;
      else               out << ",";
    }
  }
  out << ")";
}


// print unary expression

void exprUnary::print (std::ostream &out, const std::string &op = "unknown", 
		       enum pos pos = PRE) {

  if (pos == PRE)  out << op << " "; fflush (stdout);
  argument_ -> print (out);
  if (pos == POST) out << op; fflush (stdout);
}


// does this expression depend on variables in varlist?

bool exprOp::dependsOn (int *varlist = NULL, int n = 1) {

  for (register int i = nargs_; i--;)
    if (arglist_ [i] -> dependsOn (varlist, n))
      return true;

  return false;
}


// Get lower and upper bound of a generic expression

inline void expression::getBounds (expression *&lb, expression *&ub) {

  lb = new exprConst (- COUENNE_INFINITY);
  ub = new exprConst (  COUENNE_INFINITY);
}


// generate cuts for expression associated with this auxiliary

void exprAux::generateCuts (const OsiSolverInterface &si, 
			    OsiCuts &cs, const CouenneCutGenerator *cg)
{
  printf ("----------------Generating cut for "); 
  print (std::cout);  printf (" := ");
  image_ -> print (std::cout); printf("\n");
  int j = cs.sizeRowCuts ();
  image_ -> generateCuts (this, si, cs, cg);
  for (;j < cs.sizeRowCuts ();j++)
    cs.rowCutPtr (j) -> print ();
}


// generate one cut for a constant

void exprConst::generateCuts (exprAux *w, const OsiSolverInterface &si, 
			      OsiCuts &cs, const CouenneCutGenerator *cg) {

  OsiRowCut *cut = new OsiRowCut;

  CouNumber *coeff = new CouNumber [1];  
  int       *index = new int       [1];  

  cut -> setLb (currValue_);
  cut -> setUb (currValue_);

  coeff [0] = 1; index [0] = w -> Index ();

  cs.insert (cut);
}

