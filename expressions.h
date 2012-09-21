#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H



class Expr {
  unsigned int width;
 public:
  Expr(unsigned int w);
  Expr();

  // dont use pure virtual, because for circular dependencies we
  //  sometimes initialize and then overwrite later
  virtual std::string printExprVerilog();
  virtual unsigned int getWidth();
  Expr * setWidth( unsigned int w );
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
    ASSERT(a->getWidth() == b->getWidth());
    _a = a;
    _b = b;
  }


  string printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " < "<< _b->printExprVerilog() << ")";
    return out.str();
  };
};

class Lte_Expr : public Expr {
  Expr *_a;
  Expr *_b;
 public:
 Lte_Expr( Expr *a, Expr *b) : Expr(1) {
    ASSERT(a->getWidth() == b->getWidth());
    _a = a;
    _b = b;
  }

  string printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " <= "<< _b->printExprVerilog() << ")";
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
    ASSERT(ea->getWidth() == eb->getWidth());
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
    ASSERT(w == a->getWidth() );
    ASSERT(w == b->getWidth() );
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
    ASSERT(w == a->getWidth() );
    ASSERT(w == b->getWidth() );
    _a = a;
    _b = b;
  }
 Bvsub_Expr( Expr *a, Expr *b ) : Expr() {
    ASSERT(a->getWidth() == b->getWidth() );
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
    ASSERT2((a->getWidth() == 1), "bitwidth problem: "+a->printExprVerilog() );
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
 Id_Expr(unsigned int w, Expr *a ) : Expr(w) {
  ASSERT(w == a->getWidth());
    _a = a;
  }
 Id_Expr(Expr *a ) : Expr(a->getWidth()) {
    ASSERT2(a->getWidth() > 0, "bad width for Id_Expr "+ a->printExprVerilog() );
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












namespace {
  map <string, int> signal_name_counts;
}



/*! a signal is a named expression */
class Signal : public Expr {
  bool isAsserted;
  Expr *_e;
  bool _hasExpr; // not counting the null expr from constructor
 public:
  string name;


  // add a new signal to the ckt
  // width defaults to 1 if not provided;
  // name defaults to "gen" if not provided

 Signal( string n ) : Expr() {  
    setName(n);
    isAsserted = false;
    g_ckt->signals.push_back(this);
    _e = new Expr();
  };
 Signal(  ) : Expr() {      
    setName("generic");
    isAsserted = false;
    g_ckt->signals.push_back(this);
    _e = new Expr();
  };


  Signal * setExpr(Expr *e) {
    _e = e;
    _hasExpr = true;
    return this;
  }

  void assertSignalTrue() {
    ASSERT2(getWidth() == 1, "asserted signals must be boolean");
    cout << "asserting\t" << name << "\n"; 
    isAsserted = true;
  }

  Signal * setName( string n ) {
    // if multiple signals created with same name, rename them for uniqueness
    int name_count = signal_name_counts[n]++; 
    if (name_count > 0) 
      n += "_DUP"+itos(name_count);
    name = n;
    return this;
  }

  bool hasExpr() {return _hasExpr; }

  string getName() {  return name; }

  unsigned int getWidth() {  return _e->getWidth(); }

  Signal * setWidth(unsigned int w) { 
    _e->setWidth(w);
    return this;
  }


  // this gets called when this signal is used as operand in other expr
  string printExprVerilog() {
    return " "+name+" "; 
  }


  virtual void declareSignalVerilog(ostream &f) {
    ASSERT2(_e->getWidth()>0, "width==0 for signal "); //+getName() );
    if (getWidth() == 1)
      f << setw(15) << left << "wire "                       << name << ";\n";
    else 
      f << setw(15) << left << "wire ["+itos(_e->getWidth()-1)+":0] " << name << ";\n";; 
    return;
  }


  virtual void assignSignalVerilog(ostream &f) {
    ASSERT(_e != 0);
    ASSERT2(_e->getWidth()>0, "width==0 for signal "+getName() );
    ASSERT2(hasExpr(),"no assignment for signal:"+getName()); 
    f << "assign " << setw(25) << left << name << " = " ;
    f << _e -> printExprVerilog() << ";\n";
    
    if (isAsserted) {
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

 Seq_Signal(string n) : Signal(n ) {
    resetval = new Expr();
    nxt = new Expr();    
  }
  
  Seq_Signal * setResetExpr( Expr *e ) { resetval = e; return this; };
  Seq_Signal * setNxtExpr( Expr *e ) { nxt = e; return this; };
  unsigned int getWidth()    { return resetval->getWidth(); }
  Seq_Signal * setWidth(unsigned int w) { resetval->setWidth(w); return this; }

  string printExprVerilog() { return " "+name+" ";}

  void declareSignalVerilog(ostream &f) {
    unsigned int w = resetval->getWidth();
    ASSERT2(w>0, "width==0 for signal "+getName() );
    ASSERT2(w == nxt->getWidth(), "mismatch in sequential reset:"+itos(w)+" nxt:"+itos(nxt->getWidth())+getName());
    if (w == 1)
      f << setw(15) << left << "reg "                   << name << ";\n";
    else 
      f << setw(15) << left << "reg ["+itos(w-1)+":0] " << name << ";\n"; 
    return;
  }

  void assignSignalVerilog(ostream & f) {
    f << "always @(posedge clk)\n\t"  << name << " <= (reset) ?  "
      << resetval->printExprVerilog()
      << " : "
      << nxt->printExprVerilog()
      <<";\n";
    return;
  };
};




class Oracle_Signal : public Signal {
 public:
  unsigned int width;
 Oracle_Signal(string n) : Signal(n) { width = 0; }
  unsigned int getWidth() { return width; }
  void widen(unsigned int w) {  width += w; }
  string printExprVerilog() { return " "+name+" ";}
  void declareSignalVerilog(ostream &f) { return ;}
  void assignSignalVerilog(ostream &f) { return;}
};


#endif
