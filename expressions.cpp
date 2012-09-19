
#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <set>


using namespace std;

#include "main.h"
#include "expressions.h"




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







