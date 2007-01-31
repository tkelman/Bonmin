/*
 * Name:    nl2e.cpp
 * Author:  Pietro Belotti
 * Purpose: converts a nl expression into a Feline expression
 *
 * This file is licensed under the Common Public License (CPL)
 */

#include <CouenneTypes.h>

#include <exprAbs.h>
#include <exprSum.h>
#include <exprSub.h>
#include <exprMul.h>
#include <exprDiv.h>
#include <exprInv.h>
#include <exprSin.h>
#include <exprPow.h>
#include <exprClone.h>
#include <exprLog.h>
#include <exprOpp.h>
#include <exprCos.h>
#include <exprExp.h>

#include <asl_pfgh.h>
#include <nlp2.h>
#include <opcode.hd>


// get ASL op. code relative to function pointer passed as parameter 

extern "C" {
  int getOperator (efunc2 *);
}


// converts an AMPL expression (sub)tree into an expression* (sub)tree

expression *nl2e (expr2 *e) {

  expression **al;
  expr **ep;

  switch (getOperator (e -> op)) {

  case OPPLUS:   /*printf ("[+]");*/ return new exprSum (nl2e (e -> L.e), nl2e (e -> R.e));
  case OPMINUS:  /*printf ("[-]");*/ return new exprSub (nl2e (e -> L.e), nl2e (e -> R.e));
  case OPMULT:   /*printf ("[*]");*/ return new exprMul (nl2e (e -> L.e), nl2e (e -> R.e));
  case OPDIV:    /*printf ("[/]");*/ return new exprDiv (nl2e (e -> L.e), nl2e (e -> R.e));
    //  case OPREM:
  case OPPOW:    /*printf ("[^]");*/ return new exprPow (nl2e (e -> L.e), nl2e (e -> R.e));
    //  case OPLESS:    
    //  case MINLIST:   
    //  case MAXLIST:   
    //  case FLOOR:     
    //  case CEIL:      
  case ABS:       /*printf ("[|]");*/ return new exprAbs (nl2e (e -> L.e));
  case OPUMINUS:  /*printf ("[~]");*/ return new exprOpp (nl2e (e -> L.e));
    //  case OPIFnl:
    //  case OP_tanh:
    //  case OP_tan:
  case OP_sqrt:   /*printf ("[v]");*/ return new exprPow (nl2e (e -> L.e), new exprConst (0.5));
  case OP_sinh:   /*printf ("[s]");*/ return new exprMul (new exprConst (0.5),
				      new exprSub (new exprExp (nl2e (e -> L.e)),
						   new exprExp (new exprOpp (nl2e (e -> L.e)))));
  case OP_sin:    /*printf ("[S]");*/ return new exprSin (nl2e (e -> L.e));
  case OP_log10:  /*printf ("[L]");*/ return new exprMul (new exprConst (1.0 / log (10.0)), 
							  new exprLog (nl2e (e -> L.e)));
  case OP_log:    /*printf ("[l]");*/ return new exprLog (nl2e (e -> L.e));
  case OP_exp:    /*printf ("[e]");*/ return new exprExp (nl2e (e -> L.e));
  case OP_cosh:   /*printf ("[c]");*/ return new exprMul (new exprConst (0.5),
				      new exprSum (new exprExp (nl2e (e -> L.e)),
						   new exprExp (new exprOpp (nl2e (e -> L.e)))));

  case OP_cos:    /*printf ("[C]");*/ return new exprCos (nl2e (e -> L.e));
    //  case OP_atanh:
    //  case OP_atan2:
    //  case OP_atan:
    //  case OP_asinh:
    //  case OP_asin:
    //  case OP_acosh:
    //  case OP_acos:
  case OPSUMLIST: {
    register int i=0;
    al = new expression * [(e->R.ep - e->L.ep)];
    for (ep = e->L.ep; ep < e->R.ep; ep++)
      al [i++] = nl2e (*ep);
    return new exprSum (al, i);
  }
    //  case OPintDIV:
    //  case OPprecision:
    //  case OPround:
    //  case OPtrunc:

  case OP1POW: /*printf ("[p]");*/ return new exprPow (nl2e (e -> L.e), 
						       new exprConst (((expr_n *)e->R.e)->v));
  case OP2POW: /*printf ("[P]");*/ return new exprPow (nl2e (e -> L.e), 
						       new exprConst (2));
  case OPCPOW: /*printf ("[E]");*/ return new exprPow (new exprConst (((expr_n *)e->L.e)->v),
						       nl2e (e -> R.e));
    //  case OPFUNCALL:
  case OPNUM:   return new exprConst (((expr_n *)e)->v);
    //  case OPPLTERM:
    //  case OPIFSYM:
    //  case OPHOL:
  case OPVARVAL: return new exprVar (e->a);

  case -1:
  default: printf ("WARNING: operator %d in expression not implemented\n", (long int) e -> op); 
    return new exprConst (0);
  }
}
