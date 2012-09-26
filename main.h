#ifndef MAIN_H
#define MAIN_H

#include <cstdlib>

#define HIER_SEPARATOR "__"
 
//for timebounds that are not assigned anything meaningful
#define T_PROP_NULL 999

#define ASSERT(x)					\
  if (! (x))						\
    {							\
      cout << "ERROR!! Assert " << #x << " failed\n";	\
      cout << " on line " << __LINE__  << "\n";		\
      cout << " in file " << __FILE__ << "\n";		\
      exit(1) ;						\
    }

#define ASSERT2(x,s)					\
  if (! (x))						\
    {							\
      cout << "ERROR!! Assert " << #x << " failed\n";	\
      cout << " " << s << "\n";				\
      cout << " on line " << __LINE__  << "\n";		\
      cout << " in file " << __FILE__ << "\n";		\
      exit(1) ;						\
    }



class Hier_Object;
class Channel;
class Network;
class Ckt;
class Primitive;
class Init_Port; //initiator of some channel
class Targ_Port; //target of some channel
class Port;
class Signal;
class Seq_Signal;
class Oracle_Signal;
class Source;
class Sink;
class Queue;
class Verification_Settings;

using namespace std;

// global variables
extern Ckt *g_ckt;         // a collections of expressions/signals/etc
extern Network *g_network; // a collection of prims/composites/channels/etc
extern ofstream g_outQos;  // to write qosReport


enum OracleType {
  ORACLE_EAGER, 
  ORACLE_DEAD, 
  ORACLE_NONDETERMINISTIC, 
  ORACLE_BOUNDED_RESPONSE,
  ORACLE_BOUNDED
};



//for queues and channels, this indicates whether to consider
//timestamps or not
enum PacketType {
  PACKET_CREDIT,
  PACKET_DATA
};

class Expr;

string itos(int i);
unsigned int numBitsRequired( unsigned int maxval);
Signal * BvIncrExprModM (Expr *a, unsigned int m, string name);
Expr * BvsubExprModM (Expr *a, Expr *b , unsigned int maxval, string name);
Signal * intervalMonitor( Expr * a, Expr * b, unsigned int t, string name);
void aImpliesBoundedFutureB( Expr * a, Expr * b, unsigned int t, string name);

string validTimeOrDash (unsigned int n);






/*! \brief a port connects a channel to a port of a primitive*/
class Port  {
public:
  Primitive *owner; // primitive that contains the port
  Channel *channel; // channel that is driven by port
  string pname;
  // /* \param n an arbitrary local name for this port within prim (e.g. "a")
  //   \param c the channel connecting to the port (from outside the owner primitive)
  //   \param p the primitive that port is being instantiated within
  // */
  Port(string n, Channel *c, Primitive *p)
    {
      pname = n;
      owner = p;
      channel = c;
    };
};

/*! \brief port acting as a target to a channel */
class Init_Port : public Port {
public:
  Init_Port(string n, Channel *c, Primitive *p);
};


// /*! \brief port acting as initiator of a channel */
class Targ_Port : public Port {
public:
  Targ_Port(string n, Channel *c, Primitive *p);
};





class Verification_Settings {
  unsigned int tMax;
 public:
  bool isEnabledPhiLQueue;
  bool isEnabledPhiGQueue;
  bool isEnabledBoundChannel; //irdy or trdy unconditional bounds
  bool isEnabledResponseBoundChannel; //irdy or trdy response bounds
  bool isEnabledPsi;
  bool isEnabledPersistance;

  Verification_Settings() { 
    tMax = T_PROP_NULL;
    isEnabledPhiLQueue = false;
    isEnabledPhiGQueue = false;
    isEnabledResponseBoundChannel = false;
    isEnabledBoundChannel = false;
    isEnabledPsi = false;
    isEnabledPersistance = false;
  };

  

  void  enableBoundChannel() { isEnabledBoundChannel = true;}
  void disableBoundChannel() { isEnabledBoundChannel = false;}
  void  enableResponseBoundChannel() { isEnabledResponseBoundChannel = true;}
  void disableResponseBoundChannel() { isEnabledResponseBoundChannel = false;}

  void  enablePersistance() { isEnabledPersistance = true;}
  void disablePersistance() { isEnabledPersistance = false;}
  void  enablePhiLQueue() { isEnabledPhiLQueue = true;}
  void disablePhiLQueue() { isEnabledPhiLQueue = false;}
  void  enablePhiGQueue() { isEnabledPhiGQueue = true;}
  void disablePhiGQueue() { isEnabledPhiGQueue = false;}
  void  enablePsi() { isEnabledPsi = true;}
  void disablePsi() { isEnabledPsi = false;}

  void setTMax( unsigned int t) {
    tMax = t;
    isEnabledPhiGQueue = true;
    return;
  }

  unsigned int getTMax() {return tMax;}
  bool hasTMax() {return tMax != T_PROP_NULL;}

  void printSettings() {
    cout 
      << "\n===================================================\n"
      << "voptions:\n"
      << "\t isEnabledPhiLQueue =                " << isEnabledPhiLQueue             << "\n"
      << "\t isEnabledPhiGQueue =                " << isEnabledPhiGQueue             << "\n"
      << "\t isEnabledResponseBoundChannel =     " << isEnabledResponseBoundChannel  << "\n"
      << "\t isEnabledBoundChannel =             " << isEnabledBoundChannel          << "\n"
      << "\t isEnabledPsi =                      " << isEnabledPsi                   << "\n"
      << "\t isEnabledPersistance =              " << isEnabledPersistance           << "\n"
      << "\t tMax =                              " << tMax                           << "\n"
      << "===================================================\n"
      ;
  }
};




class Ckt {
 public:

  Verification_Settings *voptions;
  vector<Signal*> signals;
    
  // these signals belong only to the ckt and are not within any
  // network items. they are globally available
  Seq_Signal *tCurrent;

  // the only input to ckt. All oracle signals are extracted from this.
  Oracle_Signal *oracleBus;
 
  Ckt() { 
    voptions = new Verification_Settings();
  };


  void buildNetworkLogic(Network *n);
  void dumpAsVerilog (string fname);
  void dumpAsUclid (string fname) {return;};
};



class Network {
 public:
  Network( ) {;}

  vector<Channel*>   channels; 
  vector<Primitive*> primitives; 
  vector<Source*>    sources;
  vector<Sink*>      sinks;
  vector<Queue*>     queues;

  // to keep track of propagations
  set <Channel*> modifiedChannels;

  void printQos(ostream &f);
  void printNetwork ();
  void addLatencyLemmas ();
};







/*!A Hier Object is anything in the hierarchy that contains 1 or more
  primitives or composite (e.g. credit loop) objects in its subtree.*/
class Hier_Object {
 public:
  string name; 
  vector<Hier_Object*> children; 
    
  //root not has no name... (to help keep flat names shorter)
  Hier_Object( );
  Hier_Object( const string n, Hier_Object *parent);
  void addChild (Hier_Object *x);
};



#endif

