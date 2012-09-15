



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


string itos(int i);




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
    tMax = 50;
  };

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




namespace network { Network *n; }
namespace logic {  Ckt *c; }


unsigned int numBitsRequired( unsigned int maxval) {
  unsigned int x = log2(maxval);
  unsigned int w = x + 2;
  //cout << "using " << w << " bits for signal with maxval " << maxval << "\n";
  return w;
}




class Channel_Qos {
  // unconditionally asserted within time bound (stronger than response)
  unsigned int targetBound;
  unsigned int initiatorBound;

  unsigned int targetResponseBound;
  unsigned int initiatorResponseBound;

  unsigned int timeToSinkBound;
  unsigned int ageBound;
  Channel *ch;

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

  void printNetwork ();
  void addLatencyLemmas ();
};






