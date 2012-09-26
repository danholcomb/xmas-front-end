
/* class Primitive; */

class Init_Port; //initiator of some channel
class Targ_Port; //target of some channel
class Port;
class Slot_Qos;


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
  Seq_Signal *numItems;
  unsigned int numItemsMax;
  vector <Slot_Qos*> slotQos;
  vector <Signal*> qslots;
  unsigned int depth;

  Queue(Channel *in, Channel *out, unsigned int d, const string n, Hier_Object *p);

  void setPacketType ( PacketType p) { type = p; }
  PacketType getPacketType () { return type;}
  
  void buildPrimitiveLogic ( );
  void propagateLatencyLemmas( );        
};


/* class Join; */
class Join : public Primitive {
 public:
  Targ_Port *a;
  Targ_Port *b;
  Init_Port *o;

 Join(Channel *in1, Channel *in2, Channel *out, string n, Hier_Object *p) : Primitive(n,p) {
    a = new Targ_Port("a",in1,this);
    b = new Targ_Port("b",in2,this);
    o = new Init_Port("o",out,this);
    (*g_network).primitives.push_back(this);
  }


  void propagateLatencyLemmas( );
  void buildPrimitiveLogic ( );

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
    (*g_network).primitives.push_back(this);
  }

  void propagateLatencyLemmas( );
  void buildPrimitiveLogic ( );
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

    (*g_network).primitives.push_back(this);
  }

  void propagateLatencyLemmas( );
  void buildPrimitiveLogic ( );
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

    (*g_network).primitives.push_back(this);
  }

  void propagateLatencyLemmas( );
  void buildPrimitiveLogic ( );
};




class Sink : public Primitive {
 public:
  Targ_Port *i;
  OracleType sink_type;
  unsigned int bound;

 Sink(Channel *in, string n, Hier_Object *p) : Primitive(n,p) {
    i = new Targ_Port("i",in,this);
    sink_type = ORACLE_NONDETERMINISTIC;
    (*g_network).primitives.push_back(this);
    (*g_network).sinks.push_back(this);
  }

  void setTypeEager()      { sink_type = ORACLE_EAGER; }
  void setTypeDead()       { sink_type = ORACLE_DEAD;  } 
  void setTypeNondeterministic()         { sink_type = ORACLE_NONDETERMINISTIC;    }
  void setTypeBoundedResponse(unsigned int n)   { sink_type = ORACLE_BOUNDED_RESPONSE; bound = n; }
  void setTypeBounded(unsigned int n)   { sink_type = ORACLE_BOUNDED; bound = n; }

  void propagateLatencyLemmas( );
  void buildPrimitiveLogic ();
};





class Source : public Primitive {
 public:
  Init_Port *o;
  OracleType source_type;

 Source(Channel *out, string n, Hier_Object *p) : Primitive(n,p) {
    o = new Init_Port("o",out,this);
    source_type = ORACLE_NONDETERMINISTIC;
    (*g_network).primitives.push_back(this);
    (*g_network).sources.push_back(this);
  }

  void setTypeEager() { source_type = ORACLE_EAGER;}
  void setTypeDead() { source_type = ORACLE_DEAD;}
  void setTypeNondeterministic() { source_type = ORACLE_NONDETERMINISTIC;}

  void buildPrimitiveLogic ( );
  void propagateLatencyLemmas( );
};




class Slot_Qos {
  unsigned int timeToSink;
  unsigned int maxAge;
  Queue * parentQueue;
  unsigned int slotIndexInParentQueue;
  bool _isEnabled;
  string printHeader();

 public:
  Slot_Qos(unsigned int i, Queue *q);

  bool isEnabled() { return _isEnabled;}
  void enable()  { _isEnabled =  true; return;}
  void disable() { _isEnabled = false; return;}

  bool         hasTimeToSink()                 {return timeToSink != T_PROP_NULL;};
  unsigned int getTimeToSink()                 {return timeToSink;};
  bool         hasMaxAge()                     {return maxAge != T_PROP_NULL;};
  unsigned int getMaxAge()                     {return maxAge;}

  void printSlotQos(ostream &f);
  void setTimeToSink(unsigned int t);
  void setMaxAge(unsigned int t);
};






