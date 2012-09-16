



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
class Slot_Qos;

using namespace std;



// g is just a generic namespace for global vars and instances
namespace g {
  ofstream outQos;
}


namespace network { Network *n; }
namespace logic {  Ckt *c; }


//for queues and channels, this indicates whether to consider
//timestamps or not
enum PacketType {
  PACKET_CREDIT,
  PACKET_DATA
};

string itos(int i);
unsigned int numBitsRequired( unsigned int maxval);





class Verification_Settings {
  unsigned int tMax;
 public:
  bool isEnabledPhiLQueue;
  bool isEnabledPhiGQueue;
  bool isEnabledBoundChannel;
  bool isEnabledResponseBoundChannel;
  bool isEnabledPsi;
  bool isEnabledPersistance;

  Verification_Settings() { 
    isEnabledPhiLQueue = true;
    isEnabledPhiGQueue = true;
    isEnabledResponseBoundChannel   = true;
    isEnabledBoundChannel   = true;
    isEnabledPsi   = true;
    isEnabledPersistance = true;
    tMax = 20;
  };

  void  enablePhiLQueue() { isEnabledPhiLQueue = true;}
  void disablePhiLQueue() { isEnabledPhiLQueue = false;}
  void  enablePsi() { isEnabledPsi = true;}
  void disablePsi() { isEnabledPsi = false;}

  void setTMax( unsigned int t) {
    tMax = t;
    isEnabledPhiGQueue = true;
    return;
  }

  unsigned int getTMax() {return tMax;}

  void printSettings() {
    cout 
      << "\nvoptions\n"
      << "\t isEnabledPhiLQueue=                " << isEnabledPhiLQueue  << "\n"
      << "\t isEnabledPhiGQueue=                " << isEnabledPhiGQueue  << "\n"
      << "\t isEnabledResponseBoundChannel=     " << isEnabledResponseBoundChannel          << "\n"
      << "\t isEnabledBoundChannel=             " << isEnabledBoundChannel          << "\n"
      << "\t isEnabledPsi=                      " << isEnabledPsi         << "\n"
      << "\t isEnabledPersistance=              " << isEnabledPersistance << "\n"
      << "\t tMax=                              " << tMax       << "\n"
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

  // width of clk, computed to fit max latency
  unsigned int wClk; 
 
  Ckt() { 
    voptions = new Verification_Settings();
  };


  void buildNetworkLogic(Network *n);
  void dumpAsVerilog (string fname);
  void dumpAsUclid (string fname) {return;};
};






class Channel_Qos {
  // unconditionally asserted within time bound (stronger than response)
  unsigned int targetBound;
  unsigned int initiatorBound;

  unsigned int targetResponseBound;
  unsigned int initiatorResponseBound;

  unsigned int timeToSinkBound;
  unsigned int ageBound;
  Channel *ch;

  string printHeader();

 public:
  Channel_Qos(Channel *c);

  bool hasTargetResponseBound ();
  bool hasTargetBound ();
  bool hasInitiatorResponseBound ();
  bool hasInitiatorBound ();
  bool hasTimeToSinkBound ();
  bool hasAgeBound ();

  // if modifiedChannels bound is lower than current bound, add self to modified set
  void updateTargetResponseBound( unsigned int b);
  void updateInitiatorResponseBound( unsigned int b);
  void updateTargetBound( unsigned int b);
  void updateInitiatorBound( unsigned int b);
  void updateTimeToSinkBound( unsigned int t);
  void updateAgeBound( unsigned int t);

  unsigned int getTargetResponseBound ()    ;
  unsigned int getTargetBound ()            ;
  unsigned int getInitiatorResponseBound () ;
  unsigned int getInitiatorBound ()         ;
  unsigned int getTimeToSinkBound ()        ;
  unsigned int getAgeBound ()               ;

  void printChannelQos (ostream &f);

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










/*! \brief xmas kernel primitive primitives don't have internal
  channels
*/
class Primitive : public Hier_Object {
 public:
  vector <Init_Port *> init_ports;
  vector <Targ_Port *> targ_ports;
 Primitive(string n, Hier_Object *parent) : Hier_Object(n,parent) { }

  virtual void buildPrimitiveLogic ( ) =0;
  virtual void propagateLatencyLemmas() {;}
  void printConnectivity(ostream &f);
};










class Queue : public Primitive {
  PacketType type;
 public:
  Targ_Port *i;
  Init_Port *o;
  Signal *numItems;
  unsigned int numItemsMax;
  vector <Slot_Qos*> slotQos;
  vector <Signal*> qslots;
  unsigned int depth;
  Queue(Channel *in, Channel *out, unsigned int d, const string n, Hier_Object *p);
  Queue * setType ( PacketType p);
  PacketType getPacketType ();
  void buildPrimitiveLogic ( );
  void propagateLatencyLemmas( );        
};




class Slot_Qos {
  unsigned int timeToSink;
  unsigned int maxAge;
  Queue * parentQueue;
  unsigned int slotIndexInParentQueue;

  string printHeader() {
    std::stringstream out;
    out << "queue: " << setw(16) << parentQueue->name << " slot:" << setw(2) << slotIndexInParentQueue ;
    return out.str();
  };

 public:
  Slot_Qos(unsigned int i, Queue *q) {
    slotIndexInParentQueue = i;
    parentQueue = q;
    timeToSink = T_PROP_NULL;
    maxAge = T_PROP_NULL;
  }

  bool         hasTimeToSink()                 {return timeToSink != T_PROP_NULL;};
  unsigned int getTimeToSink()                 {return timeToSink;};
  bool         hasMaxAge()                     {return maxAge != T_PROP_NULL;};
  unsigned int getMaxAge()                     {return maxAge;}
  void printSlotQos(ostream &f) {
    f    << printHeader() 
	 << "  timeToSinkBound: " << setw(3)  << right << getTimeToSink()
	 << "  ageBound: "        << setw(3)  << right << getMaxAge()
	 << "\n"; 
  }


  void         setTimeToSink(unsigned int t)   { 
    timeToSink = min(timeToSink,t);
    g::outQos << printHeader() << "  set timeToSink to " << maxAge << "\n";
  }

  void         setMaxAge(unsigned int t)       { 
    maxAge = min(maxAge,t);
    g::outQos << printHeader() << "  set maxAge to " << maxAge << "\n";
  }


};
