
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <set>



#include "main.h"
#include "expressions.h"

using namespace std;




Expr::Expr(unsigned int w) { setWidth(w); } 
Expr::Expr()               { setWidth(0); } 

  // dont use pure virtual, because for circular dependencies we */
  //  sometimes initialize and then overwrite later */
string Expr::printExprVerilog() { 
  ASSERT(0); return "";
}

unsigned int Expr::getWidth() { 
  return width; 
} 

Expr * Expr::setWidth( unsigned int w ) { 
  width = w; 
  return this; 
} 






Case_Expr * Case_Expr::setDefault( Expr *r ) {
    defaultCase = r;
    setWidth( r->getWidth() );
    return this;
  };

Case_Expr * Case_Expr::addCase( Expr *cond, Expr *val ) {
  ASSERT2(defaultCase->getWidth() > 0, "must set default expr before case");
  ASSERT2(defaultCase->getWidth() == val->getWidth(), "width mismatch");
  pair <Expr*,Expr*> p = make_pair(cond,val);
  cases.push_back(p);
  return this;
};


string Case_Expr::printExprVerilog() {
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


Lt_Expr::Lt_Expr( Expr *a, Expr *b) : Expr(1) {
    ASSERT(a->getWidth() == b->getWidth());
    _a = a;
    _b = b;
  }

string Lt_Expr::printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " < "<< _b->printExprVerilog() << ")";
    return out.str();
  };



Lte_Expr::Lte_Expr( Expr *a, Expr *b) : Expr(1) {
    ASSERT(a->getWidth() == b->getWidth());
    _a = a;
    _b = b;
  }

string Lte_Expr::printExprVerilog() {
    ASSERT2((getWidth()==1), "illegal expression width="+getWidth());
    stringstream out;
    out << "(" << _a->printExprVerilog() << " <= "<< _b->printExprVerilog() << ")";
    return out.str();
  };

  

string And_Expr::printExprVerilog() {
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


string Or_Expr::printExprVerilog() {
    ASSERT(_ins.size() > 0);
    stringstream out;
    vector<Expr*>::const_iterator i = _ins.begin();
    out << "(" << (*i)->printExprVerilog();
    for ( i++; i != _ins.end(); i++ )
      out << " | " << (*i)->printExprVerilog(); 
    out << ")";
    return out.str();
  };
