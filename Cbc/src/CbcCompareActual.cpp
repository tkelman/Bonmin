// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cassert>
#include <cmath>
#include <cfloat>
//#define CBC_DEBUG

#include "CbcMessage.hpp"
#include "CbcModel.hpp"
#include "CbcTree.hpp"
#include "CbcCompareActual.hpp"
#include "CoinError.hpp"


/** Default Constructor

*/
CbcCompareDepth::CbcCompareDepth ()
  : CbcCompareBase()
{
  test_=this;
}

// Copy constructor 
CbcCompareDepth::CbcCompareDepth ( const CbcCompareDepth & rhs)
  :CbcCompareBase(rhs)

{
}

// Clone
CbcCompareBase *
CbcCompareDepth::clone() const
{
  return new CbcCompareDepth(*this);
}

// Assignment operator 
CbcCompareDepth & 
CbcCompareDepth::operator=( const CbcCompareDepth& rhs)
{
  if (this!=&rhs) {
    CbcCompareBase::operator=(rhs);
  }
  return *this;
}

// Destructor 
CbcCompareDepth::~CbcCompareDepth ()
{
}

// Returns true if y better than x
bool 
CbcCompareDepth::test (CbcNode * x, CbcNode * y)
{
  return x->depth() < y->depth();
}
// Create C++ lines to get to current state
void
CbcCompareDepth::generateCpp( FILE * fp) 
{
  fprintf(fp,"0#include \"CbcCompareActual.hpp\"\n");
  fprintf(fp,"3  CbcCompareDepth compare;\n");
  fprintf(fp,"3  cbcModel->setNodeComparison(compare);\n");
}

/** Default Constructor

*/
CbcCompareObjective::CbcCompareObjective ()
  : CbcCompareBase()
{
  test_=this;
}

// Copy constructor 
CbcCompareObjective::CbcCompareObjective ( const CbcCompareObjective & rhs)
  :CbcCompareBase(rhs)

{
}

// Clone
CbcCompareBase *
CbcCompareObjective::clone() const
{
  return new CbcCompareObjective(*this);
}

// Assignment operator 
CbcCompareObjective & 
CbcCompareObjective::operator=( const CbcCompareObjective& rhs)
{
  if (this!=&rhs) {
    CbcCompareBase::operator=(rhs);
  }
  return *this;
}

// Destructor 
CbcCompareObjective::~CbcCompareObjective ()
{
}

// Returns true if y better than x
bool 
CbcCompareObjective::test (CbcNode * x, CbcNode * y)
{
  return x->objectiveValue() > y->objectiveValue();
}
// Create C++ lines to get to current state
void
CbcCompareObjective::generateCpp( FILE * fp) 
{
  fprintf(fp,"0#include \"CbcCompareActual.hpp\"\n");
  fprintf(fp,"3  CbcCompareObjective compare;\n");
  fprintf(fp,"3  cbcModel->setNodeComparison(compare);\n");
}


/** Default Constructor

*/
CbcCompareDefault::CbcCompareDefault ()
  : CbcCompareBase(),
    weight_(-1.0),
    saveWeight_(0.0),
    numberSolutions_(0),
    treeSize_(0)
{
  test_=this;
}

// Constructor with weight
CbcCompareDefault::CbcCompareDefault (double weight) 
  : CbcCompareBase(),
    weight_(weight) ,
    saveWeight_(0.0),
    numberSolutions_(0),
    treeSize_(0)
{
  test_=this;
}


// Copy constructor 
CbcCompareDefault::CbcCompareDefault ( const CbcCompareDefault & rhs)
  :CbcCompareBase(rhs)

{
  weight_=rhs.weight_;
  saveWeight_ = rhs.saveWeight_;
  numberSolutions_=rhs.numberSolutions_;
  treeSize_ = rhs.treeSize_;
}

// Clone
CbcCompareBase *
CbcCompareDefault::clone() const
{
  return new CbcCompareDefault(*this);
}

// Assignment operator 
CbcCompareDefault & 
CbcCompareDefault::operator=( const CbcCompareDefault& rhs)
{
  if (this!=&rhs) {
    CbcCompareBase::operator=(rhs);
    weight_=rhs.weight_;
    saveWeight_ = rhs.saveWeight_;
    numberSolutions_=rhs.numberSolutions_;
    treeSize_ = rhs.treeSize_;
  }
  return *this;
}

// Destructor 
CbcCompareDefault::~CbcCompareDefault ()
{
}

// Returns true if y better than x
bool 
CbcCompareDefault::test (CbcNode * x, CbcNode * y)
{
#if 0
  // was
  if (weight_<0.0||treeSize_>100000) {
    // before solution
    /* printf("x %d %d %g, y %d %d %g\n",
       x->numberUnsatisfied(),x->depth(),x->objectiveValue(),
       y->numberUnsatisfied(),y->depth(),y->objectiveValue()); */
    if (x->numberUnsatisfied() > y->numberUnsatisfied())
      return true;
    else if (x->numberUnsatisfied() < y->numberUnsatisfied())
      return false;
    else
      return x->depth() < y->depth();
  } else {
    // after solution
    return x->objectiveValue()+ weight_*x->numberUnsatisfied() > 
      y->objectiveValue() + weight_*y->numberUnsatisfied();
  }
#else
  if (weight_==-1.0&&(y->depth()>7||x->depth()>7)) {
    // before solution
    /* printf("x %d %d %g, y %d %d %g\n",
       x->numberUnsatisfied(),x->depth(),x->objectiveValue(),
       y->numberUnsatisfied(),y->depth(),y->objectiveValue()); */
    if (x->numberUnsatisfied() > y->numberUnsatisfied())
      return true;
    else if (x->numberUnsatisfied() < y->numberUnsatisfied())
      return false;
    else
      return x->depth() < y->depth();
  } else {
    // after solution
    double weight = CoinMax(weight_,0.0);
    return x->objectiveValue()+ weight*x->numberUnsatisfied() > 
      y->objectiveValue() + weight*y->numberUnsatisfied();
  }
#endif
}
// This allows method to change behavior as it is called
// after each solution
void 
CbcCompareDefault::newSolution(CbcModel * model,
			       double objectiveAtContinuous,
			       int numberInfeasibilitiesAtContinuous) 
{
  if (model->getSolutionCount()==model->getNumberHeuristicSolutions()&&
      model->getSolutionCount()<5&&model->getNodeCount()<500)
    return; // solution was got by rounding
  // set to get close to this solution
  double costPerInteger = 
    (model->getObjValue()-objectiveAtContinuous)/
    ((double) numberInfeasibilitiesAtContinuous);
  weight_ = 0.95*costPerInteger;
  saveWeight_ = 0.95*weight_;
  numberSolutions_++;
  if (numberSolutions_>5)
    weight_ =0.0; // this searches on objective
}
// This allows method to change behavior 
bool 
CbcCompareDefault::every1000Nodes(CbcModel * model, int numberNodes)
{
#if 0
  // was
  if (numberNodes>10000)
    weight_ =0.0; // this searches on objective
  // get size of tree
  treeSize_ = model->tree()->size();
#else
  double saveWeight=weight_;
  int numberNodes1000 = numberNodes/1000;
  if (numberNodes>10000) {
    weight_ =0.0; // this searches on objective
    // but try a bit of other stuff
    if ((numberNodes1000%4)==1)
      weight_=saveWeight_;
  } else if (numberNodes==1000&&weight_==-2.0) {
    weight_=-1.0; // Go to depth first
  }
  // get size of tree
  treeSize_ = model->tree()->size();
  if (treeSize_>10000) {
    int n1 = model->solver()->getNumRows()+model->solver()->getNumCols();
    int n2 = model->numberObjects();
    double size = n1*0.1 + n2*2.0;
    // set weight to reduce size most of time
    if (treeSize_*size>5.0e7)
      weight_=-1.0;
    else if ((numberNodes1000%4)==0&&treeSize_*size>1.0e6)
      weight_=-1.0;
    else if ((numberNodes1000%4)==1)
      weight_=0.0;
    else
      weight_=saveWeight_;
  }
#endif
  //return numberNodes==11000; // resort if first time
  return (weight_!=saveWeight);
}

// Create C++ lines to get to current state
void
CbcCompareDefault::generateCpp( FILE * fp) 
{
  CbcCompareDefault other;
  fprintf(fp,"0#include \"CbcCompareActual.hpp\"\n");
  fprintf(fp,"3  CbcCompareDefault compare;\n");
  if (weight_!=other.weight_)
    fprintf(fp,"3  compare.setWeight(%g);\n",weight_);
  fprintf(fp,"3  cbcModel->setNodeComparison(compare);\n");
}

/** Default Constructor

*/
CbcCompareEstimate::CbcCompareEstimate ()
  : CbcCompareBase()
{
  test_=this;
}

// Copy constructor 
CbcCompareEstimate::CbcCompareEstimate ( const CbcCompareEstimate & rhs)
  :CbcCompareBase(rhs)

{
}

// Clone
CbcCompareBase *
CbcCompareEstimate::clone() const
{
  return new CbcCompareEstimate(*this);
}

// Assignment operator 
CbcCompareEstimate & 
CbcCompareEstimate::operator=( const CbcCompareEstimate& rhs)
{
  if (this!=&rhs) {
    CbcCompareBase::operator=(rhs);
  }
  return *this;
}

// Destructor 
CbcCompareEstimate::~CbcCompareEstimate ()
{
}

// Returns true if y better than x
bool 
CbcCompareEstimate::test (CbcNode * x, CbcNode * y)
{
  return x->guessedObjectiveValue() >  y->guessedObjectiveValue() ;
}

// Create C++ lines to get to current state
void
CbcCompareEstimate::generateCpp( FILE * fp) 
{
  fprintf(fp,"0#include \"CbcCompareActual.hpp\"\n");
  fprintf(fp,"3  CbcCompareEstimate compare;\n");
  fprintf(fp,"3  cbcModel->setNodeComparison(compare);\n");
}

