

/* these are all very simple, just letting them be in header*/

using namespace std;

class Expr;
class Signal;
class Seq_Signal;
class Oracle_Signal;


class Expr {
  unsigned int width;
public:

  Expr(unsigned int w) { setWidth(w); }
  Expr()               { setWidth(0); }

  // dont use pure virtual, because for circular dependencies we
  //  sometimes initialize and then overwrite later
  //  virtual string printExprVerilog() { ASSERT(0);}
  virtual string printExprVerilog() { ;}

  virtual unsigned int getWidth() {
    return width;
  }

  Expr * setWidth( unsigned int w ) {
    width = w;
    return this;
  }
};




// to avoid having to write a bunch of ites
class Case_Expr : public Expr {
  Expr *defaultCase;
  vector < pair<Expr*,Expr*> > cases;
public:

  Case_Expr(unsigned int w) : Expr(w) {;}
  Case_Expr() : Expr() {;}
  
  Case_Expr * setDefault( Expr *r ) { 
    defaultCase = r; 
    setWidth( r->getWidth() );
    return this; 
  };
  
  Case_Expr * addCase( Expr *cond, Expr *val ) { 
    ASSERT2(defaultCase->getWidth() > 0, "must set default expr before case");
    ASSERT2(defaultCase->getWidth() == val->getWidth(), "width mismatch");
    pair <Expr*,Expr*> p = make_pair(cond,val);
    cases.push_back(p);
    return this; 
  };

  string printExprVerilog() {
    ASSERT(cases.size() > 0);
    ASSERT2(defaultCase!=0, "cant add case without default val first");
    ASSERT2((getWidth()>0), "illegal expression width="+getWidth());

    stringstream out;
    for (vector < pair<Expr*,Expr*> >::iterator c = cases.begin(); c != cases.end();  c++ ) {
      ASSERT( c->first->getWidth() == 1);
      ASSERT( c->second->getWidth() == getWidth());
      ASSERT( defaultCase->getWidth() == getWidth());
      
      out << "\n\t" << c->first->printExprVerilog() 
	  << " ? " << c->second->printExprVerilog() << " : ";

    }
    out << defaultCase->printExprVerilog() << "";
    return out.str();
  };

};






class Lt_Expr : public Expr {
  Expr *_a, *_b;
public:
  
  Lt_Expr( Expr *a, Expr *b) : Expr(1) {
    _a = a;
    _b = b;
  }


  string printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << _a->printExprVerilog() << " < "<< _b->printExprVerilog() << " ";
    return out.str();
  };
};

class Lte_Expr : public Expr {
  Expr *_a;
  Expr *_b;
public:
  Lte_Expr( Expr *a, Expr *b) : Expr(1) {
    _a = a;
    _b = b;
  }

  string printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << _a->printExprVerilog() << " <= "<< _b->printExprVerilog() << " ";
    return out.str();
  };
};

class And_Expr : public Expr {
  vector <Expr*> _ins;
public:
  And_Expr( )                : Expr(1) {;}
  And_Expr(Expr *a, Expr *b) : Expr(1) {
    addInput(a);
    addInput(b);
  }
  
  And_Expr * addInput (Expr *in){
    _ins.push_back(in);
    return this;
  }

  string printExprVerilog() {
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

  string printExprVerilog() {
    ASSERT(_ins.size() > 0);
    stringstream out;
    vector<Expr*>::const_iterator i = _ins.begin();
    out << "(" << (*i)->printExprVerilog();
    for ( i++; i != _ins.end(); i++ )
      out << " | " << (*i)->printExprVerilog(); 
    out << ")";
    return out.str();
  };
};



class Eq_Expr : public Expr {
  Expr *a, *b;
public:
  Eq_Expr(Expr *ea, Expr *eb ) : Expr(1) {
    a = ea;
    b = eb;
  }

  string printExprVerilog() {
    stringstream out;
    out << " (" << a->printExprVerilog() << " == "<< b->printExprVerilog() << ") ";
    return out.str();
  };
};



class Bvadd_Expr : public Expr {
  Expr *_a, *_b;
public:
  Bvadd_Expr(unsigned int w, Expr *a, Expr *b ) : Expr(w) {
    _a = a;
    _b = b;
  }
  Bvadd_Expr(Expr *a, Expr *b ) : Expr() {
    ASSERT(a->getWidth() > 0);
    ASSERT(b->getWidth() > 0);
    ASSERT(a->getWidth() == b->getWidth());
    
    _a = a;
    _b = b;
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
  Bvsub_Expr(unsigned int w, Expr *a, Expr *b ) : Expr(w) {
    _a = a;
    _b = b;
  }
  Bvsub_Expr( Expr *a, Expr *b ) : Expr() {
    _a = a;
    _b = b;
  }

  string printExprVerilog() {
    stringstream out;
    out << _a->printExprVerilog() << " - "<< _b->printExprVerilog() << " ";
    return out.str();
  };
};



class Not_Expr : public Expr {
  Expr *_a;
public:
  Not_Expr(Expr *a ) : Expr(1) {
    _a = a;
  }

  string printExprVerilog() {
    stringstream out;
    out << "~" << _a->printExprVerilog() << " ";
    return out.str();
  };
};


class Id_Expr : public Expr {
  Expr *_a;
public:
  Id_Expr(unsigned int w, Expr *a ) : Expr(w) {
    _a = a;
  }
  Id_Expr(Expr *a ) : Expr() {
    _a = a;
  }

  string printExprVerilog() {
    stringstream out;
    out << "" << _a->printExprVerilog() << "";
    return out.str();
  };
};

class Bvconst_Expr : public Expr {
public:
  unsigned int value;

  Bvconst_Expr( unsigned int v , unsigned int w ) : Expr(w) {
    value = v;
  }

  Bvconst_Expr(unsigned int v) : Expr() {
    value = v;
  }

  string printExprVerilog() {
    stringstream out;
    out << "" << value << "";
    return out.str();
  };
};







class Extract_Expr : public Expr {
  Expr *in;
  unsigned int msb;
  unsigned int lsb;
public:
  Extract_Expr(Expr *e, unsigned int m, unsigned int l) : Expr(m-l+1) {
    in = e;
    lsb = l;
    msb = m;
  }
  string printExprVerilog() {
    stringstream out;
    out << in->printExprVerilog() << "["<< msb << ":" << lsb << "]";
    return out.str();
  };

};

class Cat_Expr : public Expr {
  Expr *upperBits;
  Expr *lowerBits;
public:
  Cat_Expr(Expr *u, Expr *l) : Expr(u->getWidth() + l->getWidth()) {
    upperBits = u;
    lowerBits = l;
  }
  string printExprVerilog() {
    stringstream out;
    out << "{" << upperBits->printExprVerilog() << ","<< lowerBits->printExprVerilog() << "}";
    return out.str();
  };

};









