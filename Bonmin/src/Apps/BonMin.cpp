// (C) Copyright International Business Machines Corporation and Carnegie Mellon University 2006, 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// Authors :
// Andreas Waechter, International Business Machines Corporation
// Pierre Bonami, Carnegie Mellon University,
//
// Date : 02/15/2006


#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <iomanip>
#include <fstream>

#include "CoinTime.hpp"
#include "BonAmplSetup.hpp"
#include "BonCbc2.hpp"

namespace Bonmin{
extern int usingCouenne;}
using namespace Bonmin;
int main (int argc, char *argv[])
{
  using namespace Ipopt;
  char * pbName = NULL;
  
  if(argc > 1) {
    pbName = new char[strlen(argv[1])+1];
    strcpy(pbName, argv[1]);
  }
  
#if 0
  try
#endif 
  {

    BonminAmplSetup bonmin;
    bonmin.initialize(argv);
    Bab2 bb;

    bb(bonmin);//do branch and bound

  }
  #if 0
  catch(TNLPSolver::UnsolvedError *E) {
    E->writeDiffFiles();
    E->printError(std::cerr);
    //There has been a failure to solve a problem with Ipopt.
    //And we will output file with information on what has been changed in the problem to make it fail.
    //Now depending on what algorithm has been called (B-BB or other) the failed problem may be at different place.
    //    const OsiSolverInterface &si1 = (algo > 0) ? nlpSolver : *model.solver();
  }
  catch(OsiTMINLPInterface::SimpleError &E) {
    std::cerr<<E.className()<<"::"<<E.methodName()
    <<std::endl
    <<E.message()<<std::endl;
  }
  catch(CoinError &E) {
    std::cerr<<E.className()<<"::"<<E.methodName()
    <<std::endl
    <<E.message()<<std::endl;
  }
  catch (Ipopt::OPTION_INVALID &E)
  {
    std::cerr<<"Ipopt exception : "<<E.Message()<<std::endl;
  }
  catch(...) {
    std::cerr<<pbName<<" unrecognized exception"<<std::endl;
    std::cerr<<pbName<<"\t Finished \t exception"<<std::endl;
    throw;
  }
#endif
  
  delete [] pbName;
  return 0;
}
