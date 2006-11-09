// (C) Copyright International Business Machines Corporation and Carnegie Mellon University 2006 
// All Rights Reserved.
// This code is published under the Common Public License.
//
// Authors :
// Laszlo Ladanyi, International Business Machines Corporation
//

#include "BM.hpp"
#include "BonIpoptSolver.hpp"

//#############################################################################

void
BM_lp::pack_feasible_solution(BCP_buffer& buf, const BCP_solution* sol)
{
    const BM_solution* bs = dynamic_cast<const BM_solution*>(sol);
    if (!bs) {
	throw BCP_fatal_error("Trying to pack non-BM_solution.\n");
    }
    buf.pack(bs->_objective);
    buf.pack(bs->_ind);
    buf.pack(bs->_values);
}

/****************************************************************************/

BCP_solution*
BM_tm::unpack_feasible_solution(BCP_buffer& buf)
{
    BM_solution* bs = new BM_solution;
    buf.unpack(bs->_objective);
    buf.unpack(bs->_ind);
    buf.unpack(bs->_values);
    return bs;
}

//#############################################################################

void
BM_tm::pack_module_data(BCP_buffer& buf, BCP_process_t ptype)
{
    // possible process types looked up in BCP_enum_process_t.hpp
    switch (ptype) {
    case BCP_ProcessType_LP:
	par.pack(buf);
	buf.pack(nl_file_content);
	buf.pack(ipopt_file_content);
	break;
    default:
	abort();
    }
}

/****************************************************************************/

void
BM_lp::unpack_module_data(BCP_buffer& buf)
{
    par.unpack(buf);
    buf.unpack(nl_file_content);
    buf.unpack(ipopt_file_content);

    char* argv_[3];
    char** argv = argv_;
    argv[0] = NULL;
    argv[1] = strdup("dont_even_try_to_open_it.nl");
    argv[2] = NULL;
    std::string ipopt_content(ipopt_file_content.c_str());
    std::string nl_content(nl_file_content.c_str());
    nlp.readAmplNlFile(argv, new Bonmin::IpoptSolver,
		       &ipopt_content, &nl_content);
    free(argv[1]);

    nlp.extractInterfaceParams();

    /* synchronize bonmin & BCP parameters */
    Ipopt::SmartPtr<Ipopt::OptionsList> options = nlp.retrieve_options();

    int nlpLogLevel;
    options->GetIntegerValue("nlp_log_level", nlpLogLevel, "bonmin.");
    nlp.messageHandler()->setLogLevel(nlpLogLevel);

    double bm_intTol;
    double bm_cutoffIncr; // could be negative
    options->GetNumericValue("integer_tolerance",bm_intTol,"bonmin.");
    options->GetNumericValue("cutoff_decr",bm_cutoffIncr,"bonmin.");

    BCP_lp_prob* bcp_lp = getLpProblemPointer();
    const double bcp_intTol = bcp_lp->par.entry(BCP_lp_par::IntegerTolerance);
    const double bcp_cutoffIncr = bcp_lp->par.entry(BCP_lp_par::Granularity);

    if (fabs(bm_intTol - bcp_intTol) > 1e-10) {
	printf("WARNING!\n");
	printf("   The integrality tolerance parameters are different for\n");
	printf("   BCP (%f) and bonmin (%f). They should be identical.\n",
	       bcp_intTol, bm_intTol);
	printf("   For now both will be set to that of bonmin.\n");
    }
    if (fabs(bm_cutoffIncr - bcp_cutoffIncr) > 1e-10) {
	printf("WARNING!\n");
	printf("   The granularity (cutoff increment) parameters are different\n");
	printf("   BCP (%f) and bonmin (%f). They should be identical.\n",
	       bcp_cutoffIncr, bm_cutoffIncr);
	printf("   For now both will be set to that of bonmin.\n");
    }
    bcp_lp->par.set_entry(BCP_lp_par::IntegerTolerance, bm_intTol);
    bcp_lp->par.set_entry(BCP_lp_par::Granularity, bm_cutoffIncr);

    /* If pure BB is selected then a number of BCP parameters are changed */
    if (par.entry(BM_par::PureBranchAndBound)) {
	/* disable strong branching */
	bcp_lp->par.set_entry(BCP_lp_par::MaxPresolveIter, -1);
	/* disable a bunch of printing, all of which are meaningless, since the
	   LP relaxation is meaningless */
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_LpSolutionValue, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_FinalRelaxedSolution, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_RelaxedSolution, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_ReportLocalCutPoolSize, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_ReportLocalVarPoolSize, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_GeneratedCutCount, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_GeneratedVarCount, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_IterationCount, false);
	bcp_lp->par.set_entry(BCP_lp_par::LpVerb_RowEffectivenessCount, false);
	//  bcp_lp->par.set_entry(BCP_lp_par::LpVerb_FathomInfo, false);
    }

    /* extract the sos constraints */
    const Bonmin::TMINLP::SosInfo * sos = nlp.model()->sosConstraints();
    
    int i;
    const int numCols = nlp.getNumCols();
    const double* clb = nlp.getColLower();
    const double* cub = nlp.getColUpper();

    /* Find first the integer variables and then the SOS constraints */
    int nObj = 0;
    OsiObject** osiObj = new OsiObject*[numCols + sos->num];
    for (i = 0; i < numCols; ++i) {
	if (nlp.isInteger(i)) {
	    osiObj[nObj++] = new OsiSimpleInteger(i, clb[i], cub[i]);
	}
    }
    const int* starts = sos->starts;
    for (i = 0; i < sos->num; ++i) {
	osiObj[nObj++] = new OsiSOS(NULL, /* FIXME: why does the constr need */
				    starts[i+1] - starts[i],
				    sos->indices + starts[i],
				    sos->weights + starts[i],
				    sos->types[i]);
    }
    nlp.addObjects(nObj, osiObj);
    for (i = 0; i < nObj; ++i) {
	delete osiObj[i];
    }
    delete[] osiObj;

    /* just to be on the safe side... always allocate */
    primal_solution_ = new double[nlp.getNumCols()];
}

//#############################################################################

void
BM_tm::pack_user_data(const BCP_user_data* ud, BCP_buffer& buf)
{
    const BM_node* data = dynamic_cast<const BM_node*>(ud);
    data->pack(buf);
}

/*---------------------------------------------------------------------------*/

BCP_user_data*
BM_tm::unpack_user_data(BCP_buffer& buf)
{
    return new BM_node(buf);
}

/*****************************************************************************/

void
BM_lp::pack_user_data(const BCP_user_data* ud, BCP_buffer& buf)
{
    const BM_node* data = dynamic_cast<const BM_node*>(ud);
    data->pack(buf);
}

/*---------------------------------------------------------------------------*/

BCP_user_data*
BM_lp::unpack_user_data(BCP_buffer& buf)
{
    BM_node* data = new BM_node(buf);
    numNlpFailed_ = data->numNlpFailed_;
    return data;
}

//#############################################################################
