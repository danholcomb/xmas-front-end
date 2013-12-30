#ifndef MAIN_H
#define MAIN_H


#include <cstdlib>
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


// for flattening hierarchy names
#define HIER_SEPARATOR "__"
 

//#define DO_THIS_AT_END_OF_ASSERTION ;
#define DO_THIS_AT_END_OF_ASSERTION exit(1);


#define ASSERT(x)					\
    if (! (x))						\
    {							\
	cout << "ERROR!! Assert " << #x << " failed\n";	\
	cout << " on line " << __LINE__  << "\n";	\
	cout << " in file " << __FILE__ << "\n\n";	\
	DO_THIS_AT_END_OF_ASSERTION	       		\
	    }

#define ASSERT2(x,s)					\
    if (! (x))						\
    {							\
	cout << "ERROR!! Assert " << #x << " failed\n";	\
	cout << " " << s << "\n";			\
	cout << " on line " << __LINE__  << "\n";	\
	cout << " in file " << __FILE__ << "\n\n";	\
	DO_THIS_AT_END_OF_ASSERTION	       		\
	    }



class Hier_Object;
class Channel;
class Network;
class Ckt;
class Primitive;
class Port;
class Signal;
class Seq_Signal;
class Oracle_Signal;
class Timestamp_Signal;
class Source;
class Sink;
class Queue;
class Delay_Eq;

using namespace std;

extern Ckt *g_ckt;         // a collection of expressions/signals/etc


enum OracleType {
    ORACLE_EAGER, 
    ORACLE_DEAD, 
    ORACLE_NONDETERMINISTIC, 
    ORACLE_BOUNDED_RESPONSE,
    ORACLE_PERIODIC
};

enum LatencyOperationType {
    LATENCY_OPERATION_PLUS, 
    LATENCY_OPERATION_MAX, 
    LATENCY_OPERATION_MULTIPLY
};


//for queues and channels, this indicates whether to consider
//timestamps or not
enum PacketType {
    PACKET_TOKEN,
    PACKET_DATA
};

//for queues and channels, this indicates whether to consider
//timestamps or not
enum PortType {
    PORT_INITIATOR,
    PORT_TARGET
};


enum FunctionType {
    FUNCTION_TAG, //put bits onto the lsb
    FUNCTION_UNTAG, // remove bits from the lsb
    FUNCTION_ASSIGN // ignore data and just set to some const
};

enum TimestampType {
    TIMESTAMP_BINARY_FIXED,
    TIMESTAMP_BINARY_INCREMENTING,
    TIMESTAMP_UNARY_INCREMENTING
};


string itos(int i);
unsigned numBitsRequired( unsigned maxval);





/*! \brief a port connects a channel to a port of a primitive*/
class Port  {
    string pname;
public:
    PortType dir;
    Primitive *owner; // primitive that contains the port
    Channel *channel; // channel that is driven by port

    Signal *irdy;
    Signal *data;
    Signal *trdy;
    
    // /* \param n an arbitrary local name for this port within prim (e.g. "a")
    //   \param c the channel connecting to the port (from outside the owner primitive)
    //   \param p the primitive that port is being instantiated within
    // */
    Port(string n, Channel *c, Primitive *p, PortType d);
    string getName();
};


class Expr;


/** Ckt is a collection of signals with logic formulas and maps easily
 * to RTL some of the signals are circuit logic and some are from
 * property checkers
 */
class Ckt {

    /// all the internal signals of the ckt
    vector<Signal*> signals;
    vector<Expr*> expressions;

    /// global clock used in timestamp encoding, ignored in stopwatch encoding
    Seq_Signal *tCurrent; 
    
    /// the PI to the ckt. All oracle signals bit-extracted from this.
    Oracle_Signal *oracleBus;

    unsigned tCurrentModulus; // when does counter rollover. used to infer timer width
    unsigned wClk;
    TimestampType timestampType;


public:

    vector             < Expr *> manualLemmasCond;
    vector          < unsigned > manualLemmasBound;
    vector <           Signal *> manualLemmasData;
    vector < Timestamp_Signal *> manualLemmasTimestamp;


    
    Ckt() {
	tCurrentModulus = 500;
	timestampType = TIMESTAMP_BINARY_INCREMENTING;
    };


    void buildNetworkLogic(Network *n);
    void buildQosLogic(Network *n);
    void printAssertions( string fname );
    string sprintAssertions();
    void printSignals( string fname );
    void printExpressions( string fname );
    void dumpAsVerilog (string fname);

    void setAssertionStates( vector <pair < string, bool > > & tagStates );
    void setAssertionStates( string tag, bool state);

    void setTRange( unsigned t) { tCurrentModulus = t;  return; }
    void setTimestampType( string s) { 
	if        (s == "binary_fixed") {	   timestampType = TIMESTAMP_BINARY_FIXED; 
	} else if (s == "binary_incrementing") {   timestampType = TIMESTAMP_BINARY_INCREMENTING; 
	} else if (s == "unary_incrementing") {	   timestampType = TIMESTAMP_UNARY_INCREMENTING; 
	} else { ASSERT(0);}
	return;
    } 


    // these are friends to modify the ckt when building
    friend class Primitive;
    friend class Queue;
    friend class Source;
    friend class Sink;
    friend class Join;
    friend class Fork;
    friend class Merge;
    friend class Switch;

    friend class Demux;
    friend class Pri_Merge;
    friend class Ring_Stop;

    friend class Network;

    friend class Expr;
    friend class Signal;
    friend class Timestamp_Signal;
    friend class Main;
    friend class Delay_Eq;
    friend class Channel;

    friend Expr * updatedTimestamp(Expr *e);
    friend Expr * initialTimestamp();
    friend int mpPlusInt(int a, int b);
    friend Delay_Eq * crossProduct(Delay_Eq *x, Delay_Eq *y, LatencyOperationType op);
/*   friend class ; */
};






/*!A Hier Object is anything in the hierarchy that contains 1 or more
  primitives or composite (e.g. credit loop, queue, etc) objects in its subtree.*/
class Hier_Object {
public:
    string name; 
    
    /// to construct root with no name, and keep flattened names shorter
    Hier_Object( );  
    Hier_Object( const string n, Network *network);
    Hier_Object( const string n, Hier_Object *parent, Network *network);
};




#endif

