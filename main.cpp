
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <assert.h>
#include <cmath>
#include <unistd.h>

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

#include "main.h"
#include "expressions.h"

using namespace std;

    
enum OracleType {
  ORACLE_EAGER, 
  ORACLE_DEAD, 
  ORACLE_NONDETERMINISTIC, 
  ORACLE_BOUNDED_RESPONSE,
  ORACLE_BOUNDED
};




 




string itos(int i) {
  std::stringstream out;
  out << i;
  string s = out.str();
  return s;
}

unsigned int numBitsRequired( unsigned int maxval) {
  unsigned int x = log2(maxval);
  unsigned int w = x + 2;
  //cout << "using " << w << " bits for signal with maxval " << maxval << "\n";
  return w;
}



void aImpliesBoundedFutureB( Expr * a, Expr * b, unsigned int t, string name);
 


//root not has no name... (to help keep flat names shorter)
Hier_Object::Hier_Object( ) {
  name = "";        
};
Hier_Object::Hier_Object( const string n, Hier_Object *parent) {
  name = parent->name + n + HIER_SEPARATOR;
  parent->addChild(this);
};

void Hier_Object::addChild (Hier_Object *x) { children.push_back(x); }














/*! \brief connection between two xmas components.*/
class Channel : public Hier_Object {
  unsigned int width;
  PacketType type;
  bool assertIrdyPersistant;
  bool assertTrdyPersistant;
public:
  Init_Port *initiator; // (who controls the irdy/data)
  Targ_Port *target; // (who controls the trdy)
    
  Signal *irdy;
  Signal *trdy;
  Signal *data;

  Channel_Qos *qos;
  
  Channel(string n,                 Hier_Object *p) : Hier_Object(n,p) { Init(n,1,p);  }
  Channel(string n, unsigned int w, Hier_Object *p) : Hier_Object(n,p) { Init(n,w,p);  }

  void Init(string n, unsigned int w, Hier_Object *p) {
    setDataWidth(w); 
    initiator = NULL;
    target = NULL;
    assertIrdyPersistant = true;
    assertTrdyPersistant = true;
    qos = new Channel_Qos(this);
    (*network::n).channels.push_back(this);
  }


  void setPacketType(PacketType t) { type = t; return; }
  PacketType getPacketType() { return type; }

  void setDataWidth(unsigned int w) { width = w;}
  void widenData(unsigned int w) { width += w;}
  unsigned int getDataWidth() { return width;}
 
  void setIrdyPersistant() { assertIrdyPersistant = true; return; }
  void setTrdyPersistant() { assertTrdyPersistant = true; return; }
  void setPersistant() { 
    assertIrdyPersistant = true; 
    assertTrdyPersistant = true;
    return; 
  }
  void unsetPersistant() {
    assertIrdyPersistant = false;
    assertTrdyPersistant = false;
    return;
  }


  void buildChannelLogic ( ) {
    
    ASSERT(this->target!=0);    
    ASSERT(this->initiator!=0);    

    irdy = new Signal(name+"irdy");
    trdy = new Signal(name+"trdy");

    data = (new Signal(name+"data")) 
      -> setExpr((new Bvconst_Expr(1))->setWidth(width) );

    Signal *xfer = (new Signal(name+"xfer"))
      -> setExpr( (new And_Expr(irdy,trdy))  );

    Signal *blocked = (new Signal(name+"blocked"))
      -> setExpr( new And_Expr( irdy, new Not_Expr(trdy) )  );
         
    if (assertIrdyPersistant & logic::c->voptions->isEnabledPersistance) 
      { 
	
	Signal *preBlocked = (new Seq_Signal(name+"preBlocked" ))
	  ->setResetExpr( new Bvconst_Expr(0,1) )
	  ->setNxtExpr(blocked);
	
	Signal *irdyPersistant = (new Signal(name+"fwdPersistant"))
	  -> setExpr( 
		     new Or_Expr( irdy, new Not_Expr(preBlocked) ) 
		      );
	
	irdyPersistant -> assertSignalTrue();
      }

    if (assertTrdyPersistant & logic::c->voptions->isEnabledPersistance) 
      { 

	Signal *waiting = (new Signal(name+"waiting"))
	  -> setExpr( new And_Expr(trdy, new Not_Expr(irdy) ));

	Signal *preWaiting = (new Seq_Signal(name+"preWaiting"))
	  -> setResetExpr( new Bvconst_Expr(0,1) )
	  -> setNxtExpr (waiting);      
      
	Signal *trdyPersistant  = (new Signal(name+"bwdPersistant"))
	  -> setExpr( new Or_Expr( trdy, new Not_Expr(preWaiting) ) );
	
	trdyPersistant   -> assertSignalTrue();
      }

    Expr *bvtrue = new Bvconst_Expr(1,1);

    if ( qos->hasTargetResponseBound() & logic::c->voptions->isEnabledResponseBoundChannel) 
      {
	aImpliesBoundedFutureB(irdy, trdy, qos->getTargetResponseBound(), name+"TRB");
      }

    if ( qos->hasTargetBound() & logic::c->voptions->isEnabledBoundChannel ) 
      {
	aImpliesBoundedFutureB(bvtrue, trdy, qos->getTargetBound(), name+"TB");
      }

    if ( qos->hasInitiatorResponseBound() & logic::c->voptions->isEnabledResponseBoundChannel) 
      {
	aImpliesBoundedFutureB(irdy, trdy, qos->getTargetResponseBound(), name+"TRBB");
      }

    if ( qos->hasInitiatorBound() & logic::c->voptions->isEnabledBoundChannel ) 
      {
	aImpliesBoundedFutureB(bvtrue, trdy, qos->getTargetBound(), name+"TBB");
      }
    // need to add same for initiatiors





    return;
  }
};




string Channel_Qos::printHeader() {
  std::stringstream out;
  out << "channel: " << setw(16) << ch->name; 
  return out.str();
};


Channel_Qos::Channel_Qos(Channel *c) {
  ch = c;
  timeToSinkBound        = T_PROP_NULL;
  ageBound               = T_PROP_NULL;
  targetResponseBound    = T_PROP_NULL;
  initiatorResponseBound = T_PROP_NULL;
  targetBound            = T_PROP_NULL;
  initiatorBound         = T_PROP_NULL;
};


void Channel_Qos::printChannelQos (ostream &f) {
  //  f << "channel: "                 << setw(24) << left << ch->name;
  f << printHeader(); 
  f 
    << setw(27) << right << "  timeToSinkBound: "         << setw(3) << right << timeToSinkBound 
    << right << "  ageBound: "                << setw(3) << right << ageBound 
    << "\n" 
    << setw(36) << right << "  targetBound (response): "             << setw(3) << right << targetBound 
    << " ("     << setw(3) << right << targetResponseBound << ")" 
    << "\n"
    << setw(36) << right << " initiatorBound (repsonse): "          << setw(3) << right << initiatorBound 
    << " ("  << setw(3) << right << initiatorResponseBound << ")"
    << "\n";
}

bool Channel_Qos::hasTargetResponseBound ()     { return targetResponseBound    != T_PROP_NULL;  }
bool Channel_Qos::hasTargetBound ()             { return targetBound            != T_PROP_NULL;  }
bool Channel_Qos::hasInitiatorResponseBound ()  { return initiatorResponseBound != T_PROP_NULL;  }
bool Channel_Qos::hasInitiatorBound ()          { return initiatorBound         != T_PROP_NULL;  }
bool Channel_Qos::hasTimeToSinkBound ()         { return timeToSinkBound        != T_PROP_NULL;  }
bool Channel_Qos::hasAgeBound ()                { return ageBound               != T_PROP_NULL;  }

unsigned int Channel_Qos::getTargetResponseBound ()     { return targetResponseBound   ;  }
unsigned int Channel_Qos::getTargetBound ()             { return targetBound           ;  }
unsigned int Channel_Qos::getInitiatorResponseBound ()  { return initiatorResponseBound;  }
unsigned int Channel_Qos::getInitiatorBound ()          { return initiatorBound        ;  }
unsigned int Channel_Qos::getTimeToSinkBound ()         { return timeToSinkBound       ;  }
unsigned int Channel_Qos::getAgeBound ()                { return ageBound              ;  }

// if modifiedChannels bound is lower than current bound, add self to modified set
void Channel_Qos::updateTargetResponseBound( unsigned int b) {
  if (b < targetResponseBound) 
    {
      targetResponseBound = b;
      g::outQos << printHeader() << " updated targetResponseBound to: " << targetResponseBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateInitiatorResponseBound( unsigned int b) {
  if (b < initiatorResponseBound) 
    {
      initiatorResponseBound = b;
      g::outQos << printHeader() << " updated initiatorResponseBound to: " << initiatorResponseBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateTargetBound( unsigned int b) {
  if (b < targetBound) 
    {
      targetBound = b;
      g::outQos << printHeader() << " updated targetBound to: " << targetBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateInitiatorBound( unsigned int b) {
  if (b < initiatorBound) 
    {
      initiatorBound = b;
      g::outQos << printHeader() << " updated initiatorBound to: " << initiatorBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateTimeToSinkBound( unsigned int t) {
  if (t < timeToSinkBound) 
    {
      timeToSinkBound = t;
      g::outQos << printHeader() << " updated timeToSinkBound to: " << timeToSinkBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateAgeBound( unsigned int t) {
  if (t < ageBound) 
    {
      ageBound = t;
      g::outQos << printHeader() << " updated ageBound to: " << ageBound << "\n";
      network::n->modifiedChannels.insert(ch);
    } 
  return;
};








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
  Port(string n, Channel *c, Primitive *p) {
    pname = n;
    owner = p;
    channel = c;
  };
};

/*! \brief port acting as a target to a channel */
class Init_Port : public Port {
public:
  Init_Port(string n, Channel *c, Primitive *p) : Port(n, c, p) {
    ASSERT2(c->initiator == 0, "channel "+(c)->name+ " cannot have a second initiator");
    p->init_ports.push_back(this);
    c->initiator = this;
  };
};


// /*! \brief port acting as initiator of a channel */
class Targ_Port : public Port {
public:
  Targ_Port(string n, Channel *c, Primitive *p) : Port(n, c, p) {
    ASSERT2(c->target == 0, "channel "+(c)->name+ " cannot have a second target. Prev target=");
    p->targ_ports.push_back(this);
    c->target = this;
  };
};








    
void Primitive::printConnectivity(ostream &f) {

  f << "// " << this->name << "\n"; 
  for (vector <Targ_Port*>::iterator port = (this)->targ_ports.begin();port != (this)->targ_ports.end(); port++ )
    f << "//\t" << (*port)->pname << " ---> " << (*port)->channel->name << "\n";
  
  for (vector <Init_Port*>::iterator port = (this)->init_ports.begin();port != (this)->init_ports.end(); port++ )
    f << "//\t" << (*port)->pname << " <--- " << (*port)->channel->name << "\n";
  
  return;
};












class Source : public Primitive {
public:
  Init_Port *o;
  OracleType source_type;

  Source(Channel *out, string n, Hier_Object *p) : Primitive(n,p) {
    o = new Init_Port("o",out,this);
    source_type = ORACLE_NONDETERMINISTIC;
    (*network::n).primitives.push_back(this);
    (*network::n).sources.push_back(this);
  }

  void setTypeEager() { source_type = ORACLE_EAGER;}
  void setTypeDead() { source_type = ORACLE_DEAD;}
  void setTypeNondeterministic() { source_type = ORACLE_NONDETERMINISTIC;}


  void buildPrimitiveLogic ( ) {
    
    Signal *oracleIrdy;
    if (source_type == ORACLE_EAGER)
      {
	oracleIrdy = (new Signal(name+"oracleIrdy")) 
	  -> setExpr( new Bvconst_Expr(1,1));
      } 
    else if (source_type == ORACLE_DEAD) 
      { 
	oracleIrdy = (new Signal(name+"oracleIrdy"))
	  -> setExpr( new Bvconst_Expr(0,1));
      } 
    else if (source_type == ORACLE_NONDETERMINISTIC)
      {
	unsigned int lsb = logic::c->oracleBus->getWidth();
	unsigned int msb = lsb;
	logic::c->oracleBus->widen(1);
	oracleIrdy = (new Signal(name+"oracleIrdy"))
	  -> setExpr( new Extract_Expr(logic::c->oracleBus, msb, lsb) );
      }
    else  { ASSERT(0); }

    //determine the bitwidths to use
    unsigned int wOracle = o->channel->getDataWidth(); 
    unsigned int wData;
    if (o->channel->getPacketType() == PACKET_DATA)
      wData = wOracle + logic::c->wClk;
    else
      wData = wOracle;

    unsigned int lsb = logic::c->oracleBus->getWidth();
    unsigned msb = lsb + wOracle - 1;
    logic::c->oracleBus->widen( wOracle );
    Signal *oracle_data = (new Signal(name+"oracle_data"))
      -> setExpr( new Extract_Expr(logic::c->oracleBus, msb, lsb) );
    
    Signal *blocked = (new Signal(name+"blocked"))
      -> setExpr( new And_Expr( o->channel->irdy, new Not_Expr(o->channel->trdy) )  );

    Signal *preBlocked = (new Seq_Signal(name+"preBlocked"))
      -> setResetExpr( new Bvconst_Expr(0,1)  )
      -> setNxtExpr( blocked );

    Expr *oracleCatClk = new Cat_Expr(logic::c->tCurrent , oracle_data);
    Signal *pre_data = (new Seq_Signal(name+"pre_data"))
      -> setResetExpr( new Bvconst_Expr(0, wData) )
      -> setNxtExpr (oracleCatClk );
    
    o->channel->irdy
      -> setExpr( new Or_Expr( oracleIrdy, preBlocked) );

    o->channel->data
      -> setExpr ( (new Case_Expr())
		   -> setDefault( oracleCatClk )
		   -> addCase (preBlocked, pre_data)
		   );
    
    return;
  }

  void propagateLatencyLemmas( ) {

    cout << "propagating through source " << name << "\n";
    if (source_type == ORACLE_EAGER)
      {
	o->channel->qos->updateInitiatorBound(0);
	//      o->channel->updateAgeBound(0, modifiedChannels);
      }

    //     if (source_type == BND)
    //       {
    // 	o->channel->updateInitiatorResponseBound(0, modifiedChannels);
    // 	//o->channel->updateInitiatorResponseBound(0, modifiedChannels);
    //       }

 
    //    if ((source_type == ND) and (o->channel->hasTargetResponseBound() ))
    if (o->channel->qos->hasTargetResponseBound())
      {
	o->channel->qos->updateAgeBound(o->channel->qos->getTargetResponseBound() );
      }
    //    if ((source_type == ND) and (o->channel->qos->hasTargetBound() ))
    if (o->channel->qos->hasTargetBound())
      {
	o->channel->qos->updateAgeBound(o->channel->qos->getTargetBound());
      }    
    //    cout << "done checking\n";

    return;
  }
  
};




// a counter that asserts an outputs if x cycles elapse between a and b.
// used for enforcing bounds on sources and sinks, and for checking
// channel bounds.

Signal * intervalMonitor( Expr * a, Expr * b, unsigned int t, string name) {
  unsigned int w = numBitsRequired(t); 
  Signal *cntPlus1 = (new Signal(name+"cntPlus1"))->setWidth(w);

  Seq_Signal *cnt = (new Seq_Signal(name+"cnt"))->setWidth(w);

  Expr *cntEq0 = new Eq_Expr( cnt , new Bvconst_Expr(0,w) );

  //reset to 0 or stay at 0.
  Expr *nxtCntIs0 = new Or_Expr ( b , new And_Expr( new Not_Expr(a) , cntEq0));

  Signal *nxtCnt = (new Signal(name+"nxtCnt"))
    -> setExpr(  (new Case_Expr())
		 -> setDefault( cntPlus1 )
		 -> addCase(nxtCntIs0, new Bvconst_Expr(0,w))
		 );
      
  cnt 
    ->setResetExpr(  new Bvconst_Expr(0,w)  )
    ->setNxtExpr (nxtCnt);

  cntPlus1
    -> setExpr( 
	       (new Bvadd_Expr( cnt , new Bvconst_Expr(1,w) ))->setWidth(w) 
		);
	
  Signal *intervalViolated = (new Signal(name+"intervalViolated"))
    -> setExpr ( new Lt_Expr( new Bvconst_Expr(t,w) , cnt ) );
  
  return intervalViolated;
}



class Sink : public Primitive {
public:
  Targ_Port *i;
  OracleType sink_type;
  unsigned int bound;

  Sink(Channel *in, string n, Hier_Object *p) : Primitive(n,p) {
    i = new Targ_Port("i",in,this);
    sink_type = ORACLE_NONDETERMINISTIC;
    (*network::n).primitives.push_back(this);
    (*network::n).sinks.push_back(this);
  }

  void setTypeEager()      { sink_type = ORACLE_EAGER; }
  void setTypeDead()       { sink_type = ORACLE_DEAD;  } 
  void setTypeNondeterministic()         { sink_type = ORACLE_NONDETERMINISTIC;    }
  void setTypeBoundedResponse(unsigned int n)   { sink_type = ORACLE_BOUNDED_RESPONSE; bound = n; }
  void setTypeBounded(unsigned int n)   { sink_type = ORACLE_BOUNDED; bound = n; }

  void propagateLatencyLemmas( ) {
    if (sink_type == ORACLE_BOUNDED_RESPONSE) { 
      i->channel->qos->updateTargetResponseBound(bound);
      //      i->channel->updateInitiatorResponseBound(responseBound, modifiedChannels);
      //i->channel->qos->updateTargetResponseBound(responseBound, modifiedChannels);
      i->channel->qos->updateTimeToSinkBound(bound);
    }

    if (sink_type == ORACLE_BOUNDED) { 
      i->channel->qos->updateTargetBound(bound);
      //      i->channel->updateInitiatorResponseBound(responseBound, modifiedChannels);
      //i->channel->qos->updateTargetResponseBound(responseBound, modifiedChannels);
      i->channel->qos->updateTimeToSinkBound(bound);
    }


    return;
  }



  void buildPrimitiveLogic () {

    Signal *oracle_trdy;
    if (    (sink_type == ORACLE_BOUNDED_RESPONSE) 
	    or (sink_type == ORACLE_NONDETERMINISTIC) 
	    or (sink_type == ORACLE_BOUNDED) ) 
      {
	unsigned int lsb = logic::c->oracleBus->getWidth();
	unsigned int msb = lsb;
	logic::c->oracleBus->widen(1);
	oracle_trdy = (new Signal(name+"oracle_trdy"))
	  -> setExpr( new Extract_Expr(logic::c->oracleBus, msb, lsb) );
      }

    
    Signal *waiting = (new Signal(name+"waiting")) 
      -> setExpr( new And_Expr( i->channel->trdy , new Not_Expr(i->channel->irdy) ));

    Signal *pre = (new Seq_Signal(name+"preWaiting" ))
      -> setResetExpr( new Bvconst_Expr(0,1) )
      -> setNxtExpr ( waiting );       

    // the counter to enforce bounded non-det
    if (sink_type == ORACLE_BOUNDED_RESPONSE) 
      {
	Signal *blocked = (new Signal(name+"blocked"))
	  -> setExpr( new And_Expr( i->channel->irdy, new Not_Expr(     i->channel->trdy  ) )  );
	
	Signal *force_trdy = intervalMonitor(blocked, i->channel->trdy, bound-1, name);
	i->channel->trdy -> setExpr( new Or_Expr( oracle_trdy , force_trdy , pre ));
      } 


    else if (sink_type == ORACLE_BOUNDED) 
      {
	Signal *force_trdy = intervalMonitor( new Not_Expr(i->channel->trdy) , i->channel->trdy, bound-1, name);
	i->channel->trdy -> setExpr( new Or_Expr( oracle_trdy , force_trdy , pre ));
      } 

    else if (sink_type == ORACLE_BOUNDED) 
      {
	i->channel->trdy -> setExpr( new Or_Expr( oracle_trdy , pre ));
      } 
    else if (sink_type == ORACLE_EAGER) 
      {
	i->channel->trdy -> setExpr( new Bvconst_Expr(1,1) );
      } 
    else if (sink_type == ORACLE_DEAD) 
      {
	i->channel->trdy -> setExpr( new Bvconst_Expr(0,1) );
      } 
    else { ASSERT(0); }

    return;
  }


};



string Slot_Qos::printHeader() {
  std::stringstream out;
  out << "queue: " << setw(16) << parentQueue->name << " slot:" << setw(2) << slotIndexInParentQueue ;
  return out.str();
};

Slot_Qos::Slot_Qos(unsigned int i, Queue *q) {
  slotIndexInParentQueue = i;
  parentQueue = q;
  timeToSink = T_PROP_NULL;
  maxAge = T_PROP_NULL;
}

void Slot_Qos::printSlotQos(ostream &f) {
  f    << printHeader() 
       << "  timeToSinkBound: " << setw(3)  << right << getTimeToSink()
       << "  ageBound: "        << setw(3)  << right << getMaxAge()
       << "\n"; 
}

void Slot_Qos::setTimeToSink(unsigned int t)   { 
  timeToSink = min(timeToSink,t);
  g::outQos << printHeader() << "  set timeToSink to " << maxAge << "\n";
}


void Slot_Qos::setMaxAge(unsigned int t)       { 
  ASSERT(t < T_PROP_NULL);
  maxAge = min(maxAge,t);
  g::outQos << printHeader() << "  set maxAge to " << maxAge << " arg was " << t << "\n";
}

 







    
Queue::Queue(Channel *in, Channel *out, unsigned int d, const string n, Hier_Object *p) : Primitive(n,p) {
  i = new Targ_Port("i",in, this);
  o = new Init_Port("o",out,this);

  ASSERT (d >= 1); 
  depth = d;
  numItemsMax = depth;

  for (int i = 0; i<depth; i++) 
    //      slotQos.push_back (new Slot_Qos() );
    slotQos.push_back (new Slot_Qos(i,this) );
  qslots.resize(depth);
  type = PACKET_DATA;
  (*network::n).primitives.push_back(this);
  (*network::n).queues.push_back(this);
  // try switching to one-hot encoding for numItems?
}


Queue * Queue::setType ( PacketType p) {
  type = p;
  return this;
}

PacketType Queue::getPacketType () { return type;}

//void Queue::buildPrimitiveLogic ( );

void Queue::propagateLatencyLemmas( ) {
  Channel *ichan = i->channel;
  Channel *ochan = o->channel;
  
  
  if (ochan->qos->getTargetResponseBound() == 0) 
    {
      numItemsMax = 1; //eager queue should never exceed 1 item
      for (int i = depth-1; i >= 0; i--) 
	{ 
	  if (ichan->qos->hasAgeBound())	      
	    {
	      slotQos[i]->setMaxAge( 1 + ichan->qos->getAgeBound() );
	    }
	  if (ochan->qos->hasTimeToSinkBound())	      
	    {
	      slotQos[i]->setTimeToSink( 1 + ochan->qos->getTimeToSinkBound() );
	    }
	}
    } 
  else 
    {
      unsigned int slot_latency = min (ochan->qos->getTargetBound() , ochan->qos->getTargetResponseBound() + 1);
      for (int i = 0; i<depth; i++) 
	{
	  if (ichan->qos->hasAgeBound())
	    {
	      slotQos[i] -> setMaxAge( ichan->qos->getAgeBound() + slot_latency * (depth-i) );
	    }
	  if (ochan->qos->hasTimeToSinkBound())
	    {
	      slotQos[i] -> setTimeToSink( ochan->qos->getTimeToSinkBound()  + slot_latency * i );
	    }
	}
    }
  
  if (depth == 1) 
    {
      ichan->qos->updateTargetResponseBound( 1 + ochan->qos->getTargetResponseBound() ); //no simultaneous r/w
      ichan->qos->updateTargetBound( 1 + ochan->qos->getTargetBound() ); //no simultaneous r/w
    }
  else 
    {
      ichan->qos->updateTargetResponseBound( ochan->qos->getTargetResponseBound() );
      ichan->qos->updateTargetBound( ochan->qos->getTargetBound() );
    }
  
  
  ichan->qos->updateTimeToSinkBound(  slotQos[depth-1]->getTimeToSink() ); 
  ochan->qos->updateAgeBound( slotQos[0]->getMaxAge() ); 

  return;
}
        







void Ckt::buildNetworkLogic(Network *n) {


  // infer how many bits are needed for the clock 
  unsigned int maxLatency = 0;
  for (vector <Queue*>::iterator it = n->queues.begin(); it != n->queues.end(); it++ ) 
    for (int i = (*it)->qslots.size()-1 ; i >= 0 ; i--) 
      maxLatency = max(maxLatency, (*it)->slotQos[i]->getMaxAge() ); 
  wClk = numBitsRequired( max(maxLatency, voptions->getTMax() ) );
      
  cout << "\nglobal latency bound (tMax) is          " << voptions->getTMax() << "\n";
  cout << "largest time from latency lemmas is     " << maxLatency << "\n";
  cout << "clock will use                          " << wClk << " bits\n\n";


  this->oracleBus = new Oracle_Signal("oracles");
  this->tCurrent = new Seq_Signal("tCurrent");

  Signal *tCurrentNxt = (new Signal("tCurrentNxt"));
  tCurrent->setWidth(wClk);
  tCurrentNxt->setWidth(wClk);



  tCurrent 
    -> setResetExpr( (new Bvconst_Expr(0,wClk ))->setWidth(wClk) )
    -> setNxtExpr( tCurrentNxt );

  Expr *e = ( new Bvadd_Expr( tCurrent , new Bvconst_Expr(1,wClk) )) 
    -> setWidth(wClk);
  
  tCurrentNxt
    -> setExpr( e );
  
  for (vector <Channel*>::iterator it = n->channels.begin(); it != n->channels.end(); it++ )
    (*it)->buildChannelLogic();

  for (vector <Primitive*>::iterator it = n->primitives.begin(); it != n->primitives.end(); it++ )
    (*it)->buildPrimitiveLogic();

 
  return;
};








// a macro that add logic to ckt in order to check property
void aImpliesBoundedFutureB( Expr * a, Expr * b, unsigned int t, string name) {

  Signal *propertyViolated = intervalMonitor(a, b, t, name);

  Signal *propertyValid = (new Signal(name+"Valid"))
    -> setExpr ( new Not_Expr( propertyViolated ));

  propertyValid -> assertSignalTrue();

  return;
}














// returns expression for tCurrent - in, modulo max value of circular counter
Expr * AgeOfExpr (Expr *in) {

  unsigned int w = logic::c->wClk;
   
  // if clk = 0 and injection = 111...1, then age s/b 1. for rollover
  // in case 2, do (111...1 - injection + 1) so as not to use any
  // numbers exceeding 111...1
  double maxval = pow(double(2),double(w))-1;

  Expr *age_normal = new Bvsub_Expr(w, logic::c->tCurrent, in);
  Expr *overflow = new Lt_Expr( logic::c->tCurrent , in);
  Expr *x  = new Bvsub_Expr(w, new Bvconst_Expr(maxval,w), in);
  Expr *y = new Bvadd_Expr(w, new Bvconst_Expr(1,w) , x);
  Expr *age_overflow = new Bvadd_Expr(w,  logic::c->tCurrent , y );
  Expr *age = (new Case_Expr(w))
    -> setDefault(age_normal)
    -> addCase(overflow, age_overflow);

  return age;
};


// returns expression for (a - b) % maxval
Expr * BvsubExprModM (Expr *a, Expr *b , unsigned int maxval, string name) {

  unsigned int w = a->getWidth();
  ASSERT(w == b->getWidth());
  //logic::c->wClk;
   
  // if clk = 0 and injection = 111...1, then age s/b 1. for rollover
  // in case 2, do (111...1 - injection + 1) so as not to use any
  // numbers exceeding 111...1
  //  double maxval = pow(double(2),double(w))-1;

  Expr *diff_normal = new Bvsub_Expr(w, a, b);
  Expr *overflow = new Lt_Expr( a , b);
  Expr *x  = new Bvsub_Expr(w, new Bvconst_Expr(maxval,w), b);
  Expr *y = new Bvadd_Expr(w, new Bvconst_Expr(1,w) , x);
  Expr *diff_overflow = new Bvadd_Expr(w,  a , y );
  Expr *diff = (new Case_Expr(w))
    -> setDefault(diff_normal)
    -> addCase(overflow, diff_overflow);

  return diff;
};

// returns expression for (a+1) % maxval
Signal * BvIncrExprModM (Expr *a, unsigned int m, string name) {

  unsigned int w = a->getWidth();
  ASSERT(w >= numBitsRequired(m));

  Expr *rollover = new Eq_Expr( a , new Bvconst_Expr(m-1,w) );
  Expr *plus1 = new Bvadd_Expr(w, a , new Bvconst_Expr(1,w));  
  
  Signal *incrModM = (new Signal(name+"incrModM"))
    -> setExpr( (new Case_Expr())		 
		-> setDefault(plus1)
		-> addCase(rollover, new Bvconst_Expr(0,w) ) -> setWidth(w)
		);


  return incrModM;
};





void Queue::buildPrimitiveLogic ( ) {


  //check the base size (without timestamp), and also the type
  ASSERT( i->channel->getDataWidth() == o->channel->getDataWidth() );
  ASSERT( getPacketType() == i->channel->getPacketType() );
  ASSERT( getPacketType() == o->channel->getPacketType() );
  ASSERT( depth == qslots.size() );

  unsigned int wDepth = numBitsRequired(depth); 
  unsigned int wPacket = i->channel->getDataWidth();
  pair <unsigned int,unsigned int> tBits; //which bits hold timestamp

  if (getPacketType() == PACKET_DATA)
    {
      wPacket += logic::c->wClk; 
      tBits = make_pair(wPacket-1, i->channel->getDataWidth()); //msb is first
    } 

  

  for (unsigned int j = 0; j<depth; j++) 
    qslots[j] = (new Signal(name+"qslots"+itos(j)) );
    
  Signal *nxt_numItems = (new Signal(name+"nxt_numItems"));

  numItems = (new Seq_Signal(name+"numItems"))
    -> setResetExpr( (new Bvconst_Expr(0))->setWidth(wDepth) )
    -> setNxtExpr( nxt_numItems );

  Signal *numItemsPlus1 = (new Signal(name+"numItemsPlus1"))
    -> setExpr( new Bvadd_Expr( wDepth, numItems, new Bvconst_Expr(1,wPacket))); 

  Signal *isEmpty = (new Signal(name+"isEmpty"))
    -> setExpr( new Eq_Expr( new Bvconst_Expr(0,wDepth) , numItems ) );

  Signal *isFull  = (new Signal(name+"isFull"))
    -> setExpr( new Eq_Expr( new Bvconst_Expr(depth,wDepth) , numItems ) );

  //define the local signals and connect them to channel signals as needed
  Signal *i_irdy = new Signal(name+"i_irdy");
  Signal *i_data = new Signal(name+"i_data");
  Signal *i_trdy = (new Signal(name+"i_trdy"))
    -> setExpr( new Not_Expr(isFull) );

  i->channel->trdy     -> setExpr( (new Id_Expr(i_trdy ))->setWidth(1) );
  i_irdy               -> setExpr( (new Id_Expr(i->channel->irdy ))->setWidth(1) );
  i_data               -> setExpr( (new Id_Expr(i->channel->data))->setWidth(wPacket) );

  Signal *o_data = new Signal(name+"o_data");
  Signal *o_trdy = new Signal(name+"o_trdy");
  Signal *o_irdy = (new Signal(name+"o_irdy"))
    -> setExpr(new Not_Expr(isEmpty));
 
  o->channel->data     -> setExpr( (new Id_Expr( o_data )) ->setWidth(wPacket)  );
  o->channel->irdy     -> setExpr( (new Id_Expr( o_irdy )) ->setWidth(1) );
  o_trdy               -> setExpr( (new Id_Expr( o->channel->trdy )) -> setWidth(1) );

  Signal *readEnable   = (new Signal(name+"readEnable"))
    -> setExpr( new And_Expr(o_irdy, o_trdy));

  Signal *writeEnable   = (new Signal(name+"writeEnable"))
    -> setExpr( new And_Expr(i_irdy, i_trdy));


  ////////////////////////////////////////////////////
  // keep track of the number of items in the queue //
  ////////////////////////////////////////////////////

  Signal *incrNumItems = (new Signal(name+"incrNumItems"))
    -> setExpr( new And_Expr( writeEnable, new Not_Expr(readEnable) ) );

  Signal *decrNumItems = (new Signal(name+"decrNumItems"))
    -> setExpr( new And_Expr( readEnable, new Not_Expr(writeEnable) ) );

  Signal *numItemsMinus1 = (new Signal(name+"numItemsMinus1"))
    -> setExpr( (new Bvsub_Expr(numItems, new Bvconst_Expr(1,wDepth)))->setWidth(wDepth) );

  nxt_numItems 
    -> setExpr( (new Case_Expr())
		-> setDefault(numItems)
		-> addCase(decrNumItems, numItemsMinus1)
		-> addCase(incrNumItems, numItemsPlus1) 
		-> setWidth(wDepth)
		);
		 





  //////////////////////////////////
  // manage the tail of the queue //
  //////////////////////////////////
  Signal *nxt_tail = (new Signal(name+"nxtTail"));

  Signal *tail = (new Seq_Signal(name+"tail"))
    -> setResetExpr( (new Bvconst_Expr(0)) -> setWidth(wDepth) )
    -> setNxtExpr (nxt_tail);
  
//   Expr *expr_tail_plus_1 = new Bvadd_Expr(wDepth, tail, new Bvconst_Expr(1,wDepth));  
//   Expr *eq_mod = new Eq_Expr( tail, new Bvconst_Expr(depth-1,wDepth) );
  
//   Signal *tail_plus_1 = (new Signal(name+"tailPlus1Mod"))
//     -> setExpr( (new Case_Expr())		 
// 		-> setDefault(expr_tail_plus_1)
// 		-> addCase(eq_mod, new Bvconst_Expr(0,wDepth) ) -> setWidth(wDepth)

// 		);
  Signal *tail_plus_1 = BvIncrExprModM(tail, depth, name);
  


  nxt_tail
    -> setName(name+"nxt_tail")
    -> setExpr( (new Case_Expr())
		-> setDefault(tail)
		-> addCase(writeEnable, tail_plus_1)
		-> setWidth(wDepth)
		);

  //////////////////////////////////
  // manage the head of the queue //
  //////////////////////////////////
  Signal *nxt_head     = new Signal(name+"nxtHead"); 
  Signal *head = (new Seq_Signal(name+"head"))
    -> setResetExpr( new Bvconst_Expr(0,wDepth) )
    -> setNxtExpr ( nxt_head );

//   Expr *expr_head_plus_1 = new Bvadd_Expr( wDepth, head, new Bvconst_Expr(1,wDepth));  
//   Expr *head_eq_max = new Eq_Expr( head, new Bvconst_Expr(depth-1,wDepth) );
  
//   Signal *head_plus_1 = (new Signal())
//     -> setName(name+"head_plus1mod")
//     -> setExpr( (new Case_Expr())		 
// 		-> setDefault(expr_head_plus_1)
// 		-> addCase(head_eq_max, new Bvconst_Expr(0,wDepth) )
// 		-> setWidth(wDepth)
// 		);

  Signal *head_plus_1 = BvIncrExprModM(head, depth, name+"headPlus1ModM");


  nxt_head 
    -> setName (name+"nxt_head")
    -> setExpr ( (new Case_Expr())
		 -> setDefault(head)
		 -> addCase(readEnable, head_plus_1)
		 -> setWidth(wDepth)
		 );
		  


  //////////////////////////////////
  // manage the queue entries     //
  //////////////////////////////////
  Signal *bvc_null_pkt = (new Signal())
    -> setName("nullPkt")
    -> setExpr( new Bvconst_Expr(0,wPacket) );
                                          

  vector <Seq_Signal *> bslots (depth);
  for (unsigned int i = 0; i<depth; i++) 
    {
      bslots[i] = new Seq_Signal(name+"bslot"+itos(i));
      bslots[i] ->setWidth(wPacket);

      Signal *nxt_buffer      = (new Signal())
	-> setName(name+"bslot"+itos(i)+"nxt");

      Signal *bvi = (new Signal()) 
	-> setName( name+"const"+itos(i) )
	-> setExpr( (new Bvconst_Expr(i)) -> setWidth(wDepth) );

      Signal *is_tail = (new Signal())
	-> setName(name+"bslot"+itos(i)+"is_tail")
	-> setExpr( new Eq_Expr(bvi, tail));


      Signal *write_bslot = (new Signal())
	-> setName( name+"bslot"+itos(i)+"write" )
	-> setExpr( (new And_Expr())		   
		    -> addInput (writeEnable)
		    -> addInput (is_tail)
		    );
             
      Signal *is_head = (new Signal())
	-> setName( name+"bslot"+itos(i)+"is_head" )
	-> setExpr( new Eq_Expr(bvi, head) );


      Signal *clear_bslot = (new Signal())
	-> setName(name+"bslot"+itos(i)+"clear")
	-> setExpr( (new And_Expr())
		    -> addInput (is_head)
		    -> addInput (readEnable)
		    -> addInput (new Not_Expr(write_bslot))
		    );

      nxt_buffer 
	-> setExpr( (new Case_Expr())
		    -> setDefault(bslots[i])
		    -> addCase(clear_bslot, bvc_null_pkt)
		    -> addCase(write_bslot, i_data)
		    -> setWidth(wPacket)
		    );
		   
      bslots[i] -> setResetExpr(bvc_null_pkt);
      bslots[i] -> setNxtExpr(nxt_buffer);

    }

  // map the circular buffer slots to their fifo indexed slots.
  for (unsigned int q = 0; q<depth; q++) 
    {
      Case_Expr *e = new Case_Expr();
      e -> setWidth(wDepth);
      e -> setDefault(bvc_null_pkt);
      
      for (unsigned int b = 0; b<depth; b++) 
	{
	  // b_eq_q means: should the bth slot in circular buffer map to the qth slot in fifo queue
	  unsigned int hpos = (depth+b-q)%depth; //slot q maps to slot b when head=hpos            
	  Expr * q_eq_b = new Eq_Expr(head, new Bvconst_Expr(hpos,wDepth));
	  e -> addCase(q_eq_b, bslots[b]);
	}
      qslots[q]	-> setExpr( e );
      
    }
  
  o_data    ->setExpr( (new Id_Expr( qslots[0]))->setWidth(wPacket) );

  if (logic::c->voptions->isEnabledPsi) 
    {
      Signal *numItemsValid = (new Signal())
	-> setName( name+"numItemsValid" )
	-> setExpr( new Lte_Expr( 
				 numItems ,
				 new Bvconst_Expr(numItemsMax,wDepth)
				  )
		    );
      numItemsValid  -> assertSignalTrue();

      // need to handle ambigious case where head and tail point to same thing
      Signal *headTailDistance = (new Signal())
	-> setName( name+"headTailDistance" )
	-> setExpr( BvsubExprModM( tail, head, depth-1, name+"headTailDistance") );
      
      Signal *headTailDistanceValid = (new Signal(name+"headTailDistanceValid"))
	-> setExpr( new Eq_Expr(headTailDistance, numItems));

      headTailDistanceValid -> assertSignalTrue();


    }


  if (type == PACKET_DATA)
    {  
      // create all the ages to be checked below
      vector <Signal*> age (depth);
      for (unsigned int i = 0; i<depth; i++) 
	{

	  Signal *hasData  = (new Signal())
	    -> setName(name+"qslot"+itos(i)+"_hasdata")
	    -> setExpr( new Lt_Expr( new Bvconst_Expr(i,wDepth),  numItems ));

	  Signal *qSlotTimestamp = (new Signal())
	    -> setName(name+"qSlot"+itos(i)+"Timestamp")
	    -> setExpr( new Extract_Expr(qslots[i], tBits.first, tBits.second));
	  
	  double maxval = pow(double(2),double(logic::c->wClk))-1;

	  Signal *agex  = (new Signal(name+"ageIfValid"))
	    -> setExpr( 
		       BvsubExprModM( 
				     logic::c->tCurrent, 
				     qSlotTimestamp , 
				     maxval , 
				     name+"qSlot"+itos(i)+"currentMinusTimestamp" 
				      ) 
			);
	  //-> setExpr( AgeOfExpr(qSlotTimestamp)  );

	  age[i] = (new Signal())
	    -> setName(name+"qslot"+itos(i)+"_age")
	    -> setExpr( (new Case_Expr())
			-> setDefault( new Bvconst_Expr(0,logic::c->wClk) )
			-> addCase( hasData, agex)
			-> setWidth(logic::c->wClk)
			);

	}

      if (logic::c->voptions->isEnabledPhiLQueue) 
	{
	  for (unsigned int i = 0; i<depth; i++) 
	    {
	  
	      Signal *age_bound = (new Signal())
		-> setName( name+"qslot"+itos(i)+"phiLAgeBound")
		-> setExpr( (new Bvconst_Expr(slotQos[i]->getMaxAge() + 1)) -> setWidth(logic::c->wClk) );
	  
	      Signal *age_valid = (new Signal())
		-> setName( name+"qslot"+itos(i)+"phiLValid" )
		-> setExpr( new Lt_Expr( age[i] , age_bound) );
	  
	      age_valid -> assertSignalTrue();
	    }
	}

      if (logic::c->voptions->isEnabledPhiGQueue) 
	{
	  for (unsigned int i = 0; i<depth; i++) 
	    {

	      Signal *age_bound = (new Signal())
		-> setName(name+"qslot"+itos(i)+"phiGAgebound")
		-> setExpr( (new Bvconst_Expr( logic::c->voptions->getTMax())) -> setWidth(logic::c->wClk) );

	      Signal *age_valid = (new Signal())
		-> setName( name+"qslot"+itos(i)+"phiGValid" )
		-> setExpr( new Lt_Expr( age[i] , age_bound) );

	      age_valid -> assertSignalTrue();
	    }
	}
    }  
  
  return;
}





// print the QoS for all the channels and queue slots
void Network::printQos( ostream &f) 
{
  f << "\n===============================================================================\n";
  f << "Qos Report \n";
  for (vector <Channel*>::iterator it = (this)->channels.begin(); it != (this)->channels.end(); it++ ) 
    (*it)->qos->printChannelQos(f);

  for (vector <Queue*>::iterator it = (this)->queues.begin(); it != (this)->queues.end(); it++ )
    for (int i = (*it)->qslots.size()-1 ; i >= 0 ; i--) 
      (*it)->slotQos[i]->printSlotQos(f);
  f << "===============================================================================\n";
  return;
}

void Network::addLatencyLemmas() 
{

  g::outQos.open("dump.qos");

  // initially only the sources/sinks need to be updated with own qos guarantees
  for (vector <Sink*>::iterator it = sinks.begin(); it != sinks.end(); it++ ) 
    (*it)->propagateLatencyLemmas();
  for (vector <Source*>::iterator it = sources.begin(); it != sources.end(); it++ ) 
    (*it)->propagateLatencyLemmas();
    
  cout << "after sinks, number updated channels is " << network::n->modifiedChannels.size() << "\n\n";

  // modifiedChannels keeps track of what signals channels need to propagate
  
  unsigned int cnt = 0;
  while (network::n->modifiedChannels.size() > 0) 
    {

      g::outQos << "\n on iter " << cnt
		<< " modifiedChannels.size = " << network::n->modifiedChannels.size(); 
      for (set<Channel*>::iterator it = network::n->modifiedChannels.begin(); 
	   it != network::n->modifiedChannels.end(); it++) 
	{
	  g::outQos << "\n\t" << (*it)->name ;
	}
      g::outQos << "\n";


      set<Channel*>::const_iterator c  = network::n->modifiedChannels.begin();
      network::n->modifiedChannels.erase(*c);
        
      Primitive *iprim = (*c)->initiator->owner;
      Primitive *tprim = (*c)->target->owner;
      iprim->propagateLatencyLemmas( );
      tprim->propagateLatencyLemmas( );

      if (cnt++ >1000) ASSERT(0); 
    }

  printQos(cout);
  printQos(g::outQos);
  

  //  cout << "\ninferred numeric invariant properties \n";
  cout << "\n";
  g::outQos.close();
  return;
}








/*! \param fname the name of the file where the verilog will be
 *  printed*/
void Ckt::dumpAsVerilog (string fname) {
  
  if (0) 
    {
      for (vector <Signal*>::iterator it = signals.begin();it != signals.end(); it++ )
	cout << "name: " << setw(35) << (*it)->name << "\t width: " << setw(3) << (*it)->getWidth() << "\n";
    }

  ofstream  vfile;
  vfile.open(fname.c_str());
  vfile << "\n";
  vfile << "`timescale 1ns/1ps                                            \n";
  vfile << "\nmodule xmas_assert(clk, reset, cond );                      \n";
  vfile << "input clk;                                                    \n";
  vfile << "input reset;                                                  \n";
  vfile << "input cond;                                                   \n";    
  vfile << "`ifdef SIMULATION_ONLY                                        \n";
  vfile << "always @(posedge clk)                                         \n";
  vfile << "begin                                                         \n";
  vfile << " if (~reset & ~cond)                                          \n";
  vfile << "  begin                                                       \n";
  vfile << "   $display(\"assertion\t %m ends sim at time: %5d\",$time);  \n";
  vfile << "   $finish;	                                                  \n";
  vfile << "  end                                                         \n";
  vfile << "end                                                           \n";
  vfile << " `else                                                        \n";
  vfile << "property ax;                                                  \n";
  vfile << " @(posedge clk) disable iff (reset) (cond);                   \n";
  vfile << "endproperty                                                   \n";
  vfile << "p_ax: assert property(ax);                                    \n";
  vfile << "`endif                                                        \n";
  vfile << "endmodule                                                     \n";
  vfile << "\n\n\n";

  unsigned int num_oracles = oracleBus->getWidth();

  vfile << "`define NUM_ORACLES " << num_oracles << "\n"; //test bench uses this

  vfile << "\n\nmodule nut(  clk, reset, oracles );\n";
  vfile << "input \t\t clk;\n";
  vfile << "input \t\t reset;\n";
  vfile << "input ["+itos(num_oracles-1)+":0] \t oracles; \n";
  vfile << "\n\n";

  for (vector <Signal*>::iterator it = signals.begin();it != signals.end(); it++ )
    (*it)->declareSignalVerilog(vfile);
  vfile << "\n\n";

  for (vector <Signal*>::iterator it = signals.begin();it != signals.end(); it++ )
    (*it)->assignSignalVerilog(vfile);
  vfile << "\n\n";
      
  vfile << "endmodule\n";
  cout << "finished dumping verilog to " << fname << "\n";
  return;    
};










/*! \brief hier_object containing primitives and channels inside */
class Composite : public Hier_Object {
public:
  Composite(string n, Hier_Object *p) : Hier_Object(n,p) {}
  Composite() : Hier_Object() {}
};




class Merge : public Primitive {
public:
  Targ_Port *a;
  Targ_Port *b;
  Init_Port *o;

  Signal *u;
    
  Merge(Channel *in1, Channel *in2, Channel *out, const string n, Hier_Object *p) : Primitive(n,p) {
    a = new Targ_Port("a",in1, this);
    b = new Targ_Port("b",in2, this);
    o = new Init_Port("o",out, this);

    (*network::n).primitives.push_back(this);

    // try switching to one-hot encoding for numItems?
    //u = new Signal(name+"u", 1,  this);
  }

  void propagateLatencyLemmas( ) {
    
    if (o->channel->qos->hasTargetResponseBound() )
      {
	int n = 1 + 2 * o->channel->qos->getTargetResponseBound();
	a->channel->qos->updateTargetResponseBound( n );
	b->channel->qos->updateTargetResponseBound( n );

	if (o->channel->qos->hasTimeToSinkBound())
	  {
	    int m = o->channel->qos->getTimeToSinkBound() + n;
	    a->channel->qos->updateTimeToSinkBound( m );
	    b->channel->qos->updateTimeToSinkBound( m );
	  }
      }

    if (o->channel->qos->hasTargetBound() )
      {
	int n = 1 + 2 * o->channel->qos->getTargetBound();
	a->channel->qos->updateTargetBound( n );
	b->channel->qos->updateTargetBound( n );

	if (o->channel->qos->hasTimeToSinkBound())
	  {
	    int m = n + o->channel->qos->getTimeToSinkBound();
	    a->channel->qos->updateTimeToSinkBound( m );
	    b->channel->qos->updateTimeToSinkBound( m );
	  }
      }

    if (a->channel->qos->hasAgeBound() and b->channel->qos->hasAgeBound() )
      {
	int n = max ( a->channel->qos->getAgeBound() , b->channel->qos->getAgeBound() );
	o->channel->qos->updateAgeBound(n);
      }

    return;
  }

  void buildPrimitiveLogic ( ) {

    unsigned int w = o->channel->getDataWidth();
    ASSERT (w == a->channel->getDataWidth());
    ASSERT (w == b->channel->getDataWidth());
  
    Signal *a_irdy = new Signal(name+"a_irdy" );
    Signal *a_data = new Signal(name+"a_data" );
    Signal *a_trdy = new Signal(name+"a_trdy" );

    a_irdy             -> setExpr( new Id_Expr(1     , a->channel->irdy) );
    a_data             -> setExpr( new Id_Expr(w , a->channel->data) );
    a->channel->trdy   -> setExpr( new Id_Expr(1     , a_trdy));

    Signal *b_irdy = new Signal(name+"b_irdy" );
    Signal *b_data = new Signal(name+"b_data" );
    Signal *b_trdy = new Signal(name+"b_trdy" );
    
    b_irdy            -> setExpr( new Id_Expr(1     , b->channel->irdy ) );
    b_data            -> setExpr( new Id_Expr(w , b->channel->data ) );
    b->channel->trdy  -> setExpr( new Id_Expr(1     , b_trdy) );
    
    Signal *o_irdy = new Signal(name+"o_irdy" );
    Signal *o_data = new Signal(name+"o_data" );
    Signal *o_trdy = new Signal(name+"o_trdy" );

    o->channel->data   -> setExpr( new Id_Expr(w , o_data) );
    o->channel->irdy   -> setExpr( new Id_Expr(1     , o_irdy) );
    o_trdy             -> setExpr( new Id_Expr(1     , o->channel->trdy ) );
    o_irdy             -> setExpr( new Or_Expr( a_irdy , b_irdy) );
        
    Signal *u_nxt = new Signal(name+"u_nxt");
        
    u = (new Seq_Signal(name+"u"))
      -> setResetExpr( new Bvconst_Expr(0,1) )
      -> setNxtExpr( u_nxt );
    
 
    // at = ai & ot & ( u | ~bi)
    // bt = bi & ot & (~u | ~ai)

    Signal *u_or_nbi = (new Signal())
      -> setName (name+"u_or_nbi")
      -> setExpr( new Or_Expr(u, new Not_Expr(b_irdy)) );

    Signal *nu_or_nai = (new Signal())
      -> setName (name+"nu_or_nai")
      -> setExpr( new Or_Expr( new Not_Expr(u), new Not_Expr(a_irdy) ) );
    
    a_trdy   -> setExpr( (new And_Expr())
			 -> addInput (u_or_nbi)
			 -> addInput (o_trdy)
			 -> addInput (a_irdy)
			 );
    
    b_trdy   -> setExpr( (new And_Expr())
			 -> addInput (nu_or_nai)
			 -> addInput (o_trdy)
			 -> addInput (b_irdy)
			 );
		   

    o_data   -> setExpr ( (new Case_Expr(w))
			  -> setDefault( new Bvconst_Expr(0,w) )
			  -> addCase( b_trdy , b_data)
			  -> addCase( a_trdy , a_data)
			  );
			  
    u_nxt    -> setExpr( (new Case_Expr(1))
			 -> setDefault( u )
			 -> addCase( b_trdy , new Bvconst_Expr(1,1) )
			 -> addCase( a_trdy , new Bvconst_Expr(0,1) )
			 );


    return;
  }
        
};




void Network::printNetwork () {

  cout << "starting printNetwork()\n";
        
  printf("network has %d channels\n", (int)channels.size()  );
  for (vector <Channel*>::iterator it = channels.begin(); it != channels.end(); it++ )
    cout << "\t" << (*it)->name << "\n";

  printf("network has %d primitives \n", (int)primitives.size()  );
  for (vector <Primitive*>::iterator it = primitives.begin(); it != primitives.end(); it++ )
    cout << "\t" << (*it)->name << "\n";

  printf("network has %d sources \n", (int)sources.size()  );
  for (vector <Source*>::iterator it = sources.begin(); it != sources.end(); it++ )
    cout << "\t" << (*it)->name << "\n";

  printf("network has %d sinks \n", (int)sinks.size()  );
  for (vector <Sink*>::iterator it = sinks.begin(); it != sinks.end(); it++ )
    cout << "\t" << (*it)->name << "\n";

  printf("network has %d queues \n", (int)queues.size()  );
  for (vector <Queue*>::iterator it = queues.begin(); it != queues.end(); it++ )
    cout << "\t" << (*it)->name << "\n";

  cout << "finishing printNetwork()\n\n";

    
  return;
}









class Join : public Primitive {
public:

  Targ_Port *a;
  Targ_Port *b;
  Init_Port *o;

  Join(Channel *in1, Channel *in2, Channel *out, string n, Hier_Object *p) : Primitive(n,p) {
    a = new Targ_Port("a",in1,this);
    b = new Targ_Port("b",in2,this);
    o = new Init_Port("o",out,this);
    (*network::n).primitives.push_back(this);
  }


  void propagateLatencyLemmas( ) {

    cout << "propagating through join " << name << "\n";

    if (a->channel->qos->hasInitiatorBound() and b->channel->qos->hasInitiatorBound() )
      {
	int n = max (a->channel->qos->getInitiatorBound() , b->channel->qos->getInitiatorBound() );
	o->channel->qos->updateInitiatorBound( n );
      }

    Port *aa;
    Port *bb;

    for (int i = 0; i <=1; i++) 
      {
	if (i==0) { aa=a; bb=b;} else {aa=b; bb=a;}
	// all the bounds to set on aa:
	if (bb->channel->qos->hasInitiatorBound() and o->channel->qos->hasTargetBound() )
	  {
	    int n = max (bb->channel->qos->getInitiatorBound() , o->channel->qos->getTargetBound() );
	    aa->channel->qos->updateTargetBound( n );
	  }
	
	if (bb->channel->qos->hasInitiatorBound() and o->channel->qos->hasTargetResponseBound() )
	  {
	    int n = max (bb->channel->qos->getInitiatorBound() , o->channel->qos->getTargetResponseBound());
	    aa->channel->qos->updateTargetResponseBound( n );
	  }
	
	if (o->channel->qos->hasTimeToSinkBound() and aa->channel->qos->hasInitiatorResponseBound() )
	  {
	    int n = o->channel->qos->getTimeToSinkBound() + aa->channel->qos->getInitiatorResponseBound();
	    aa->channel->qos->updateTimeToSinkBound(n );
	  }

	if (o->channel->qos->hasTimeToSinkBound() and aa->channel->qos->hasInitiatorBound() )
	  {
	    int n = o->channel->qos->getTimeToSinkBound() + aa->channel->qos->getInitiatorBound();
	    aa->channel->qos->updateTimeToSinkBound(n );
	  }	
      }

    return;
  }




  void buildPrimitiveLogic ( ) {

    unsigned int w = o->channel->getDataWidth();
    ASSERT (w == a->channel->getDataWidth() );
    ASSERT (w == b->channel->getDataWidth() );

    Signal *a_irdy = new Signal(name+"a_irdy");
    Signal *a_data = new Signal(name+"a_data");
    Signal *a_trdy = new Signal(name+"a_trdy");

    a_irdy            -> setExpr( new Id_Expr(1 , a->channel->irdy ) );
    a_data            -> setExpr( new Id_Expr(w , a->channel->data ) );
    a->channel->trdy  -> setExpr( new Id_Expr(1 , a_trdy           ) );

    Signal *b_irdy = new Signal(name+"b_irdy");
    Signal *b_data = new Signal(name+"b_data");
    Signal *b_trdy = new Signal(name+"b_trdy");
    
    b_irdy            -> setExpr( new Id_Expr(1 , b->channel->irdy ) );
    b_data            -> setExpr( new Id_Expr(w , b->channel->data ) );
    b->channel->trdy  -> setExpr( new Id_Expr(1 , b_trdy           ) );
    
    Signal *o_irdy = new Signal(name+"o_irdy");
    Signal *o_data = new Signal(name+"o_data");
    Signal *o_trdy = new Signal(name+"o_trdy");

    o->channel->data   -> setExpr( new Id_Expr(w , o_data           ) );
    o->channel->irdy   -> setExpr( new Id_Expr(1 , o_irdy           ) );
    o_trdy             -> setExpr( new Id_Expr(1 , o->channel->trdy ) );

    // the logic for the join:
    o_data       -> setExpr ( new Id_Expr( w , a_data) );
    o_irdy       -> setExpr ( new And_Expr( a_irdy , b_irdy) );
    a_trdy       -> setExpr ( new And_Expr( o_trdy , b_irdy) );
    b_trdy       -> setExpr ( new And_Expr( o_trdy , a_irdy) );    		    
        
    return;
  }

};





class Fork : public Primitive {
public:
  Targ_Port *portI;
  Init_Port *portA;
  Init_Port *portB;

  Fork(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p) : Primitive(n,p) {
    portI = new Targ_Port("i",in,this);
    portA = new Init_Port("a",out1,this);
    portB = new Init_Port("b",out2,this);
    (*network::n).primitives.push_back(this);
  }

  void propagateLatencyLemmas( ) {

    if (portA->channel->qos->hasTargetBound() and portB->channel->qos->hasTargetBound() )
      {
     	int n = max (portA->channel->qos->getTargetBound() , portB->channel->qos->getTargetBound() );
     	portA->channel->qos->updateTargetResponseBound( n );
      }

    Port *aa;
    Port *bb;


    for (int i = 0; i <=1; i++) 
      {
	if (i==0) { aa=portA; bb=portB;} else {aa=portB; bb=portA;}
	// all the bounds to set on aa:
	if (portI->channel->qos->hasInitiatorBound() and bb->channel->qos->hasTargetBound() )
	  {
	    int n = max (portA->channel->qos->getInitiatorBound() , bb->channel->qos->getTargetBound() );
	    aa->channel->qos->updateInitiatorBound( n );
	  }

	// 	if (portI->channel->qos->hasInitiatorBound() and bb->channel->hasTargetResponseBound() )
	// 	  {
	// 	    int n = max (portA->channel->initiatorBound , bb->channel->targetBound);
	// 	    aa->channel->qos->updateInitiatorBound( n , modifiedChannels);
	// 	  }
	
	// 	if (bb->channel->qos->hasInitiatorBound() and o->channel->hasTargetResponseBound() )
	// 	  {
	// 	    int n = max (bb->channel->initiatorBound , o->channel->targetResponseBound);
	// 	    aa->channel->qos->updateTargetResponseBound( n , modifiedChannels);
	// 	  }
	
	// 	if (o->channel->qos->hasTimeToSinkBound() and aa->channel->hasInitiatorResponseBound() )
	// 	  {
	// 	    int n = o->channel->timeToSinkBound + aa->channel->initiatorResponseBound;
	// 	    aa->channel->qos->updateTimeToSinkBound(n , modifiedChannels);
	// 	  }

	// 	if (o->channel->qos->hasTimeToSinkBound() and aa->channel->qos->hasInitiatorBound() )
	// 	  {
	// 	    int n = o->channel->timeToSinkBound + aa->channel->initiatorBound;
	// 	    aa->channel->qos->updateTimeToSinkBound(n , modifiedChannels);
	// 	  }	
      }

    return;
  }




  void buildPrimitiveLogic ( ) {

    unsigned int w = portI->channel->getDataWidth();
    ASSERT (w == portA->channel->getDataWidth());
    ASSERT (w == portB->channel->getDataWidth());
    
    Signal *i_irdy = new Signal(name+"i_irdy");
    Signal *i_data = new Signal(name+"i_data");
    Signal *i_trdy = new Signal(name+"i_trdy");
    
    i_irdy                -> setExpr( new Id_Expr(1 , portI->channel->irdy ) );
    i_data                -> setExpr( new Id_Expr(w , portI->channel->data ) );
    portI->channel->trdy  -> setExpr( new Id_Expr(1 , i_trdy           ) );
     
    Signal *a_irdy = new Signal(name+"a_irdy");
    Signal *a_data = new Signal(name+"a_data");
    Signal *a_trdy = new Signal(name+"a_trdy");

    portA->channel->data   -> setExpr( new Id_Expr(w , a_data           ) );
    portA->channel->irdy   -> setExpr( new Id_Expr(1 , a_irdy           ) );
    a_trdy                 -> setExpr( new Id_Expr(1 , portA->channel->trdy ) );

    Signal *b_irdy = new Signal(name+"b_irdy");
    Signal *b_data = new Signal(name+"b_data");
    Signal *b_trdy = new Signal(name+"b_trdy");

    portB->channel->data   -> setExpr( new Id_Expr(w , b_data           ) );
    portB->channel->irdy   -> setExpr( new Id_Expr(1 , b_irdy           ) );
    b_trdy                 -> setExpr( new Id_Expr(1 , portB->channel->trdy ) );

    // logic for fork:
    a_irdy        -> setExpr ( new And_Expr( i_irdy , b_trdy) );
    b_irdy        -> setExpr ( new And_Expr( i_irdy , a_trdy) );
    a_data        -> setExpr ( new Id_Expr(w, i_data));
    b_data        -> setExpr ( new Id_Expr(w, i_data));
    i_trdy        -> setExpr ( new And_Expr( a_trdy , b_trdy ));
    return;
  }

};




class Switch : public Primitive {
public:
  Targ_Port *portI;
  Init_Port *portA;
  Init_Port *portB;

  Switch(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p) : Primitive(n,p) {
    portI = new Targ_Port("i",in,this);
    portA = new Init_Port("a",out1,this);
    portB = new Init_Port("b",out2,this);

    (*network::n).primitives.push_back(this);
  }

  //   void buildNetworkObject ( Network *n) {
  //     (*n).primitives.push_back(this);
  //     return;
  //   }

  void propagateLatencyLemmas( ) {

    //     if (portA->channel->qos->hasTargetBound() and portB->channel->qos->hasTargetBound() )
    //       {
    //      	int n = max (portA->channel->targetBound , portB->channel->targetBound);
    //      	portA->channel->qos->updateTargetResponseBound( n , modifiedChannels);
    //       }

    //     Port *aa;
    //     Port *bb;

    //     for (int i = 0; i <=1; i++) 
    //       {
    // 	if (i==0) { aa=portA; bb=portB;} else {aa=portB; bb=portA;}
    // 	// all the bounds to set on aa:
    // 	if (portI->channel->qos->hasInitiatorBound() and bb->channel->qos->hasTargetBound() )
    // 	  {
    // 	    int n = max (portA->channel->initiatorBound , bb->channel->targetBound);
    // 	    aa->channel->qos->updateInitiatorBound( n , modifiedChannels);
    // 	  }

    //       }

    return;
  }


  void buildPrimitiveLogic ( ) {

    unsigned int w = portI->channel->getDataWidth();
    ASSERT (w == portA->channel->getDataWidth());
    ASSERT (w == portB->channel->getDataWidth());
    
    Signal *i_irdy = new Signal(name+"i_irdy");
    Signal *i_data = new Signal(name+"i_data");
    Signal *i_trdy = new Signal(name+"i_trdy");
    
    i_irdy                -> setExpr( new Id_Expr(1 , portI->channel->irdy ) );
    i_data                -> setExpr( new Id_Expr(w , portI->channel->data ) );
    portI->channel->trdy  -> setExpr( new Id_Expr(1 , i_trdy           ) );
     
    Signal *a_irdy = new Signal(name+"a_irdy");
    Signal *a_data = new Signal(name+"a_data");
    Signal *a_trdy = new Signal(name+"a_trdy");

    portA->channel->data   -> setExpr( new Id_Expr(w , a_data           ) );
    portA->channel->irdy   -> setExpr( new Id_Expr(1 , a_irdy           ) );
    a_trdy                 -> setExpr( new Id_Expr(1 , portA->channel->trdy ) );

    Signal *b_irdy = new Signal(name+"b_irdy");
    Signal *b_data = new Signal(name+"b_data");
    Signal *b_trdy = new Signal(name+"b_trdy");

    portB->channel->data   -> setExpr( new Id_Expr(w , b_data           ) );
    portB->channel->irdy   -> setExpr( new Id_Expr(1 , b_irdy           ) );
    b_trdy                 -> setExpr( new Id_Expr(1 , portB->channel->trdy ) );

    Expr *routeToA = new Eq_Expr( i_data , new Bvconst_Expr(3,w) );
    Expr *routeToB = new Not_Expr( routeToA);

    a_irdy        -> setExpr ( new And_Expr( routeToA , i_irdy) );
    b_irdy        -> setExpr ( new And_Expr( routeToB , i_irdy) );

    a_data        -> setExpr ( (new Case_Expr(w))
			       -> setDefault( new Bvconst_Expr(0,w))
			       -> addCase(routeToA, i_data));
      
    b_data        -> setExpr ( (new Case_Expr(w))
			       -> setDefault( new Bvconst_Expr(0,w))
			       -> addCase( routeToB, i_data));

    i_trdy        -> setExpr ( new Or_Expr( new And_Expr(routeToB, b_trdy),
					    new And_Expr(routeToA, a_trdy)  )
			       );    
    return;
  }

};












class Credit_Counter : public Composite {
public:
  unsigned int depth;

  Credit_Counter(Channel *in, Channel *out, unsigned int de, string n, Hier_Object *p) :  Composite(n,p) {

    depth = de;
    Channel *a = new Channel("a",10,this);
    Channel *b = new Channel("b",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
	 
    Source *token_src = new Source(a,"token_src",this);
    (token_src)->setTypeEager();
    Sink *token_sink = new Sink(b,"token_sink",this);
    (token_sink)->setTypeEager();
	 
    new Fork(a,out,c,"f1",this);
    //    credit_queue = 
    Queue *oc = new Queue(c,d,depth,"outstanding_credits",this);
    oc->setType(PACKET_CREDIT);
    new Join(d,in,b,"j1",this);        

    //    new Queue(in,out,depth,"outstanding_credits",this);

  }

};

class Credit_Loop : public Composite {
public:
  Credit_Loop(string n, Hier_Object *p) : Composite(n,p) {
 
    Channel *a = new Channel("a",10,this);
    Channel *b = new Channel("b",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
    Channel *e = new Channel("e",10,this);
    Channel *f = new Channel("f",10,this);
    Channel *g = new Channel("g",10,this);
                
    Source *pkt_src = new Source(a,"pkt_src",this);
    Queue *tokens = new Queue(f,g,2,"available_tokens",this);
    tokens -> setType(PACKET_CREDIT);

    new Join(a,g,b,"j",this);

    Queue *packets = new Queue(b,c,2,"packets",this);
    new Fork(c,d,e,"f",this);
    Sink *pkt_sink = new Sink(d,"pkt_sink",this);
    //    (pkt_sink)->setTypeEager();
    (pkt_sink)->setTypeBounded(10);
    Credit_Counter *cc = new Credit_Counter(e, f, 2, "cc", this);

  }

};


class Ex_Tree : public Composite {
public:
  Ex_Tree(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *b = new Channel("b",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
    Channel *e = new Channel("e",10,this);
    Channel *f = new Channel("f",10,this);
    Channel *g = new Channel("g",10,this);
    Channel *h = new Channel("h",10,this);
    Channel *i = new Channel("i",10,this);
    Channel *j = new Channel("j",10,this);
    Channel *k = new Channel("k",10,this);
    Channel *l = new Channel("l",10,this);
                
    Source *src_a = new Source(a,"src_a",this);
    Source *src_c = new Source(c,"src_c",this);
    Source *src_f = new Source(f,"src_f",this);
    Source *src_i = new Source(i,"src_i",this);

    new Merge(b,c,d,"m1",this);
    new Merge(e,f,g,"m2",this);
    new Merge(h,j,k,"m3",this);
 
    new Queue(a,b,2,"q1",this);
    new Queue(d,e,2,"q2",this);
    new Queue(g,h,2,"q3",this);
    new Queue(i,j,2,"q4",this);
    new Queue(k,l,2,"q5",this);

    //Sink *pkt_sink = new Sink(l,"pkt_sink",this);
    Sink *sink_l = new Sink(l,"sink_l",this);
    (sink_l)->setTypeBoundedResponse(0);
    (src_a)->setTypeNondeterministic();
    (src_c)->setTypeNondeterministic();
    (src_f)->setTypeNondeterministic();
    (src_i)->setTypeNondeterministic();

        
  }    
};




class Ex_Join : public Composite {
public:
  Ex_Join(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
                
    Source *src_a = new Source(a,"src_a",this);
    Source *src_c = new Source(c,"src_c",this);
    new Join(a,c,d,"m1",this);
    Sink *sink_d = new Sink(d,"sink_l",this);

    //(sink_d)->setTypeEager();
    //    (sink_d)->setTypeBoundedResponse(3);
    (sink_d)->setTypeBoundedResponse(3);
    //    (src_a)->setTypeNondeterministic();
    (src_a)->setTypeEager();
    (src_c)->setTypeEager();
    //     (src_c)->setTypeNondeterministic();
  }    
};



class Ex_Fork : public Composite {
public:
  Ex_Fork(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
                
    Source *src_a = new Source(a,"src_a",this);
    Sink *sink_c = new Sink(c,"sink_c",this);
    Sink *sink_d = new Sink(d,"sink_d",this);
    new Fork(a,c,d,"m1",this);

    (sink_c)->setTypeBoundedResponse(3);
    //    (sink_d)->setTypeBoundedResponse(3);
    (sink_d)->setTypeBounded(3);
    (src_a)->setTypeEager();
    //    (src_a)->setTypeNondeterministic();
  }    
};



class Ex_Tree0 : public Composite {
public:
  Ex_Tree0(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
                
    Source *src_a = new Source(a,"src_a",this);
    Source *src_c = new Source(c,"src_c",this);
    new Merge(a,c,d,"m1",this);
    Sink *sink_d = new Sink(d,"sink_l",this);

    //(sink_d)->setTypeEager();
    (sink_d)->setTypeBoundedResponse(3);
    (src_a)->setTypeNondeterministic();
    (src_c)->setTypeNondeterministic();
  }    
};


class Ex_Tree1 : public Composite {
public:
  Ex_Tree1(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *b = new Channel("b",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
                
    Source *src_a = new Source(a,"src_a",this);
    Source *src_c = new Source(c,"src_c",this);

    new Queue(a,b,2,"q1",this);
    new Merge(b,c,d,"m1",this);

    Sink *sink_d = new Sink(d,"sink_l",this);

    //(sink_d)->setTypeEager();
    (sink_d)->setTypeBoundedResponse(3);
    (src_a)->setTypeNondeterministic();
    (src_c)->setTypeNondeterministic();
        
  }
};



class Ex_Queue : public Composite {
public:
  Ex_Queue(string n, Hier_Object *p) : Composite(n,p) {
    int queue_size = 6;
    int lb_sink = 2; 
    
    //    Channel *a = new Channel("a",10,this);
    Channel *a = new Channel("a",4,this);
    Channel *b = new Channel("b",4,this);
    a->setPacketType(PACKET_DATA);
    b->setPacketType(PACKET_DATA);

    //a->widenData(2);

    Source *src_a = new Source(a,"src_a",this);
    new Queue(a,b,queue_size,"q1",this);
    
    Sink *sink_b = new Sink(b,"sink_b",this);
    (sink_b)->setTypeBoundedResponse(lb_sink);
    //(sink_b)->setTypeBounded(lb_sink);
    (src_a)->setTypeNondeterministic();
  }
    
};





class Ex_Queue_Chain : public Composite {
public:
  Ex_Queue_Chain(string n, Hier_Object *p) : Composite(n,p) {

    Channel *a = new Channel("a",10,this);
    Channel *b = new Channel("b",10,this);
    Channel *c = new Channel("c",10,this);
    Channel *d = new Channel("d",10,this);
                
    Source *src_a = new Source(a,"src_a",this);

    new Queue(a,b,2,"q1",this);
    new Queue(b,c,2,"q2",this);
    new Queue(c,d,2,"q3",this);

    Sink *sink_x = new Sink(d,"sink_x",this);
    (sink_x)->setTypeBoundedResponse(1);
    (src_a)->setTypeNondeterministic();
        
  }
};



int main (int argc, char **argv)
{

  cout << "\n\n\n======================================================\n";
  logic::c = new Ckt();

  //string network = "ex_tree";
  string network = "ex_queue";
  string fnameOut = "dump.v";


  std::cout << argv[0];
  for (int i = 1; i < argc; i++) 
    { 
      string s = string(argv[i]);
      cout << "\nparsing arg: " << argv[i] << "\n";

      if        ( s == "--dump")    { fnameOut = argv[++i];
      } else if ( s == "--network") { network  = argv[++i];
      } else if ( s == "--t_max")                          { logic::c->voptions->setTMax( atoi(argv[++i]) );
      } else if ( s ==  "--enable_persistance")            { logic::c->voptions->enablePersistance();
      } else if ( s == "--disable_persistance")            { logic::c->voptions->disablePersistance();
      } else if ( s ==  "--enable_lemmas")                 { logic::c->voptions->enablePhiLQueue();
      } else if ( s == "--disable_lemmas")                 { logic::c->voptions->disablePhiLQueue();
      } else if ( s ==  "--enable_psi")                    { logic::c->voptions->enablePsi() ;
      } else if ( s == "--disable_psi")                    { logic::c->voptions->disablePsi(); 
      } else if ( s == "--enable_bound_channel")           { logic::c->voptions->enableBoundChannel();
      } else if ( s == "--disable_bound_channel")          { logic::c->voptions->disableBoundChannel();
      } else if ( s == "--enable_response_bound_channel")  { logic::c->voptions->enableResponseBoundChannel();
      } else if ( s == "--disable_response_bound_channel") { logic::c->voptions->disableResponseBoundChannel();
      } else {
	ASSERT2(0,"cmd line argument "+s+" is not understood\n");
      }
    }

  logic::c -> voptions->printSettings();

  cout << "\n\nnetwork = " << network << "\n";
  network::n = new Network();


  // create one of the networks depending on cmd line args. Top level
  // network is within this null root because every object must be
  // contained within one
  Composite *hier_root = new Composite();
  if      (network == "credit_loop")        {  new Credit_Loop(     "top",hier_root ); } 
  else if (network == "ex_tree")            {  new Ex_Tree(         "top",hier_root ); } 
  else if (network == "ex_tree0")           {  new Ex_Tree0(        "top",hier_root ); } 
  else if (network == "ex_tree1")           {  new Ex_Tree1(        "top",hier_root ); } 
  else if (network == "ex_queue")           {  new Ex_Queue(        "top",hier_root ); } 
  else if (network == "ex_queue_chain")     {  new Ex_Queue_Chain(  "top",hier_root ); } 
  else if (network == "ex_join")            {  new Ex_Join(         "top",hier_root ); } 
  else if (network == "ex_fork")            {  new Ex_Fork(         "top",hier_root ); } 
  else {  ASSERT(0);  }


  network::n->printNetwork();
  network::n->addLatencyLemmas();    // propagate assertions

  logic::c -> voptions->printSettings();
  logic::c -> buildNetworkLogic( network::n );
  //  delete network::n;

  logic::c -> dumpAsVerilog( fnameOut );     
  //logic::c -> dumpAsUclid( fnameOut );     

  cout << "ending main.cpp normally\n";

  return 0;
};

  
