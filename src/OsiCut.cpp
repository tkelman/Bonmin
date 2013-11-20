// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "OsiCut.hpp"

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiCut::OsiCut ()
:
  effectiveness_(0.),
  globallyValid_(false)
//timesUsed_(0),
//timesTested_(0)
{
  // nothing to do here
}
//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiCut::OsiCut (
                  const OsiCut & source)
:
  effectiveness_(source.effectiveness_),
  globallyValid_(source.globallyValid_)
//timesUsed_(source.timesUsed_),
//timesTested_(source.timesTested_)
{  
  // nothing to do here
}

#if 0
//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiCut * OsiCut::clone() const
{  return (new OsiCut(*this));}
#endif

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiCut::~OsiCut ()
{
  // nothing to do here
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiCut &
OsiCut::operator=(const OsiCut& rhs)
{
  if (this != &rhs) {
    effectiveness_=rhs.effectiveness_;
    globallyValid_ = rhs.globallyValid_;
    //timesUsed_=rhs.timesUsed_;
    //timesTested_=rhs.timesTested_;
  }
  return *this;
}




