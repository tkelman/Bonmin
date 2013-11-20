// Name:     CglRedSplit2Param.hpp
// Author:   Giacomo Nannicini
//           Singapore University of Technology and Design
//           Singapore
//           email: nannicini@sutd.edu.sg
// Date:     03/09/09
//-----------------------------------------------------------------------------
// Copyright (C) 2010, Giacomo Nannicini and others.  All Rights Reserved.

#ifndef CglRedSplit2Param_H
#define CglRedSplit2Param_H

#include "CglParam.hpp"
#include <vector>

  /**@name CglRedSplit2 Parameters */
  //@{

  /** Class collecting parameters the Reduced-and-split cut generator.

      An important thing to note is that the cut generator allows for
      the selection of a number of strategies that can be combined
      together. By default, a selection that typically yields a good
      compromise between speed and cut strenght is made. The selection
      can be changed by resetting the default choices (see the
      functions whose name starts with "reset") or by setting the
      parameter use_default_strategies to false in the
      constructors. After this, the chosen strategies can be added to
      the list by using the functions whose name starts with
      "add". All strategies will be combined together: if we choose 3
      row selection strategies, 2 column selection strategies, and 2
      possible numbers of rows, we end up with a total of 3*2*2
      combinations.

      For a detailed explanation of the parameters and their meaning,
      see the paper by Cornuejols and Nannicini: "Practical strategies
      for generating rank-1 split cuts in mixed-integer linear
      programming", on Mathematical Programming Computation.

      Parameters of the generator are listed below. 

      - MAXDYN: Maximum ratio between largest and smallest non zero 
                coefficients in a cut. See method setMAXDYN().
      - EPS_ELIM: Precision for deciding if a coefficient is zero when 
                  eliminating slack variables. See method setEPS_ELIM().
      - MINVIOL: Minimum violation for the current basic solution in 
                 a generated cut. See method setMINVIOL().
      - EPS_RELAX_ABS: Absolute relaxation of cut rhs.
      - EPS_RELAX_REL: Relative relaxation of cut rhs.
      - MAX_SUPP_ABS: Maximum cut support (absolute).
      - MAX_SUPP_REL: Maximum cut support (relative): the formula to 
                      compute maximum cut support is 
		      MAX_SUPP_ABS + ncol*MAX_SUPP_REL.
      - USE_INTSLACKS: Use integer slacks to generate cuts. (not implemented).
                       See method setUSE_INTSLACKS().
      - normIsZero: Norm of a vector is considered zero if smaller than
                    this value. See method setNormIsZero(). 
      - minNormReduction: a cut is generated if the new norm of the row on the
                          continuous nonbasics is reduced by at least
			  this factor (relative reduction).
      - away: Look only at basic integer variables whose current value
              is at least this value from being integer. See method setAway().
      - maxSumMultipliers: maximum sum (in absolute value) of row multipliers
      - normalization: normalization factor for the norm of lambda in the
                       coefficient reduction algorithm (convex min problem)
      - numRowsReduction: Maximum number of rows in the linear system for
                          norm reduction.
      - columnSelectionStrategy: parameter to select which columns should be
                                 used for coefficient reduction.
      - rowSelectionStrategy: parameter to select which rows should be
                              used for coefficient reduction.
      - timeLimit: Time limit (in seconds) for cut generation.
      - maxNumCuts: Maximum number of cuts that can be returned at each pass;
                    we could generate more cuts than this number (see below)
      - maxNumComputedCuts: Maximum number of cuts that can be computed
                            by the generator at each pass
      - maxNonzeroesTab : Rows of the simplex tableau with more than
                          this number of nonzeroes will not be
                          considered for reduction. Only works if
                          RS_FAST_* are defined in CglRedSplit2.
      - skipGomory: Skip traditional Gomory cuts, i.e. GMI cuts arising from
                    a single row of the tableau (instead of a combination).
                    Default is 1 (true), because we assume that they are 
		    generated by a traditional Gomory generator anyway. 
  */
  //@}

class CglRedSplit2Param : public CglParam {

public:
  /** Enumerations for parameters */

  /** Row selection strategies; same names as in the paper */
  enum RowSelectionStrategy{
    /* Pick rows that introduce the fewest nonzeroes on integer nonbasics */
    RS1, 
    /* Pick rows that introduce the fewest nonzeroes on the set of working
       continuous nonbasics */
    RS2,
    /* Pick rows that introduce the fewest nonzeroes on both integer and
       working continuous nonbasics */
    RS3,
    /* Same as RS0 but with greedy algorithm */
    RS4,
    /* Same as RS1 but with greedy algorithm */
    RS5,
    /* Same as RS2 but with greedy algorithm */
    RS6,
    /* Pick rows with smallest angle in the space of integer and working
       continuous nonbasics */
    RS7,
    /* Pick rows with smallest angle in the space of working
       continuous nonbasics */
    RS8,
    /* Use all strategies */
    RS_ALL,
    /* Use best ones - that is, RS8 and RS7 */
    RS_BEST
  };

  /** Column selection strategies; again, look them up in the paper. */
  enum ColumnSelectionStrategy{
    /* C-3P */
    CS1, CS2, CS3,
    /* C-5P */
    CS4, CS5, CS6, CS7, CS8,
    /* I-2P-2/3 */
    CS9, CS10,
    /* I-2P-4/5 */
    CS11, CS12,
    /* I-2P-1/2 */
    CS13, CS14,
    /* I-3P */
    CS15, CS16, CS17,
    /* I-4P */
    CS18, CS19, CS20, CS21,
    /* Use all strategies up to this point */
    CS_ALL,
    /* Use best strategies (same effect as CS_ALL, because it turns out that
       using all strategies is the best thing to do) */
    CS_BEST,
    /* Optimize over all continuous nonbasic columns; this does not give
       good results, but we use it for testing Lift & Project + RedSplit */
    CS_ALLCONT,
    /* Lift & Project specific strategy: only select variables which
       are nonbasic in the tableau but are basic in the point to cut
       off.  This strategy cannot be used outside L&P. It is not very
       effective even with L&P, but is left here for testing.*/
    CS_LAP_NONBASICS
  };

  /** Scaling strategies for new nonbasic columns for Lift & Project;
   *  "factor" is the value of columnScalingBoundLAP_ */
  enum ColumnScalingStrategy{
    /* No scaling */
    SC_NONE,
    /* Multiply by |xbar[i]| where xbar[i] is the value of the 
       corresponding component of the point that we want to cut off */
    SC_LINEAR,
    /* Multiply by min(factor,|xbar[i]|) */
    SC_LINEAR_BOUNDED,
    /* Multiply by min(factor,log(|xbar[i]|)) */
    SC_LOG_BOUNDED,
    /* Multiply all new nonbasics by factor */
    SC_UNIFORM,
    /* Multiply only nonzero coefficients by factor */
    SC_UNIFORM_NZ
  };

  /**@name Set/get methods */
  //@{
  /** Set away, the minimum distance from being integer used for selecting 
      rows for cut generation;  all rows whose pivot variable should be 
      integer but is more than away from integrality will be selected; 
      Default: 0.005 */
  virtual void setAway(double value);
  /// Get value of away
  inline double getAway() const {return away_;}

  /** Set the value of EPS_ELIM, epsilon for values of coefficients when 
      eliminating slack variables;
      Default: 0.0 */
  void setEPS_ELIM(double value);
  /** Get the value of EPS_ELIM */
  double getEPS_ELIM() const {return EPS_ELIM;}
  
  /** Set EPS_RELAX_ABS */
  virtual void setEPS_RELAX_ABS(double eps_ra);
  /** Get value of EPS_RELAX_ABS */
  inline double getEPS_RELAX_ABS() const {return EPS_RELAX_ABS;}

  /** Set EPS_RELAX_REL */
  virtual void setEPS_RELAX_REL(double eps_rr);
  /** Get value of EPS_RELAX_REL */
  inline double getEPS_RELAX_REL() const {return EPS_RELAX_REL;}

  // Set the maximum ratio between largest and smallest non zero 
  // coefficients in a cut. Default: 1e6.
  virtual void setMAXDYN(double value);
  /** Get the value of MAXDYN */
  inline double getMAXDYN() const {return MAXDYN;}

  /** Set the value of MINVIOL, the minimum violation for the current 
      basic solution in a generated cut. Default: 1e-3 */
  virtual void setMINVIOL(double value);
  /** Get the value of MINVIOL */
  inline double getMINVIOL() const {return MINVIOL;}

  /** Maximum absolute support of the cutting planes. Default: INT_MAX.
      Aliases for consistency with our naming scheme. */
  inline void setMAX_SUPP_ABS(int value) {setMAX_SUPPORT(value);}
  inline int getMAX_SUPP_ABS() const {return MAX_SUPPORT;}

  /** Maximum relative support of the cutting planes. Default: 0.0.
      The maximum support is MAX_SUPP_ABS + MAX_SUPPREL*ncols. */
  inline void setMAX_SUPP_REL(double value); 
  inline double getMAX_SUPP_REL() const {return MAX_SUPP_REL;}

  /** Set the value of USE_INTSLACKS. Default: 0 */
  virtual void setUSE_INTSLACKS(int value);
  /** Get the value of USE_INTSLACKS */
  inline int getUSE_INTSLACKS() const {return USE_INTSLACKS;}

  /** Set the value of normIsZero, the threshold for considering a norm to be 
      0; Default: 1e-5 */
  virtual void setNormIsZero(double value);
  /** Get the value of normIsZero */
  inline double getNormIsZero() const {return normIsZero_;}

  /** Set the value of minNormReduction; Default: 0.1 */
  virtual void setMinNormReduction(double value);
  /** Get the value of normIsZero */
  inline double getMinNormReduction() const {return minNormReduction_;}

  /** Set the value of maxSumMultipliers; Default: 10 */
  virtual void setMaxSumMultipliers(int value);
  /** Get the value of maxSumMultipliers */
  inline int getMaxSumMultipliers() const {return maxSumMultipliers_;}

  /** Set the value of normalization; Default: 0.0001 */
  virtual void setNormalization(double value);
  /** Get the value of normalization */
  inline double getNormalization() const {return normalization_;}

  /** Set the value of numRowsReduction, max number of rows that are used
   *  for each row reduction step. In particular, the linear system will
   *  involve a numRowsReduction*numRowsReduction matrix */
  virtual void addNumRowsReduction(int value);
  /// get the value
  inline std::vector<int> getNumRowsReduction() const {return numRowsReduction_;}
  /// reset
  inline void resetNumRowsReduction() {numRowsReduction_.clear();}

  /** Add the value of columnSelectionStrategy */
  virtual void addColumnSelectionStrategy(ColumnSelectionStrategy value);
  /// get the value
  inline std::vector<ColumnSelectionStrategy> getColumnSelectionStrategy() const {return columnSelectionStrategy_;}
  /// reset
  inline void resetColumnSelectionStrategy(){columnSelectionStrategy_.clear();}

  /** Set the value for rowSelectionStrategy, which changes the way we choose
   *  the rows for the reduction step */
  virtual void addRowSelectionStrategy(RowSelectionStrategy value);
  /// get the value
  inline std::vector<RowSelectionStrategy> getRowSelectionStrategy() const {return rowSelectionStrategy_;};
  /// reset
  inline void resetRowSelectionStrategy() {rowSelectionStrategy_.clear();}

  /** Set the value of numRowsReductionLAP, max number of rows that are used
   *  for each row reduction step during Lift & Project. 
   *  In particular, the linear system will involve a 
   *  numRowsReduction*numRowsReduction matrix */
  virtual void addNumRowsReductionLAP(int value);
  /// get the value
  inline std::vector<int> getNumRowsReductionLAP() const {return numRowsReductionLAP_;}
  /// reset
  inline void resetNumRowsReductionLAP() {numRowsReductionLAP_.clear();}

  /** Add the value of columnSelectionStrategyLAP */
  virtual void addColumnSelectionStrategyLAP(ColumnSelectionStrategy value);
  /// get the value
  inline std::vector<ColumnSelectionStrategy> getColumnSelectionStrategyLAP() const {return columnSelectionStrategyLAP_;}
  /// reset
  inline void resetColumnSelectionStrategyLAP(){columnSelectionStrategyLAP_.clear();}

  /** Set the value for rowSelectionStrategyLAP, which changes the way we 
   *  choose the rows for the reduction step */
  virtual void addRowSelectionStrategyLAP(RowSelectionStrategy value);
  /// get the value
  inline std::vector<RowSelectionStrategy> getRowSelectionStrategyLAP() const {return rowSelectionStrategyLAP_;};
  /// reset
  inline void resetRowSelectionStrategyLAP() {rowSelectionStrategyLAP_.clear();}

  /** Set the value for columnScalingStrategyLAP, which sets the way nonbasic
   *  columns that are basic in the fractional point to cut off are scaled */
  virtual void setColumnScalingStrategyLAP(ColumnScalingStrategy value);
  /// get the value
  inline ColumnScalingStrategy getColumnScalingStrategyLAP() const {return columnScalingStrategyLAP_; };

  /** Set the value for the bound in the column scaling factor */
  virtual void setColumnScalingBoundLAP(double value);
  /// get the value
  inline double getColumnScalingBoundLAP() const {return columnScalingBoundLAP_;};

  /** Set the value of the time limit for cut generation (in seconds) */
  virtual void setTimeLimit(double value);
  /// get the value
  inline double getTimeLimit() const {return timeLimit_;}

  /** Set the value for the maximum number of cuts that can be returned */
  virtual void setMaxNumCuts(int value);
  /// get the value
  inline int getMaxNumCuts() const {return maxNumCuts_;}

  /** Set the value for the maximum number of cuts that can be computed */
  virtual void setMaxNumComputedCuts(int value);
  /// get the value
  inline int getMaxNumComputedCuts() const {return maxNumComputedCuts_;}

  /** Set the value for the maximum number of nonzeroes in a row of
   * the simplex tableau for the row to be considered */
  virtual void setMaxNonzeroesTab(int value);
  /// get the value
  inline int getMaxNonzeroesTab() const {return maxNonzeroesTab_;}

  /** Set the value of skipGomory: should we skip simple Gomory cuts,
   *  i.e. GMI cuts derived from a single row of the simple tableau?
   *  This is 1 (true) by default: we only generate cuts from linear
   *  combinations of at least two rows. */
  virtual void setSkipGomory(int value);
  /// get the value
  inline int getSkipGomory() const {return skipGomory_;}

  //@}

  /**@name Constructors and destructors */
  //@{
  /// Default constructor. If use_default_strategies is true, we add
  /// to the list of strategies the default ones. If is false, the
  /// list of strategies is left empty (must be populated before usage!).
  CglRedSplit2Param(bool use_default_strategies = true,
		    double eps = 1e-12,
		    double eps_coeff = 1e-11,
		    double eps_elim = 0.0,
		    double eps_relax_abs = 1e-11,
		    double eps_relax_rel = 1e-13,
		    double max_dyn = 1e6,
		    double min_viol = 1e-3,
		    int max_supp_abs = 1000,
		    double max_supp_rel = 0.1,
		    int use_int_slacks = 0,
		    double norm_zero = 1e-5,
		    double minNormReduction = 0.1,
		    int maxSumMultipliers = 10,
		    double normalization = 0.0001,
		    double away = 0.005,
		    double timeLimit = 60,
		    int maxNumCuts = 10000,
		    int maxNumComputedCuts = 10000,
		    int maxNonzeroesTab = 1000,
		    double columnScalingBoundLAP = 5.0,
		    int skipGomory = 1);

  /// Constructor from CglParam. If use_default_strategies is true, we
  /// add to the list of strategies the default ones. If is false, the
  /// list of strategies is left empty (must be populated before
  /// usage!).
  CglRedSplit2Param(const CglParam &source,
		    bool use_default_strategies = true,
		    double eps_elim = 0.0,
		    double eps_relax_abs = 1e-11,
		    double eps_relax_rel = 1e-13,
		    double max_dyn = 1e6,
		    double min_viol = 1e-3,
		    double max_supp_rel = 0.1,
		    int use_int_slacks = 0,
		    double norm_zero = 1e-5,
		    double minNormReduction = 0.1,
		    int maxSumMultipliers = 10,
		    double normalization = 0.0001,
		    double away = 0.005,
		    double timeLimit = 60,
		    int maxNumCuts = 10000,
		    int maxNumComputedCuts = 10000,
		    int maxNonzeroesTab = 1000,
		    double columnScalingBoundLAP = 5.0,
		    int skipGomory = 1);

  /// Copy constructor 
  CglRedSplit2Param(const CglRedSplit2Param &source);

  /// Clone
  virtual CglRedSplit2Param* clone() const;

  /// Assignment operator 
  virtual CglRedSplit2Param& operator=(const CglRedSplit2Param &rhs);

  /// Destructor 
  virtual ~CglRedSplit2Param();
  //@}

protected:

  /**@name Parameters */
  //@{

  /** Epsilon for value of coefficients when eliminating slack variables. 
      Default: 0.0. */
  double EPS_ELIM;

  /** Value added to the right hand side of each generated cut to relax it.
      Default: 1e-11 */
  double EPS_RELAX_ABS;

  /** For a generated cut with right hand side rhs_val, 
      EPS_RELAX_EPS * fabs(rhs_val) is used to relax the constraint.
      Default: 1e-13 */
  double EPS_RELAX_REL;

  // Maximum ratio between largest and smallest non zero 
  // coefficients in a cut. Default: 1e6.
  double MAXDYN;

  /// Minimum violation for the current basic solution in a generated cut.
  /// Default: 1e-3.
  double MINVIOL;

  /// Maximum support - relative part of the formula
  double MAX_SUPP_REL;

  /// Use integer slacks to generate cuts if USE_INTSLACKS = 1. Default: 0.
  int USE_INTSLACKS;

  /// Norm of a vector is considered zero if smaller than normIsZero;
  /// Default: 1e-5.
  double normIsZero_;

  /// Minimum reduction to accept a new row.
  double minNormReduction_;

  /// Maximum sum of the vector of row multipliers to generate a cut
  int maxSumMultipliers_;

  /// Normalization factor for the norm of lambda in the quadratic
  /// minimization problem that is solved during the coefficient reduction step
  double normalization_;

  /// Use row only if pivot variable should be integer but is more 
  /// than away_ from being integer. Default: 0.005
  double away_;
  
  /// Maximum number of rows to use for the reduction of a given row.
  std::vector<int> numRowsReduction_;

  /// Column selection method
  std::vector<ColumnSelectionStrategy> columnSelectionStrategy_;

  /// Row selection method
  std::vector<RowSelectionStrategy> rowSelectionStrategy_;

  /// Maximum number of rows to use for the reduction during Lift & Project
  std::vector<int> numRowsReductionLAP_;

  /// Column selection method for Lift & Project
  std::vector<ColumnSelectionStrategy> columnSelectionStrategyLAP_;

  /// Row selection method for Lift & Project
  std::vector<RowSelectionStrategy> rowSelectionStrategyLAP_;

  /// Column scaling strategy for the nonbasics columns that were basic in
  /// the point that we want to cut off (Lift & Project only)
  ColumnScalingStrategy columnScalingStrategyLAP_;

  /// Minimum value for column scaling (Lift & Project only)
  double columnScalingBoundLAP_;

  /// Time limit
  double timeLimit_;

  /// Maximum number of returned cuts
  int maxNumCuts_;

  /// Maximum number of computed cuts
  int maxNumComputedCuts_;

  /// Maximum number of nonzeroes in tableau row for reduction
  int maxNonzeroesTab_;

  /// Skip simple Gomory cuts
  int skipGomory_;

  //@}
};

#endif
