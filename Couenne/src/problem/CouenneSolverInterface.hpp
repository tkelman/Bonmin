/*
 * Name:    CouenneSolverInterface.hpp
 * Authors: Pietro Belotti, Carnegie Mellon University
 * Purpose: OsiSolverInterface with a pointer to a CouenneProblem object
 *
 * (C) Carnegie-Mellon University, 2006. 
 * This file is licensed under the Common Public License (CPL)
 */

#ifndef COUENNESOLVERINTERFACE_HPP
#define COUENNESOLVERINTERFACE_HPP

#include <CoinFinite.hpp>
#include <OsiClpSolverInterface.hpp>
#include <CouenneCutGenerator.hpp>


/// Solver interface class with a pointer to a Couenne cut
/// generator. Its main purposes are:
///
/// 1) to apply bound tightening before re-solving
/// 2) to replace OsiSolverInterface::isInteger () with problem_ -> [expression] -> isInteger ()
/// 3) to use NLP solution at branching
 
class CouenneSolverInterface: public OsiClpSolverInterface {

private:

  /// The pointer to the Couenne cut generator. Gives us a lot of
  /// information, for instance the nlp solver pointer, and the chance
  /// to do bound tightening before resolve ().
  CouenneCutGenerator *cutgen_;

public:

  /// Constructor
  CouenneSolverInterface (CouenneCutGenerator *cg = NULL):
    OsiClpSolverInterface(),
    cutgen_ (cg) {}

  /// Copy constructor
  CouenneSolverInterface (const CouenneSolverInterface &src):
    OsiClpSolverInterface (src),
    cutgen_ (src.cutgen_) {}

  /// Destructor
  ~CouenneSolverInterface () {}

  /// Clone
  virtual OsiSolverInterface * clone (bool copyData = true) const
    {return new CouenneSolverInterface (*this);}

  /// Return cut generator pointer
  CouenneCutGenerator *CutGen ()
    {return cutgen_;}

  /// Set cut generator pointer after setup, to avoid changes in the
  /// pointer due to cut generator cloning (it happens twice in the
  /// algorithm)
  void setCutGenPtr (CouenneCutGenerator *cg)
    {cutgen_ = cg;}

  /// Solve initial LP relaxation 
  virtual void initialSolve (); 

  /// Resolve an LP relaxation after problem modification
  virtual void resolve ();
};

#endif
