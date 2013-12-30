#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H


#include "main.h"

Expr * BvIncrExprModM (Expr *a, unsigned int m);
Expr * BvIncrExprCeilM (Expr *in, unsigned int m);
Expr * BvsubExprModM (Expr *a, Expr *b , unsigned int m);
Expr * BvminExpr (Expr *a, Expr *b );
Expr * BvmaxExpr (Expr *a, Expr *b );
Expr * BvmaxExpr (vector <Expr*> e);

Expr * BvextendExpr(Expr *a, unsigned int w); //extend expression with 0 msbs
Expr * BvDecrExprFloor0(Expr *a); 
Expr * BvIsZero (Expr *a);
Expr * BvIsRangeMax (Expr *a);


Expr *BvaddExpr( vector <Expr*> &e);
Expr * ShiftLeftExpr( Expr *e);



void matchOperandWidth(Expr *aIn, Expr *bIn, Expr* &aOut, Expr* &bOut); //



class Expr {
    unsigned int width;
public:
    Expr(unsigned int w);
    Expr();

    // dont use pure virtual, because for circular dependencies we
    //  sometimes initialize and then overwrite later
    virtual std::string printExprVerilog()=0;
    virtual unsigned int getWidth();
};









// to avoid having to write a bunch of ites
class Case_Expr : public Expr {
    Expr *defaultCase;
    vector < pair<Expr*,Expr*> > cases;
public:
Case_Expr(unsigned int w) : Expr(w) {;}
Case_Expr() : Expr() {;}
    Case_Expr * setDefault( Expr *r );
    Case_Expr * addCase( Expr *cond, Expr *val );
    string printExprVerilog();
};






class Lt_Expr : public Expr {
    Expr *_a, *_b;
public:
    Lt_Expr( Expr *a, Expr *b);
    string printExprVerilog();
};


class Lte_Expr : public Expr {
    Expr *_a, *_b;
public:
    Lte_Expr( Expr *a, Expr *b);
    string printExprVerilog();
};


class And_Expr : public Expr {
    vector <Expr*> _ins;
public:
And_Expr( )                : Expr(1) {;}
And_Expr(Expr *a, Expr *b) : Expr(1) {
	addInput(a);
	addInput(b);
    }
And_Expr(Expr *a, Expr *b, Expr *c) : Expr(1) {
	addInput(a);
	addInput(b);	
	addInput(c);
    }
    And_Expr * addInput (Expr *in) {
	_ins.push_back(in);
	return this;
    }
    string printExprVerilog();
};

/// a implies b
class Implies_Expr : public Expr {
    Expr *_a, *_b;
public:
Implies_Expr(Expr *a, Expr *b) : Expr(1) {
	ASSERT( a != 0);
	ASSERT( b != 0);
	ASSERT2((a->getWidth()==1), "illegal bool signal width");
	ASSERT2((b->getWidth()==1), "illegal bool signal width");
	_a = a;
	_b = b;
    }
    string printExprVerilog() {
	ASSERT( _a != 0);
	ASSERT( _b != 0);
	ASSERT2((_a->getWidth()==1), "illegal bool signal width");
	ASSERT2((_b->getWidth()==1), "illegal bool signal width");
	ASSERT2((getWidth()==1), "illegal expression width");
	stringstream out;
	out << " (~(" << _a->printExprVerilog() << " & (~"<< _b->printExprVerilog() << "))) ";
	return out.str();
    };

};



class Or_Expr : public Expr {
    vector <Expr*> _ins;
public:
    
Or_Expr( )                : Expr(1) {;}
Or_Expr(Expr *a, Expr *b) : Expr(1) {
	addInput(a);
	addInput(b);
    }
Or_Expr(Expr *a, Expr *b, Expr *c) : Expr(1) {
	addInput(a);    
	addInput(b);    
	addInput(c);
    }
    
    Or_Expr * addInput (Expr *in){
	_ins.push_back(in);
	return this;
    }
    string printExprVerilog();
};



class Eq_Expr : public Expr {
    Expr *a, *b;
public:
Eq_Expr(Expr *ea, Expr *eb ) : Expr(1) {
	matchOperandWidth( ea, eb, a, b );
    }
    string printExprVerilog() {
	stringstream out;
	out << " (" << a->printExprVerilog() << " == "<< b->printExprVerilog() << ") ";
	return out.str();
    };
};

class Neq_Expr : public Expr {
    Expr *a, *b;
public:
Neq_Expr(Expr *ea, Expr *eb ) : Expr(1) {
	matchOperandWidth( ea, eb, a, b );
    }
    string printExprVerilog() {
	stringstream out;
	out << " (" << a->printExprVerilog() << " != "<< b->printExprVerilog() << ") ";
	return out.str();
    };
};



class Bvadd_Expr : public Expr {
    Expr *_a, *_b;
public:
Bvadd_Expr(Expr *a, Expr *b ) : Expr( max(a->getWidth(),b->getWidth()) ) {
	ASSERT(a != 0);
	ASSERT(b != 0);
	ASSERT(a->getWidth() > 0);
	ASSERT(b->getWidth() > 0);
	matchOperandWidth( a, b, this->_a, this->_b );
    }
    string printExprVerilog() {
	ASSERT( getWidth() > 0 );
	stringstream out;
	out << "(" << _a->printExprVerilog() << " + "<< _b->printExprVerilog() << ") ";
	return out.str();
    };
};





class Bvsub_Expr : public Expr {
    Expr *_a, *_b;
public:
Bvsub_Expr( Expr *a, Expr *b ) : Expr( max(a->getWidth(),b->getWidth()) ) {
	matchOperandWidth( a, b, this->_a, this->_b );
    }
    string printExprVerilog() {
	stringstream out;
	out << "(" << _a->printExprVerilog() << " - "<< _b->printExprVerilog() << ")";
	return out.str();
    };
};



class Not_Expr : public Expr {
    Expr *_a;
public:
Not_Expr(Expr *a ) : Expr(1) {
	ASSERT2((a->getWidth() == 1), "bitwidth problem");
	ASSERT(a->getWidth() == 1);
	_a = a;
    }

    string printExprVerilog() {
	stringstream out;
	out << "(~" << _a->printExprVerilog() << ")";
	return out.str();
    };
};


class Id_Expr : public Expr {
    Expr *_a;
public:
Id_Expr(Expr *a ) : Expr(a->getWidth()) {
	ASSERT(a!=0);
	ASSERT2(a->getWidth() > 0, "bad width for Id_Expr ");
	_a = a;
    };
    string printExprVerilog() {
	stringstream out;
	out << "" << _a->printExprVerilog() << "";
	return out.str();
    };
};

class Bvconst_Expr : public Expr {
    unsigned int value;
public:
Bvconst_Expr( unsigned int v , unsigned int w ) : Expr(w) {
	value = v;
    }

    //unsigned int getValue() { return value;}
    string printExprVerilog() {
	stringstream out;
	out << " " << this->getWidth() << "'d" << value << " ";
	return out.str();
    };
};







class Extract_Expr : public Expr {
    Expr *in;
    unsigned int msb;
    unsigned int lsb;
public:
Extract_Expr(Expr *e, unsigned int m, unsigned int l) : Expr(m-l+1) {
	ASSERT(e->getWidth() >= (m-l+1));
	in = e;
	msb = m;
	lsb = l;
    }
    string printExprVerilog() {
	stringstream out;
	if (in->getWidth() == this->getWidth()){
	    out << in->printExprVerilog() ;
	} else {
	    out << in->printExprVerilog() << "["<< msb << ":" << lsb << "]";
	}
	return out.str();
    };
};


class Cat_Expr : public Expr {
    Expr *upperBits;
    Expr *lowerBits;
public:
Cat_Expr(Expr *u, Expr *l) : Expr(u->getWidth() + l->getWidth()) {
	ASSERT(u!=0);
	ASSERT(u->getWidth() > 0);
	ASSERT(l!=0);
	ASSERT(l->getWidth() > 0);
	upperBits = u;
	lowerBits = l;
    }
    string printExprVerilog() {
	stringstream out;
	out << "{" << upperBits->printExprVerilog() << ","<< lowerBits->printExprVerilog() << "}";
	return out.str();
    };

};














// this should be a static member of signal
namespace {
    map <string, int> signalNameCounts;
}



/*! a signal is a named expression */
class Signal : public Expr {
    bool signalIsAssertion; 
    bool assertionIsEnabled; // if signal is assertion, choose whether to enable
    Expr *_e;
    unsigned int signalWidth; // different from exp width, because it
    // can be assigned before the signal gets an expression.
    
public:
    bool signalExprAssigned;
    string name;
    set <string> assertionClassTags; //these tags are used to turn on/off classes of assertions
    
/// pre-instantiate signal using width. Expression will be assigned later
Signal( string n, unsigned int x ) : Expr(x) {  
	if ( ++signalNameCounts[n] > 1)
	    n += "XX"+itos(signalNameCounts[n]);
	name = n;
	signalIsAssertion = false;
	g_ckt->signals.push_back(this);

	signalWidth = x;
	signalExprAssigned = false;
	_e = 0; // unassigned
    };
    

/// construct signal from an existing expression and use its width
Signal( string n, Expr *e ) : Expr(e->getWidth()) {  
	ASSERT(e!=0);
	ASSERT(e->getWidth()>0);

 	if ( ++signalNameCounts[n] > 1)
	    n += "XX"+itos(signalNameCounts[n]);
	name = n;
	signalIsAssertion = false;
	assertionIsEnabled = false;
	g_ckt->signals.push_back(this);

	signalWidth = e->getWidth();
	signalExprAssigned = true;
	_e = e;
    };

    
    Signal * setExpr(Expr *e) {
	if (signalExprAssigned) {
	    cout << "ERROR: trying to set 2nd expr for signal " << name << "\n";
	    ASSERT(0);
	}
	ASSERT(not signalExprAssigned); // cannot set twice
	ASSERT(e != 0);
	ASSERT(e->getWidth() > 0);
	ASSERT(e->getWidth() == signalWidth);
	_e = e;
	signalExprAssigned = true;
	return this;
    }
    
    bool getAssertionIsEnabled()       { return assertionIsEnabled;}
    Signal * setAssertionIsEnabled(bool x) { assertionIsEnabled = x; return this; }
    
    bool getSignalIsAssertion() { return signalIsAssertion; }
    Signal * setSignalIsAssertion(bool x) {
	ASSERT2(getWidth() == 1, "signal " + name+" is asserting non-boolean");
	signalIsAssertion = x;
	assertionClassTags.insert(name); // every assertion gets unique class 
	assertionClassTags.insert("all");
	return this;
    }
        
    Signal * addAssertionClassTag(string s) {
	ASSERT2(getWidth() == 1, "asserted signals must be boolean");
	assertionClassTags.insert(s);
	return this;
    }
    
    string getName() {  return name; }
    unsigned int getWidth() { return signalWidth; }

    Signal * setWidth(unsigned int w) {
	ASSERT2(_e == 0, "can only change width before signal has expression");
	signalWidth = w;
	return this;
    }

    // this gets called when this signal is used as operand in other expr
    string printExprVerilog() { return " "+name+" "; }
    
    virtual void declareSignalVerilog(ostream &f) {
	if (! signalExprAssigned ) {
	    cout << "ERROR: signal " << name << " has no expr\n";
	    ASSERT(0);
	}
	ASSERT( signalExprAssigned );

	if (_e == 0) {
	    cout << "ERROR: cannot declare signal " << name << "\n";
	    ASSERT(0);
	}
	ASSERT(_e != 0);
	
	if (_e->getWidth()==0) {
	    cout << "width==0 for signal "+getName() << "\n";
	}
	
	if (getWidth() == 1)
	    f << setw(15) << left << "wire "                                << name << ";\n";
	else 
	    f << setw(15) << left << "wire ["+itos(_e->getWidth()-1)+":0] " << name << ";\n";; 
	return;
    }


    virtual void assignSignalVerilog(ostream &f) {
	ASSERT2( _e != 0,"no assignment for signal:"+getName()); 
	ASSERT2( _e->getWidth()>0, "width==0 for signal "+getName() );
	f << "assign " << setw(25) << left << name << " = " ;
	f << _e -> printExprVerilog() << ";\n";
    
	if (signalIsAssertion and assertionIsEnabled) {
	    ASSERT2(_e->getWidth()==1, "asserted signal is not boolean");
	    f << "xmas_assert assert_"+name+"(clk, reset, "+name+" );\n";
	}
	return; 
    };


};



 
class Seq_Signal : public Signal {
    Expr *resetval;
    Expr *nxt;

public:
Seq_Signal(string n, unsigned int x) : Signal(n,x ) {;}

    Seq_Signal * setResetExpr( Expr *e ) { 
	ASSERT2(e->getWidth() == this->getWidth(), "sequential reset has wrong width\n");
	resetval = e;           
	return this; 
    };
    Seq_Signal * setNxtExpr(   Expr *e ) { 
	//cout << "\n" << name << "\n";
	ASSERT( e !=0 );
	ASSERT2(e->getWidth() == this->getWidth(), "sequential has wrong width\n");
	nxt = e;
	return this;
    };


    void declareSignalVerilog(ostream &f) {
	unsigned int w = resetval->getWidth();
	ASSERT2(w>0, "width==0 for signal "+getName() );
	ASSERT2(w == nxt->getWidth(), "mismatch in sequential reset:"+itos(w)+" nxt:"+itos(nxt->getWidth())+" signal:"+getName());
	if (w == 1)
	    f << setw(15) << left << "reg "                   << name << ";\n";
	else 
	    f << setw(15) << left << "reg ["+itos(w-1)+":0] " << name << ";\n"; 
	return;
    }

    void assignSignalVerilog(ostream & f) {
	f << "always @(posedge clk)\t"  << name << " <= (reset) ?  "
	  << resetval->printExprVerilog()
	  << " : "
	  << nxt->printExprVerilog()
	  <<";\n";
	return;
    };
};





//
// ultimately, used for checking age of packet
// the architecture is that some_value is stored in
// circuit. some_value can be either an unchanging timestamp, or an age
// that increments every cycle. The storage is outside of class
//
//

Expr * updatedTimestamp(Expr *e); // how to modify the timestamp in-place within queue
Expr * initialTimestamp(); // what the source adds to each data packet

class Timestamp_Signal : public Signal {
    Signal *age; 
public:
    Timestamp_Signal(string n, Expr *e); // e is whatever is stored for the timestamp
    
    // this returns either 
    // (1) age = clk - t % tMax (in case of timestamp encoding)
    // or (2) age field of packet (in case of stopwatch encoding)
    //
    Expr * getAge() { return age; }; 

    void assertAgeBound( Expr *en, unsigned e, string tag);
};



class Oracle_Signal : public Signal {
public:
    unsigned int width;
Oracle_Signal(string n, unsigned int x) : Signal(n,x ) { width = x; }
    unsigned int getWidth() { return width; }
    void widen(unsigned int w) {  width += w; }
    string printExprVerilog() { return " "+name+" ";}
    void declareSignalVerilog(ostream &f) { return ;}
    void assignSignalVerilog(ostream &f) { return;}
};


#endif
