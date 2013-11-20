// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CbcHeuristicLocal_H
#define CbcHeuristicLocal_H

#include "CbcHeuristic.hpp"
/** LocalSearch class
 */

class CbcHeuristicLocal : public CbcHeuristic {
public:

  // Default Constructor 
  CbcHeuristicLocal ();

  /* Constructor with model - assumed before cuts
     Initial version does not do Lps
  */
  CbcHeuristicLocal (CbcModel & model);
  
  // Copy constructor 
  CbcHeuristicLocal ( const CbcHeuristicLocal &);
   
  // Destructor 
  ~CbcHeuristicLocal ();
  
  /// Clone
  virtual CbcHeuristic * clone() const;

  /// Assignment operator 
  CbcHeuristicLocal & operator=(const CbcHeuristicLocal& rhs);

  /// Create C++ lines to get to current state
  virtual void generateCpp( FILE * fp) ;

  /// Resets stuff if model changes
  virtual void resetModel(CbcModel * model);

  /// update model (This is needed if cliques update matrix etc)
  virtual void setModel(CbcModel * model);
  
  using CbcHeuristic::solution ;
  /** returns 0 if no solution, 1 if valid solution.
      Sets solution values if good, sets objective value (only if good)
      This is called after cuts have been added - so can not add cuts
      First tries setting a variable to better value.  If feasible then
      tries setting others.  If not feasible then tries swaps

      ********

      This first version does not do LP's and does swaps of two integer 
      variables.  Later versions could do Lps.
  */
  virtual int solution(double & objectiveValue,
		       double * newSolution);
  /// This version fixes stuff and does IP
  int solutionFix(double & objectiveValue,
		  double * newSolution,
		  const int * keep);

  /// Sets type of search
  inline void setSearchType(int value)
  { swap_=value;}
  /// Used array so we can set
  inline char * used() const
  { return used_;}

protected:
  // Data

  // Original matrix by column
  CoinPackedMatrix matrix_;

  // Number of solutions so we only do after new solution
  int numberSolutions_;
  // Type of search 0=normal, 1=BAB
  int swap_;
  /// Whether a variable has been in a solution
  char * used_;
};


/** Naive class
    a) Fix all ints as close to zero as possible
    b) Fix all ints with nonzero costs and < large to zero
    c) Put bounds round continuous and UIs and maximize
 */

class CbcHeuristicNaive : public CbcHeuristic {
public:

  // Default Constructor 
  CbcHeuristicNaive ();

  /* Constructor with model - assumed before cuts
     Initial version does not do Lps
  */
  CbcHeuristicNaive (CbcModel & model);
  
  // Copy constructor 
  CbcHeuristicNaive ( const CbcHeuristicNaive &);
   
  // Destructor 
  ~CbcHeuristicNaive ();
  
  /// Clone
  virtual CbcHeuristic * clone() const;

  /// Assignment operator 
  CbcHeuristicNaive & operator=(const CbcHeuristicNaive& rhs);

  /// Create C++ lines to get to current state
  virtual void generateCpp( FILE * fp) ;

  /// Resets stuff if model changes
  virtual void resetModel(CbcModel * model);

  /// update model (This is needed if cliques update matrix etc)
  virtual void setModel(CbcModel * model);
  
  using CbcHeuristic::solution ;
  /** returns 0 if no solution, 1 if valid solution.
      Sets solution values if good, sets objective value (only if good)
  */
  virtual int solution(double & objectiveValue,
		       double * newSolution);

  /// Sets large cost value
  inline void setLargeValue(double value)
  { large_=value;}
  /// Gets large cost value
  inline double largeValue() const
  { return large_;}

protected:
  /// Data
  /// Large value
  double large_;
};


#endif
