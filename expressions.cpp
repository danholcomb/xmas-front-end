

#include "main.h"
#include "expressions.h"

using namespace std;



Expr::Expr(unsigned int w)
{
    width = w; 
    g_ckt->expressions.push_back(this);
} 
Expr::Expr()
{
    width = 0; 
    g_ckt->expressions.push_back(this);
} 


unsigned int Expr::getWidth() { return width; } 







Case_Expr * Case_Expr::setDefault( Expr *r )
{
    ASSERT(r!=0);
    ASSERT(r->getWidth() > 0);
    defaultCase = r;    

    // only need to use this if didnt instantiate with a width
    ASSERT(this->getWidth() == defaultCase->getWidth());
    return this;
};

Case_Expr * Case_Expr::addCase( Expr *cond, Expr *val )
{
    ASSERT(cond!=0);
    ASSERT(cond->getWidth()==1);
    ASSERT2(defaultCase != 0, "must set default expr before case");
    ASSERT2(defaultCase->getWidth() > 0, "must set default expr before case");
    ASSERT2(defaultCase->getWidth() == val->getWidth(), "width mismatch");
    pair <Expr*,Expr*> p = make_pair(cond,val);
    cases.push_back(p);
    return this;
};

// cases evaluated as if,elsif,elsif,...,else
string Case_Expr::printExprVerilog()
{
    ASSERT(cases.size() > 0);
    ASSERT2(defaultCase!=0, "cant add case without default val first");
    ASSERT2((getWidth()>0), "illegal expression width="+getWidth());

    stringstream out;
    out << "(\n";
    for (vector < pair<Expr*,Expr*> >::iterator c = cases.begin(); c != cases.end();  c++ ) {
        ASSERT( c->first->getWidth() == 1);
        ASSERT( c->second->getWidth() == getWidth());
        ASSERT( defaultCase->getWidth() == getWidth());
        out << "\t"
            << c->first->printExprVerilog()  << " ? "
            << c->second->printExprVerilog() << " :\n";
    }
    out << "\t" << defaultCase->printExprVerilog() << ")";
    return out.str();
};


Lt_Expr::Lt_Expr( Expr *a, Expr *b) : Expr(1)
{
    ASSERT(a!=0);
    ASSERT(b!=0);
    matchOperandWidth( a, b, _a, _b );
}

string Lt_Expr::printExprVerilog()
{
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " < "<< _b->printExprVerilog() << ")";
    return out.str();
};



Lte_Expr::Lte_Expr( Expr *a, Expr *b) : Expr(1)
{
    ASSERT(a->getWidth() == b->getWidth());
    _a = a;
    _b = b;
}

string Lte_Expr::printExprVerilog()
{
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " <= "<< _b->printExprVerilog() << ")";
    return out.str();
};

  

string And_Expr::printExprVerilog()
{
    if(_ins.size() == 0) {return "1'd1";};
    ASSERT(_ins.size() > 0);
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    vector<Expr*>::const_iterator i = _ins.begin();
    out << "(" << (*i)->printExprVerilog();
    for ( i++; i != _ins.end(); i++ )
        out << " & " << (*i)->printExprVerilog(); 
    out << ")";
    return out.str();
};


string Or_Expr::printExprVerilog()
{
    ASSERT(_ins.size() > 0);
    stringstream out;
    vector<Expr*>::const_iterator i = _ins.begin();
    out << "(" << (*i)->printExprVerilog();
    for ( i++; i != _ins.end(); i++ ) {
        out << " | " << (*i)->printExprVerilog(); 
    }
    out << ")";
    return out.str();
};




///////////////////////////////////////////////////////////////////
// Macros that create one or more expressions and return pointer //
///////////////////////////////////////////////////////////////////

Expr * BvIsZero (Expr *a)
{
    Expr *out = new Eq_Expr( a, new Bvconst_Expr(0,a->getWidth()) );
    return out;
}

//
//
Expr * BvIsRangeMax (Expr *a)
{
    unsigned int w = a->getWidth();
    unsigned int maxval = pow(double(2),double(w))-1;
    Expr *out = new Eq_Expr( a, new Bvconst_Expr(maxval,a->getWidth()) );
    return out;
}

// returns expression for (a - b) % m
Expr * BvsubExprModM (Expr *a, Expr *b , unsigned int m)
{

    ASSERT(a!=0);
    ASSERT(b!=0);
    unsigned int w = a->getWidth();
    ASSERT(w == b->getWidth());

    // if clk = 0 and injection = 111...1, then age s/b 1. for rollover
    // in case 2, do (111...1 - injection + 1) so as not to use any
    // numbers exceeding 111...1
    //  double maxval = pow(double(2),double(w))-1;

    Expr *diff_normal   = new Bvsub_Expr( a, b);
    Expr *overflow      = new Lt_Expr(    a, b);
    Expr *x             = new Bvsub_Expr( new Bvconst_Expr(m-1,w)  , b);
    Expr *y             = new Bvadd_Expr( new Bvconst_Expr(1,w)    , x);
    Expr *diff_overflow = new Bvadd_Expr( a ,                        y);

    Expr *diff = (new Case_Expr(w))
        -> setDefault(diff_normal)
        -> addCase(overflow, diff_overflow);

    return diff;
};






// return smaller of a and b
Expr * BvminExpr (Expr *a, Expr *b )
{
    Expr *_a, *_b;
    matchOperandWidth( a, b, _a, _b );
   
    Expr *aLteB = new Lte_Expr( _a, _b);
    Expr *out = (new Case_Expr( _a->getWidth() ))
        -> setDefault( _b )
        -> addCase(aLteB, _a);

    return out;
};


// return larger of a and b
Expr * BvmaxExpr (Expr *a, Expr *b )
{
    Expr *_a, *_b;
    matchOperandWidth( a, b, _a, _b );
   
    Expr *aLteB = new Lte_Expr( _a, _b);
    Expr *out = (new Case_Expr( _a->getWidth() ))
        -> setDefault( _a )
        -> addCase(aLteB, _b );

    return out;
};

Expr * BvmaxExpr (vector <Expr*> e )
{
    ASSERT(e.size() > 0);
    Expr *m = e[0];
    for (unsigned int i = 1; i < e.size(); i++) {
        m = BvmaxExpr(m, e[i]);
    }
    return m;
};




// overflow is computed as m-b < a to avoid exceeding m
Expr * BvaddExprCeilM (Expr *a, Expr *b , unsigned int m)
{
    ASSERT(a!=0);
    ASSERT(b!=0);
    unsigned int w = a->getWidth();
    ASSERT(w == b->getWidth());
    Expr *mm          = new Bvconst_Expr( m, a->getWidth() );
    Expr *apb_gt_mm   = new Lt_Expr( new Bvsub_Expr(mm,b), a ); //computed within range of m
    Expr *apb         = new Bvadd_Expr( a , b);
    Expr *out = (new Case_Expr(w))
        -> setDefault(apb)
        -> addCase(apb_gt_mm, mm);
    return out;
};




// pad upper bits with zeros if needed
Expr * BvextendExpr (Expr *a, unsigned int w )
{
    ASSERT(a!=0);
    unsigned int wOrig = a->getWidth();

    if ( w == wOrig) {
        return a;
    } else {
        ASSERT(w > wOrig);
        Expr *zeros = new Bvconst_Expr(0,w-wOrig);
        Expr *out = (new Cat_Expr(zeros,a));
        return out;
    }
};


// returns expression for (a+1) % maxval
Expr * BvIncrExprModM (Expr *a, unsigned int m)
{
    ASSERT(a!=0);
    ASSERT(a->getWidth() > 0);

    unsigned int w = a->getWidth();
    w = max(w, numBitsRequired(m));
        
    Expr *rollover = new Eq_Expr( a , new Bvconst_Expr(m-1,w) );
    Expr *plus1 = new Bvadd_Expr( a , new Bvconst_Expr(1,w));  
    
    Expr *incrModM = (new Case_Expr(w))		 
        -> setDefault(plus1)
        -> addCase( rollover, new Bvconst_Expr(0,w) );
    
    return incrModM;
};



Expr * BvIncrExprCeilM (Expr *in, unsigned int m, string name)
{
    ASSERT(in!=0);
    ASSERT(in->getWidth() > 0);
    unsigned int w = in->getWidth();

    Expr *eqM = new Eq_Expr( in , new Bvconst_Expr(m,w) );
    Expr *out = (new Case_Expr(w))
        -> setDefault( new Bvadd_Expr( in , new Bvconst_Expr(1,w)))
        -> addCase( eqM, new Bvconst_Expr(m,w));
    
    return out;
};



Expr * BvDecrExprFloor0 (Expr *in)
{
    unsigned int w = in->getWidth();

    Expr *eq0 = new Eq_Expr( in , new Bvconst_Expr(0,w) );
    Expr *out = (new Case_Expr(w))
        -> setDefault( new Bvsub_Expr( in , new Bvconst_Expr(1,w)))
        -> addCase( eq0, new Bvconst_Expr(0,w));
    
    return out;
};




Expr *BvDecreasedBy1( Signal * in, string name)
{
    unsigned int w = in->getWidth();

    Seq_Signal *inPrev = (new Seq_Signal(name+"inPrev",w))
        -> setResetExpr( new Bvconst_Expr(0,w) )
        -> setNxtExpr( in );
    
    Expr *inPlus1 = new Bvadd_Expr( in, new Bvconst_Expr(1,w) );

    Signal *out = new Signal(name+"decreasedBy1" , new Eq_Expr( inPlus1, inPrev) );
    return out;
}


void matchOperandWidth(Expr *aIn, Expr *bIn, Expr* &aOut, Expr* &bOut)
{

    ASSERT(aIn  != 0);
    ASSERT(bIn  != 0);
    ASSERT(aIn->getWidth()  > 0);
    ASSERT(bIn->getWidth()  > 0);
    
    if (aIn->getWidth() > bIn->getWidth()) {
	    aOut = aIn;
	    bOut = BvextendExpr( bIn, aIn->getWidth() );
	}
	else if (bIn->getWidth() > aIn->getWidth()) {
	    aOut = BvextendExpr( aIn , bIn->getWidth() );
	    bOut = bIn;
	} else {
	    aOut = aIn;
	    bOut = bIn;
	}

    if ((aIn->getWidth() != aOut->getWidth()) or (bIn->getWidth() != bOut->getWidth())) {
        cout << "matchOperandWidth() : "
             << "input widths = (" << aIn->getWidth() << "," << bIn->getWidth() << "), "
             << "output widths = (" << aOut->getWidth() << "," << bOut->getWidth() << ")\n";
    }

    return;
}







Expr * updatedTimestamp(Expr *e)
{
    if        (g_ckt->timestampType == TIMESTAMP_BINARY_FIXED) {
        return e;
    } else if (g_ckt->timestampType == TIMESTAMP_BINARY_INCREMENTING) {
        return BvIncrExprModM (e, g_ckt->tCurrentModulus);
    } else if (g_ckt->timestampType == TIMESTAMP_UNARY_INCREMENTING) {
        return ShiftLeftExpr(e); //shift in a 1 at lsb
    } 
    ASSERT(0);
}


Expr * initialTimestamp()
{

    switch (g_ckt->timestampType) {
        case TIMESTAMP_BINARY_FIXED:
            return  g_ckt->tCurrent;
        case TIMESTAMP_BINARY_INCREMENTING:
            return new Bvconst_Expr(0, g_ckt->wClk );
        case TIMESTAMP_UNARY_INCREMENTING:
            return new Bvconst_Expr(1, g_ckt->wClk ); // 1 in the 0 bit = age 0
        default: ASSERT(0);
    }
}


Timestamp_Signal::Timestamp_Signal(string n , Expr *e) : Signal(n,e)
{

    if        (g_ckt->timestampType == TIMESTAMP_BINARY_FIXED) {
        age = new Signal( name+"__age", BvsubExprModM( g_ckt->tCurrent , e , g_ckt->tCurrentModulus ));
    } else if (g_ckt->timestampType == TIMESTAMP_BINARY_INCREMENTING) {
        age = new Signal( name+"__age",  e );
    } else if (g_ckt->timestampType == TIMESTAMP_UNARY_INCREMENTING) {
        age = new Signal( name+"__age",  e ); //unary
    } 

}



void Timestamp_Signal::assertAgeBound( Expr *en, unsigned b, string tag)
{
    ASSERT(b < g_ckt->tCurrentModulus);

    if        (g_ckt->timestampType == TIMESTAMP_BINARY_FIXED) {
        Expr *eb = new Bvconst_Expr(b, g_ckt->wClk);
        Signal *ageValid = (new Signal(  name+"__"+tag, new Implies_Expr( en , new Lt_Expr( age , eb))))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag(tag)
            -> addAssertionClassTag("phi");


    } else if (g_ckt->timestampType == TIMESTAMP_BINARY_INCREMENTING) {
        Expr *eb = new Bvconst_Expr(b, g_ckt->wClk);
        Signal *ageValid = (new Signal(  name+"__"+tag, new Implies_Expr( en , new Lt_Expr( age , eb))))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag(tag)
            -> addAssertionClassTag("phi");

    } else if (g_ckt->timestampType == TIMESTAMP_UNARY_INCREMENTING) {

        Expr *e = new Extract_Expr(age, b, b);
        Signal *ageValid = (new Signal(  name+"__"+tag, new Implies_Expr( en , new Not_Expr(e)) ))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag(tag)
            -> addAssertionClassTag("phi");

        // these could make problem solvable without specialized invariant because it imposes order
        And_Expr *unaryInv = new And_Expr();
        for (unsigned i = 0; i<g_ckt->wClk-1; i++) {
            unsigned j = i+1;
            Expr *oneBelow  = new Implies_Expr( new Extract_Expr(age,j,j),
                                                new Extract_Expr(age,i,i)  );
            Expr *zeroAbove = new Implies_Expr( new Not_Expr(new Extract_Expr(age,i,i) ),
                                                new Not_Expr(new Extract_Expr(age,j,j) )  );
            unaryInv -> addInput(oneBelow);
            unaryInv -> addInput(zeroAbove);
        }

        Signal *inv = (new Signal(  name+"__"+tag+"__unaryInv", unaryInv))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag(tag)
            -> addAssertionClassTag("phi")
            -> addAssertionClassTag("phil");
    } 
    
	return;
};








Expr * BvaddExpr( vector <Expr*> &operands)
{
    Expr *sum = new Bvconst_Expr(0,1);
    for (vector < Expr*>::iterator it = operands.begin(); it != operands.end();  it++ ) {
        sum = new Bvadd_Expr(sum,*it);
    }
    return sum;
}

Expr * ShiftLeftExpr( Expr *e)
{
    Expr *in = new Bvconst_Expr(1,1);
    Expr *e1 = new Extract_Expr( e, e->getWidth()-2, 0);
    Expr *e2 = new Cat_Expr(e1, in);
    return e2;
}
