

//#include "expressions.h"


unsigned int numBitsRequired( unsigned int maxval) {
  unsigned int x = log2(maxval);
  unsigned int w = x + 2;
  //cout << "using " << w << " bits for signal with maxval " << maxval << "\n";
  return w;
}

