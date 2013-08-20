
class Port;
class Age_Bounds_One_Source;



enum ConstraintType {
    CONSTRAINT_EQ, 
    CONSTRAINT_NEQ
};

class Queue_Constraint {
public:
    Queue* queue; 
    ConstraintType constraint;
    unsigned num;
    Queue_Constraint(Queue *q, unsigned n, ConstraintType c) {
        queue = q;
        num = n;
        constraint = c;
    } 
    Queue_Constraint * invertQueueConstraint() {
	ConstraintType c2 = (constraint == CONSTRAINT_EQ) ? CONSTRAINT_NEQ: CONSTRAINT_EQ;
	return new Queue_Constraint(queue, num, c2);
    }
};



/*! \brief xmas kernel primitive primitives don't have internal
  channels
*/
class Primitive : public Hier_Object {
public:
    vector <Port*> ports;
Primitive(string n, Network *network) : Hier_Object(n,network) { }
Primitive(string n, Hier_Object *p, Network *network) : Hier_Object(n,p,network) { }
  
    virtual void buildPrimitiveLogic ( ) =0;

    Delay_Eq *  propagateDelayWrapper( Port *p, set <Port*> & visitedPorts );
    virtual Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts )=0;

    void propagateAgeWrapper(  Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound);
    virtual void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound) = 0;
};


struct Delay_Eq_Case {
    vector <Queue_Constraint*> condition; //condition is AND of all expr
    unsigned int value;
    Delay_Eq_Case(int v) {
        value = v;
    } 
    Delay_Eq_Case(Queue_Constraint *qc, int v) {
        condition.push_back(qc);
        value = v;
    } 
    Delay_Eq_Case(vector <Queue_Constraint*> &qc, int v) {
        condition = qc;
        value = v;
    } 
};

class Delay_Eq {

public:
    vector < Delay_Eq_Case* > cases;
    Signal *currentBound;
    Signal *maxFiniteBound;
    Signal *condFiniteBound;
    Signal *condInfiniteBound;
    unsigned intMaxFiniteBound;

    Delay_Eq(int i) {     cases.push_back(new Delay_Eq_Case(i)); } //terminal eq (leaf of tree)
    Delay_Eq(vector <Delay_Eq_Case*> &c) { cases = c; }

    void buildSignals(string s);
    string sprint();
};




class Queue : public Primitive {
    PacketType type;
  
    /// the age of each slot, or 0 if the slot is empty;
    vector <Signal*> qslotIsOccupied;

    Signal *isEmpty; 
    Signal *isFull;
    Port *portI;
    Port *portO;

public:
    vector <Signal*> qslots; //so that ring can use it for its lemmas
    vector <Timestamp_Signal *> qslotTimestamps;

    Seq_Signal *numItems;
    unsigned int numItemsMax;
    unsigned int depth;

    Queue(Channel *in, Channel *out, unsigned int d, const string n, Hier_Object *p, Network *network);

    Queue * setPacketType ( PacketType p) { type = p; return this; }
    PacketType getPacketType () { return type;}
  
    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );

    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );

    // check some age against all packet
    // scalar signal checked against all slots
    // tag is for the assertionClassTag
    void buildSlotAgeAssertions(                             unsigned b, string tag );
    void buildSlotAgeAssertions( unsigned int idx,           unsigned b, string tag );
    void buildSlotAgeAssertions( unsigned int idx, Expr* en, unsigned b, string tag );
};


class Join : public Primitive {
public:
    Port *portA;
    Port *portB;
    Port *portO;
    Join(Channel *in1, Channel *in2, Channel *out, string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};


class Fork : public Primitive {
public:
    Port *portI;
    Port *portA;
    Port *portB;
    Fork(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};




class Switch : public Primitive {
public:
    Port *portI;
    Port *portA;
    Port *portB;
    
    //route to A if expr matches these bits of input data
    pair <unsigned int, unsigned int> routeBits; 
    Expr * routeExpr;

    Switch(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );

    Switch * setRoutingFunction( unsigned int msb, unsigned int lsb, Expr *e);
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};

class Demux : public Primitive {
public:
    Port *portI;
    Port *portA;
    Port *portB;
    Signal *demux_ctrl;

    Demux( Signal *s, Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );

    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};



class Function : public Primitive {
public:
    Port *portI;
    Port *portO;
    FunctionType functionType;
    unsigned int tagWidth;
    unsigned int tagValue;
    Function(Channel *in, Channel *out, string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );
    Function * setFunction( FunctionType f, unsigned int n, unsigned int v);
    Function * setFunction( FunctionType f, unsigned int n);
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};




class Merge : public Primitive {
public:
    Port *portA;
    Port *portB;
    Port *portO;
    Signal *u;
    
    Merge(Channel *in1, Channel *in2, Channel *out, const string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};

class Pri_Merge : public Primitive {
public:
    Port *portA;
    Port *portB;
    Port *portO;
    Signal *u;
    
    Pri_Merge(Channel *in1, Channel *in2, Channel *out, const string n, Hier_Object *p, Network *network);
    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );
};






class Sink : public Primitive {
    // these are assigned in build
    unsigned int bound;
    OracleType sink_type;
    Port *portI;

public:
    Sink(Channel *in, string n, Hier_Object *p, Network *network);

    Sink * setOracleType( OracleType x );
    Sink * setOracleType( OracleType x, unsigned int n );

    void buildPrimitiveLogic ();
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );

    friend class Network; // hack: to access the port to start propagation
};




class Source : public Primitive {
    unsigned int bound;
    OracleType source_type;
    Port *portO;
    vector <Expr*> datas;

public:

    // data is not-deterministic according to width of channel
    Source(Channel *out, string n, Hier_Object *p, Network *network );

    // data is non-deterministically chosen for list 
    Source(Channel *out, string n, Hier_Object *p, Network *network, vector <Expr*> &dataChoices); 

    Source * setOracleType( OracleType x );
    Source * setOracleType( OracleType x, unsigned int n );

    void buildPrimitiveLogic ( );
    Delay_Eq * propagateDelay( Port *p, set <Port*> & visitedPorts );
    void propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound );

    friend class Network; // hack: to access the port to start propagation
};









/* // a counter that asserts an output if t cycles elapse between a and b. */
/* // used for enforcing bounds on sources and sinks, and for checking */
/* // channel bounds. */
// same as normal channel monitor except time bound is specified as Expr
// the expr is still intended to be const
class Channel_Monitor {
    void init(       Expr * trig, Expr * targ,        Expr *t, string name, bool assertInvariant = true);
public:
    Signal *tFutureTarg;
    Signal *tFutureTargValid; //should just be targ!=infty
    Channel_Monitor( Expr * trig, Expr * targ, unsigned int t, string name, bool assertInvariant = true);
    Channel_Monitor( Expr * trig, Expr * targ,        Expr *t, string name, bool assertInvariant = true);
};

