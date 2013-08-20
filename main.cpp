 

#include "main.h"
#include "expressions.h"
#include "primitives.h"
#include "channel.h"
#include "network.h"
using namespace std;

Ckt *g_ckt;

    
string itos(int i) {
    std::stringstream out;
    out << i;
    string s = out.str();
    return s;
}

int stoi(string s) { return atoi(s.c_str()); } 

unsigned numBitsRequired( unsigned maxval) {
    unsigned x = log2(maxval);
    unsigned w = x + 1;
    return w;
}




// top level object has no name because it is a "network" and has the
// real top level object as a child. this is done so that the real top
// level child is just like any other hierarchical object, and any
// hier object can be used as the top level object
Hier_Object::Hier_Object( ) {};
Hier_Object::Hier_Object( const string n, Hier_Object *parent, Network *network) {
    // for readable signals, dont add tb to hierarchy name
    name = (n=="tb") ? "" : name = parent->name + n + HIER_SEPARATOR ;
};





string Port::getName() { return this->owner->name + pname; }

Port::Port(string n, Channel *c, Primitive *p, PortType d)
{
	pname = n;
	owner = p;
	channel = c;
    dir = d;
	if (dir == PORT_INITIATOR) {
	    ASSERT2(c->initiator == 0, "channel has 2nd initiator");
	    c->initiator = this;
	} else if (dir == PORT_TARGET) {
	    ASSERT2(c->target == 0, "channel has 2nd target");
	    c->target = this;
	} else {
	    ASSERT(0);
	}
    p->ports.push_back(this);
	
};




void Ckt::buildNetworkLogic(Network *n)
{
    cout << "starting buildNetworkLogic()\n";
    
    if        (timestampType == TIMESTAMP_BINARY_FIXED) {
        wClk = numBitsRequired( tCurrentModulus );
    } else if (timestampType == TIMESTAMP_BINARY_INCREMENTING) {
        wClk = numBitsRequired( tCurrentModulus );
    } else if (timestampType == TIMESTAMP_UNARY_INCREMENTING) {
        wClk = tCurrentModulus ;
    } else { ASSERT(0); }
    cout << "time rollover at                 " << tCurrentModulus << "\n";
    cout << "clock will use                   " << wClk << " bits\n\n";
    
    unsigned int _w = 0; 
    this->oracleBus = new Oracle_Signal("oracles",_w);

    Signal *tCurrentNxt = (new Signal("tCurrentNxt",wClk));

    this->tCurrent = (new Seq_Signal("tCurrent",wClk))
        -> setResetExpr( new Bvconst_Expr(0,wClk ) )
        -> setNxtExpr( tCurrentNxt );

    tCurrentNxt-> setExpr( BvIncrExprModM (tCurrent, tCurrentModulus) );
    
    Signal *clkRangeValid = (new Signal("clkRangeValid", new Lt_Expr(tCurrent, new Bvconst_Expr(tCurrentModulus,wClk))))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("range")
        -> addAssertionClassTag("psi");
    
    
    // if channel carries packet, widen it for the timestamp
    for (vector <Channel*>::iterator it = n->channels.begin(); it != n->channels.end(); it++ ) {
        if ((*it)->getPacketType() == PACKET_DATA) {
            (*it)->widenForTimestamp(wClk);
        }
    }
    
    // buildChannelLogic should have an input for timestampWidth
    for (vector <Channel*>::iterator it = n->channels.begin(); it != n->channels.end(); it++ ) {
        (*it)->buildChannelLogic();
    }

    for (vector <Primitive*>::iterator it = n->primitives.begin(); it != n->primitives.end(); it++ ) {
        cout << "about to build primitive " << (*it)->name << "\n";
        (*it)->buildPrimitiveLogic();
    }
    
    cout << "finishing buildNetworkLogic()\n";
    return;
};








void Ckt::printAssertions( string fname )
{
    cout << "\nprintAssertions...\n";
    ofstream  f;
    f.open(fname.c_str());
    f << sprintAssertions() << "\n";
    f.close();
    return;
};

string Ckt::sprintAssertions( )
{
    cout << "\nsprintAssertions...\n";
    
    stringstream out;
    for (vector <Signal*>::iterator it = this->signals.begin(); it != this->signals.end(); it++ ) {
        if ((*it)->getSignalIsAssertion()) {
            if (!(*it)->getAssertionIsEnabled())
                out << " "; // easier to see in output
            out << (*it)->getAssertionIsEnabled() << "  " << setw(50) << left << (*it)->name;
            for (set <string>::iterator n = (*it)->assertionClassTags.begin(); n != (*it)->assertionClassTags.end(); n++ ) {
                if ((*it)->name != (*n))
                    out << "   " << *n << " ";
            }
            out << "\n";
        }
    }
    return out.str();
};


void Ckt::printSignals( string fname )
{
    cout << "\nprintSignals...\n";
    
    ofstream  f;
    f.open(fname.c_str());
    f << "signals.size() = " << this->signals.size() << "\n";
    for (vector <Signal*>::iterator it = this->signals.begin(); it != this->signals.end(); it++ ) {
        f << "signal " << setw(3) << right << distance(this->signals.begin(),it) << ":  " 
          << setw(50) << left << (*it)->name 
          << " width = " << (*it)->getWidth() << "\n";
    }
    f.close();
    return;
};


void Ckt::printExpressions( string fname )
{
    cout << "\nprintExpressions...\n";
    
    ofstream  f;
    f.open(fname.c_str());
    f << "expressions.size() = " << this->expressions.size() << "\n";

    for (vector <Expr*>::iterator it = this->expressions.begin(); it != this->expressions.end(); it++ ) {
        f << "expression " << setw(3) << right << distance(this->expressions.begin(),it) << ":  " 
          << " width = " << (*it)->getWidth() << "\n";
        if ((*it)->getWidth()==0) {
            cout << "expression " << setw(3) << right << distance(this->expressions.begin(),it) << ":  " ;
            cout << " width = " << (*it)->getWidth() << "\n";
            ASSERT(0);
        }
    }

    f.close();
    return;
};




/*! \brief turn on/off all assertions matching tag */
void Ckt::setAssertionStates( vector <pair < string, bool > > & tagStates )
{
    for (vector < pair<string,bool> >::iterator c = tagStates.begin(); c != tagStates.end();  c++ )
        this-> setAssertionStates( c->first, c->second );     
    return;
};

/*! \brief turn on/off all assertions matching tag */
void Ckt::setAssertionStates( string tag, bool state )
{
    unsigned n = 0;
    for (vector <Signal*>::iterator it = this->signals.begin(); it != this->signals.end(); it++ ) {
        if ( (*it)->getSignalIsAssertion() and (*it)->assertionClassTags.count(tag) ) {
            (*it)->setAssertionIsEnabled(state);
            n++;
            continue;
        }
    }

    cout << "set " << setw(3) << n << " assertions with tag " << setw(20) << tag << " to state " << state << "\n";
    ASSERT2(n>0,"no matches for tag "+tag+"\n");
    return;
};




















/*! \param fname the name of the file where the verilog will be
 *  printed*/
void Ckt::dumpAsVerilog (string fname)
{
  
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



class Credit_Counter : public Hier_Object
{
public:
    unsigned depth;
    Queue *oc; // is public for use in numeric invariants outside of object

    Credit_Counter(Channel *in, Channel *out, unsigned int de, string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p, network)
        {

            depth = de;
            Channel *a = (new Channel("a",this, network));
            Channel *b = (new Channel("b",this, network));
            Channel *c = (new Channel("c",this, network));
            Channel *d = (new Channel("d",this, network));

            Source *token_src = (new Source( a, "token_src",this, network)) ->setOracleType(ORACLE_EAGER);;
            Sink *token_sink  = (new Sink(   b,"token_sink",this, network)) ->setOracleType(ORACLE_EAGER);
	 
            new Fork(a,out,c,"f1",this, network);
            oc = new Queue(c,d,depth,"credits",this, network);
            oc->setPacketType(PACKET_TOKEN);
            new Join(d,in,b,"j1",this, network);        
        }
};



// a chain of queues connected in sequence
class Queue_Chain : public Hier_Object {
public:
    Signal *numItems;
    Queue_Chain(Channel *in, Channel *out, unsigned queueDepth, unsigned numQueues,
                string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p, network) {
        
        PacketType type = in -> getPacketType();
        ASSERT(type == out->getPacketType());
        unsigned w = in->getDWidth();
        ASSERT( w == out->getDWidth());

        vector < Channel *> ch (numQueues+1);
        ch[0] = in;
        for (unsigned i=1; i< numQueues; i++) 
            ch[i] = (new Channel("ch"+itos(i), w, this, network));
        ch[numQueues] = out;

        vector <Queue*> queues (numQueues);
        vector <Expr*>    nums (numQueues);
        for (unsigned i=0; i< numQueues; i++) {
            queues[i] = (new Queue(ch[i], ch[i+1] , queueDepth, "q"+itos(i) , this, network)) -> setPacketType(type);
            nums[i] = queues[i]->numItems;
        }
        numItems = new Signal(name+"numItems", BvaddExpr(nums) );

    }
};


// one queue chain enforces cap, other enforces delay
class Delay_And_Capacity : public Hier_Object
{
public:
    Delay_And_Capacity(Channel *in, Channel *out, unsigned capacity, unsigned delay,
                       string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p, network)
        {

            ASSERT(in ->getPacketType() == out->getPacketType() );
            unsigned w = in->getDWidth();
            ASSERT(w == out->getDWidth());

            Channel *a0  = (new Channel("a0",w,this, network));
            Channel *an  = (new Channel("an",w,this, network));
            Channel *b0  = (new Channel("b0",this, network));
            Channel *bn  = (new Channel("bn",this, network));

            new Fork(in, b0, a0, "xf", this, network);        
            Queue_Chain *qc1 = new Queue_Chain(a0,an,1,delay,   "dpath",this,network);
            Queue *qc2 = (new Queue(b0,bn,capacity,"cpath",this,network)) -> setPacketType( in->getPacketType() );
            new Join(bn, an, out,"xj", this, network);        

            Signal *numInv = (new Signal(name+"numInv", new Eq_Expr(qc1->numItems, qc2->numItems )))
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi")
                -> addAssertionClassTag("numinv");

        }
};


class Token_Bucket_Regulator : public Hier_Object
{
public:

    Token_Bucket_Regulator(Channel *in, Channel *out, unsigned sigma, unsigned rho,
                           string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p, network)
        {

            Channel *a = (new Channel("a",this, network));
            Channel *b = (new Channel("b",this, network));
        
            (new Source(a,"token_src",this, network)) ->setOracleType(ORACLE_EAGER);;
            new Delay_And_Capacity(a,b,sigma,rho,"dc",this, network); 
            new Join(b,in,out,"j1",this, network);        
        }
};





class Ring_Stop : public Hier_Object
{
    
public:
    void addManualLemmas();
    Queue *packets;
    Queue *ingress;
    unsigned addr, numAddrBits, numStops, sinkBound;
    Signal *rsv; // current state of rsv logic

    Ring_Stop(Channel *in, Channel *out,
              unsigned _addr, unsigned _numStops,
              unsigned queueDepth, unsigned _sinkBound,
              string n, Hier_Object *p, Network *network )
        : Hier_Object(n,p,network)
        {

            addr = _addr;
            numStops = _numStops;
            numAddrBits = numBitsRequired( numStops);
            sinkBound = _sinkBound;
        
            unsigned int w = in->getDWidth();
            ASSERT(w == out->getDWidth());
            Channel *a = (new Channel("ca", w, this, network));
            Channel *b = (new Channel("cb", w, this, network));
            Channel *c = (new Channel("cc", w, this, network));
            Channel *d = (new Channel("cd", w, this, network));
            Channel *e = (new Channel("ce", w, this, network));
            Channel *f = (new Channel("cf", w, this, network));
            Channel *g = (new Channel("cg", w, this, network));
            Channel *h = (new Channel("ch", w, this, network));

            string sname = "stage"+itos(addr)+"__";
            Signal *admit = new Signal(sname+"admit", 1);

            Switch *sw = new Switch(in,a,b,"sw",this,network);
            (sw) -> setRoutingFunction( numAddrBits-1,0, new Bvconst_Expr(addr,numAddrBits) ); // check the dest

            Expr *ee = new Extract_Expr(in->data, numAddrBits-1, 0 );
            Signal *addrInRange = new Signal(name+"addrInRange", new Lt_Expr(ee, new Bvconst_Expr(numStops, numAddrBits)));
            addrInRange
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi");
        
            Signal *demux_ctrl = new Signal(sname+"demux_ctrl", new Id_Expr(admit)); 
            new Demux(demux_ctrl,a,c,d,"pswitch",this,network);

            new Merge(b,d,f,"merge",this,network);
            new Pri_Merge(f,g,h,"pmerge",this,network);
        
            packets = (new Queue(h, out, 2,          "packets",this, network)) -> setPacketType(PACKET_DATA);
            ingress = (new Queue(c, e,   queueDepth, "ingress",this, network)) -> setPacketType(PACKET_DATA);
        
            // the set of values that can be injected by the source
            vector <Expr *> validData (numStops); 
            for (unsigned i = 0; i < numStops; i++)
                validData[i] = new Bvconst_Expr(i,g->getDWidth());
        
            Source *src = (new Source(g,"pkt_src",this, network, validData)) -> setOracleType(ORACLE_NONDETERMINISTIC);
                
            Sink *pkt_sink = (new Sink(e,"pkt_sink",this, network))
                -> setOracleType(ORACLE_BOUNDED_RESPONSE,sinkBound);
        
            Signal *rsvIsAvail   = new Signal(sname +"rsvIsAvail", 1);
            Signal *packetHasRsv = new Signal(sname+"packetHasRsv", 1);
        
            // outstanding if someone other than packet has the rsv
            Expr *numOutstandingRsv = new And_Expr(new Not_Expr(rsvIsAvail), new Not_Expr(packetHasRsv));

            Expr *depthExpr = new Bvconst_Expr(ingress->depth, ingress->numItems->getWidth());
            Signal *numEmptySlots = new Signal(sname+"numEmptySlots", new Bvsub_Expr(depthExpr, ingress->numItems ));
            admit -> setExpr(new Lt_Expr(numOutstandingRsv, numEmptySlots) ); 

            // using these to test that everything works as expected
            // Signal *foo = (new Signal("foo", new Implies_Expr(packetHasRsv, a->irdy)))
            //  -> setSignalIsAssertion(true)
            //  -> addAssertionClassTag("psi");

            //  a -> setNonBlocking(true);
            //  d -> setNonBlocking(true);
            //  b -> setNonBlocking(true);
            //  f -> setNonBlocking(true);
            //  h -> setNonBlocking(true);
        
            Signal *rsvNxt = new Signal("rsvNxt",numAddrBits);
        
            rsv = (new Seq_Signal(sname+"rsv",numAddrBits))
                -> setResetExpr( new Bvconst_Expr(numStops,numAddrBits ) )
                -> setNxtExpr( rsvNxt );
        
            Signal *makeRsv  = new Signal(sname+"makeRsv", (new And_Expr())
                                          -> addInput(rsvIsAvail)
                                          -> addInput(BvIsZero(numEmptySlots) )
                                          -> addInput(a->irdy)
                );

            Signal *returnRsv       = new Signal(sname+"returnRsv",       new And_Expr( packetHasRsv, admit) );
            Signal *rsvRemainsAvail = new Signal(sname+"rsvRemainsAvail", new And_Expr( rsvIsAvail, new Not_Expr(makeRsv)));
            rsvNxt -> setExpr(  (new Case_Expr(numAddrBits))
                                -> setDefault(BvIncrExprModM(rsv,numStops) )
                                -> addCase( makeRsv,          new Bvconst_Expr(0,         numAddrBits))
                                -> addCase( returnRsv,        new Bvconst_Expr(numStops,  numAddrBits))
                                -> addCase( rsvRemainsAvail , new Bvconst_Expr(numStops,  numAddrBits))
                );


            packetHasRsv -> setExpr(new Eq_Expr( rsv, new Bvconst_Expr(numStops-1,numAddrBits)));
            rsvIsAvail   -> setExpr(new Eq_Expr( rsv, new Bvconst_Expr(numStops,  numAddrBits)));

            Expr *constOne = new Bvconst_Expr(1,packets->numItems->getWidth());
            Signal *numInv = (new Signal("numInv", new Lte_Expr(packets->numItems, constOne)))
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi")
                -> addAssertionClassTag("numinv");        

        }
};








class Ring : public Hier_Object
{
public:
    vector <Ring_Stop *> ringStops; 
    Ring(unsigned numStops, unsigned queueDepth, unsigned sinkBound, string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p,network)
        {
            unsigned w = numBitsRequired( numStops);

            vector <Channel *> c (numStops+1);
            for (unsigned i = 0; i<numStops; i++) {
                c[i] = new Channel("c"+itos(i), w, this, network);
            }
            c[numStops] = c[0]; 
        
            ringStops.resize(numStops);
            for (unsigned i = 0; i<numStops; i++) {
                cout << "ring stop from channel " << i << " to channel " << i+1 << "\n";
                ringStops[i] = new Ring_Stop(c[i], c[i+1], i, numStops, queueDepth, sinkBound,
                                             "ringstop"+itos(i), p, network); 
                c[i] -> setNonBlocking(true);
            }
        
        }
};


class Tb_Ring : public Hier_Object
{
public:

    Ring *r;
    void addManualRingLemmas();
    unsigned  numStages; 

    Network *foo;
    Tb_Ring(string n, Hier_Object *p, Network *network, vector <pair <string,string> > & tbArgs)
        : Hier_Object(n,p,network)
        {
            foo = network;
            unsigned  queueDepth = 2;
            unsigned  sinkBound  = 2; 
            numStages  = 3; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "queueDepth") queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")   sinkBound = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "numStages")   numStages = stoi(tbArgs[i].second);
                else ASSERT(0);
            }
        
            r = new Ring(numStages, queueDepth, sinkBound, n+"_ring",this,network);        
        }
};




// add the lemmas for the ring stages based on current state of
// destination and location of packet in the ring
void Tb_Ring::addManualRingLemmas()
{

    unsigned int num_of_row = numStages+1;
    unsigned int num_of_col = numStages;
    unsigned init_value = 0;

    // row is the state of the destination
    // column is distance from ring stop to destination.
    vector< vector<unsigned> > matrix;
    matrix.resize( num_of_row , vector<unsigned>( num_of_col , init_value ) );

    // the 1st state in the limit cycle
    unsigned row = numStages;
    unsigned col = numStages-1;
    matrix[row][col] = 2;

    while(true) {
        unsigned _row = (row + 1) % num_of_row;
        unsigned _col = (num_of_col + col - 1) % num_of_col;
        if (matrix[_row][_col] != 0) break;
        matrix[_row][_col] = matrix[row][col] + 1;
        row = _row;
        col = _col;
    }

    for (unsigned i = 0; i < num_of_row; i++) {
        for (unsigned j = 0; j < num_of_col; j++) {
            cout << i << "," << j << ": " << setw(2) << matrix[i][j] << "   ";
        }
        cout << "\n";
    }



    unsigned wAddr = r->ringStops[0]->rsv->getWidth();

    unsigned tl = 0;

    for (unsigned iDest = 0; iDest < r->ringStops.size(); iDest++) {
        for (unsigned iDestState = 0; iDestState <= r->ringStops.size(); iDestState++) {
            
            for (unsigned iStop = 0; iStop < r->ringStops.size(); iStop++) {
                unsigned distToDest = (numStages + iDest - iStop - 1) % numStages;
                cout << "iDest " << iDest << "   iStop " << iStop << "   distToDest " << distToDest << "\n";
                Expr *mine = new Eq_Expr(
                    new Extract_Expr( r->ringStops[iStop]->packets->qslots[0], wAddr-1, 0),
                    new Bvconst_Expr( iDest, wAddr)
                    );
                Expr *stateIsI = new Eq_Expr(r->ringStops[iDest]->rsv, new Bvconst_Expr(iDestState, wAddr));
                Expr *en = new And_Expr(mine, stateIsI);
                unsigned b = matrix[iDestState][distToDest];
                r->ringStops[iStop]->packets->buildSlotAgeAssertions(0, en, b, "phil");

                
            }
        }

        // add ingress bounds 
        unsigned maxRingBound = 1 + num_of_row * num_of_col;
        for (int j = r->ringStops[iDest]->ingress->depth-1; j >= 0; j--) {
            int x = r->ringStops[iDest]->ingress->depth - j;
            unsigned b = maxRingBound + x * (r->ringStops[iDest]->sinkBound+1);
            r->ringStops[iDest]->ingress->buildSlotAgeAssertions(j, b, "phil");
            cout << "ingress slot " << j << " = " << b << "\n";
            tl = max(tl, b);
        }
    }

    cout << "max age bound at any location is "<< tl << "\n"; // DO NOT REMOVE
    foo->buildPhiG(tl, "phig_tl"); //asserts bound everywhere

    // adding dummy because ring is only ckt without any psil
    Signal *dummy = (new Signal("dummy", new Bvconst_Expr(1,1)))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("psil");

    return;   
}













class Vc_Core : public Hier_Object
{
public:
    Queue *ingress1; // make these public to use numItems in numInvs
    Queue *ingress2;


    /// @param x output channel with data =  0 (1) when packet enters ingress1 (ingress2). 
    Vc_Core(Channel *in1, Channel *in2, Channel *x, Channel *out1, Channel *out2,
            unsigned depth1, unsigned depth2,
            string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p,network)
        {


            unsigned w = in1->getDWidth();
            ASSERT(w == in2->getDWidth());

            Channel *b1 = new Channel("b1",  this, network);
            Channel *b2 = new Channel("b2",  this, network);
            Channel *c1 = new Channel("c1",  this, network);
            Channel *c2 = new Channel("c2",  this, network);
            Channel *d1 = new Channel("d1",w,this, network);
            Channel *d2 = new Channel("d2",w,this, network);
            Channel *l1 = new Channel("l1",w,this, network);  
            Channel *l2 = new Channel("l2",w,this, network);  
            Channel *h1 = new Channel("h1",  this, network);
            Channel *h2 = new Channel("h2",  this, network);
            Channel *k1 = new Channel("k1",w,this, network);
            Channel *k2 = new Channel("k2",w,this, network);

            Channel *d1tag = new Channel("d1tag",w+1,this, network);
            Channel *d2tag = new Channel("d2tag",w+1,this, network);
            Channel *k1tag = new Channel("k1tag",w+1,this, network);
            Channel *k2tag = new Channel("k2tag",w+1,this, network);

            new Join(b1,in1,d1,"j1",this, network);        
            new Join(b2,in2,d2,"j2",this, network);        

            // add an extra bit to indicate which input packet comes
            // from. route to an output based on this tag, and then remove
            // the tag from the data
            Channel *e = new Channel("e",w+1,this, network);
            e -> setBreakCycleTrdy(true);


            Function *f1 = (new Function(d1,d1tag,"d1tagger",this,network)) -> setFunction(FUNCTION_TAG, 1,0);
            Function *f2 = (new Function(d2,d2tag,"d2tagger",this,network)) -> setFunction(FUNCTION_TAG, 1,1);
            new Merge(d1tag,d2tag,e,"merge",this,network);
            Switch *sw = new Switch(e,k1tag,k2tag,"sw",this,network);
            (sw) -> setRoutingFunction( 0,0, new Bvconst_Expr(0,1) ); // check the tag
            Function *f3 = (new Function(k1tag,k1,"k1untagger",this,network))  -> setFunction(FUNCTION_UNTAG, 1);
            Function *f4 = (new Function(k2tag,k2,"k2untagger",this,network))  -> setFunction(FUNCTION_UNTAG, 1);

            // before sending data to ingress, get the ordering token out
            Channel *x1  = new Channel("x1",   this, network);
            Channel *x2  = new Channel("x2",   this, network);
            Channel *x1d = new Channel("x1d",  this, network);
            Channel *x2d = new Channel("x2d",  this, network);
            Channel *kk1 = new Channel("kk1",w,this, network);
            Channel *kk2 = new Channel("kk2",w,this, network);
            new Fork(k1,x1,kk1,"ff1",this, network);
            new Fork(k2,x2,kk2,"ff2",this, network);
            Function *f5 = (new Function(x1,x1d,"x1d",this,network))  -> setFunction(FUNCTION_ASSIGN, 1,0);
            Function *f6 = (new Function(x2,x2d,"x2d",this,network))  -> setFunction(FUNCTION_ASSIGN, 1,1);
            new Merge(x1d,x2d,x,"merge",this,network); // the ordering token
            //x->setNonBlocking(true); // maybe needed for network with ordering
        
            ingress1 = (new Queue(kk1,l1,depth1,"ingress1",this, network)) -> setPacketType(PACKET_DATA);
            ingress2 = (new Queue(kk2,l2,depth2,"ingress2",this, network)) -> setPacketType(PACKET_DATA);

            new Fork(l1,h1,out1,"f1",this, network);
            new Fork(l2,h2,out2,"f2",this, network);

            Credit_Counter *cc1 = new Credit_Counter(h1, c1, depth1, "cc1", this, network);
            Credit_Counter *cc2 = new Credit_Counter(h2, c2, depth2, "cc2", this, network);
            Queue *tokens1 = (new Queue(c1,b1,depth1,"tokens1",this, network)) -> setPacketType(PACKET_TOKEN);
            Queue *tokens2 = (new Queue(c2,b2,depth2,"tokens2",this, network)) -> setPacketType(PACKET_TOKEN);

            Expr * tokensPlusPackets1 = new Bvadd_Expr(tokens1->numItems, ingress1->numItems );
            Expr * tokensPlusPackets2 = new Bvadd_Expr(tokens2->numItems, ingress2->numItems );

            Signal *numInv1 = (new Signal(name+"numInv1", new Eq_Expr(cc1->oc->numItems, tokensPlusPackets1)))
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi")
                -> addAssertionClassTag("numinv");

            Signal *numInv2 = (new Signal(name+"numInv2", new Eq_Expr(cc2->oc->numItems, tokensPlusPackets2)))
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi")
                -> addAssertionClassTag("numinv");

        }
};


class Virtual_Channel : public Hier_Object
{
public:
    Virtual_Channel(Channel *in1, Channel *in2, Channel *out1, Channel *out2,
                    unsigned depth1, unsigned depth2,
                    string n, Hier_Object *p, Network *network)
        : Hier_Object(n,p, network)
        {

            ASSERT( in1->getDWidth() == in2->getDWidth() );
        
            Channel *x = new Channel("x",this, network); //the ordering output
            new Vc_Core(in1, in2, x, out1, out2, depth1, depth2, "vccore", this, network);
            (new Sink(x,"sinkx",this,network)) ->setOracleType(ORACLE_EAGER);
        }
};




class Tb_Virtual_Channel : public Hier_Object
{
public:
    Tb_Virtual_Channel(string n, Hier_Object *p, Network *network, vector <pair <string,string> > & tbArgs )
        : Hier_Object(n,p, network)
        {

            unsigned depth1 = 2; 
            unsigned depth2 = 2; 
            unsigned sinkBound1 = 3; 
            unsigned sinkBound2 = 5; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "depth1")       depth1     = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "depth2")       depth2     = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound1")   sinkBound1 = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound2")   sinkBound2 = stoi(tbArgs[i].second);
                else ASSERT(0);
            }

            Channel *in1  = (new Channel("in1", 4,this, network)); 
            Channel *in2  = (new Channel("in2", 4,this, network)); 
            Channel *out1 = (new Channel("out1",4,this, network)); 
            Channel *out2 = (new Channel("out2",4,this, network)); 

            new Virtual_Channel(in1, in2, out1, out2, depth1, depth2, "vc", this, network);
            Source *src1 = (new Source(in1,"src1",this,network)) ->setOracleType(ORACLE_NONDETERMINISTIC);
            Source *src2 = (new Source(in2,"src2",this,network)) ->setOracleType(ORACLE_NONDETERMINISTIC);
            Sink *sink1 = (new Sink(out1,"sink1",this,network)) ->setOracleType(ORACLE_BOUNDED_RESPONSE, sinkBound1);
            Sink *sink2 = (new Sink(out2,"sink2",this,network)) ->setOracleType(ORACLE_BOUNDED_RESPONSE, sinkBound2);

        }
};









class Credit_Loop : public Hier_Object
{
public:
    Credit_Loop(Channel *in, Channel *out, string n, Hier_Object *p, Network *network, int queueDepth = 2)
        : Hier_Object(n,p,network)
        {

            unsigned int w = in->getDWidth();
            ASSERT(w == out->getDWidth());
            Channel *b = (new Channel("b", w, this, network));
            Channel *c = (new Channel("c", w, this, network));
            Channel *e = (new Channel("e",    this, network));
            Channel *j = (new Channel("j",    this, network));
            Channel *k = (new Channel("k",    this, network));

            Queue *tokens = (new Queue(j,k,queueDepth,"availableTokens",this, network))
                -> setPacketType(PACKET_TOKEN);

            new Join(k,in,b,"xj",this, network);

            Queue *packets = (new Queue(b,c,queueDepth,"packets",this, network))
                -> setPacketType(PACKET_DATA);

            new Fork(c,e,out,"xf",this, network);

            Credit_Counter *cc = new Credit_Counter(e, j, queueDepth, "cc", this, network);
        
            Expr * tokensPlusPackets = new Bvadd_Expr(tokens->numItems, packets->numItems );
            Signal *numInv = (new Signal("numInv", new Eq_Expr(cc->oc->numItems, tokensPlusPackets)))
                -> setSignalIsAssertion(true)
                -> addAssertionClassTag("psi")
                -> addAssertionClassTag("numinv");        
        }
};



class Tb_Credit_Loop : public Hier_Object
{
public:
    Tb_Credit_Loop(string n, Hier_Object *p, Network *network, vector <pair <string,string> > & tbArgs)
        : Hier_Object(n,p,network)
        {

            int queueDepth = 2;
            int  sinkBound = 5; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "queueDepth") queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")   sinkBound = stoi(tbArgs[i].second);
                else ASSERT(0);
            }
        
            Channel *a = (new Channel("a", 2, this, network));
            Channel *d = (new Channel("d", 2, this, network));
            Source *pkt_src = (new Source(a,"pkt_src",this, network))
                -> setOracleType(ORACLE_NONDETERMINISTIC);
            Sink *pkt_sink = (new Sink(d,"pkt_sink",this, network))
                -> setOracleType(ORACLE_BOUNDED_RESPONSE,sinkBound);

            new Credit_Loop(a,d,"cl",this,network,queueDepth);                
        }
};


class Tb_Token_Bucket : public Hier_Object
{
public:
    Tb_Token_Bucket(string n, Hier_Object *p, Network *network, vector <pair <string,string> > & tbArgs )
        : Hier_Object(n,p,network)
        {

            unsigned  queueDepth = 2;
            unsigned   sinkBound = 5; 
            unsigned       sigma = 3;
            unsigned         rho = 5; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "queueDepth")  queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")    sinkBound = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sigma")            sigma = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "rho")                rho = stoi(tbArgs[i].second);
                else ASSERT(0);
            }

            Channel *a = (new Channel("a", 2, this, network));
            Channel *b = (new Channel("b", 2, this, network));
            Channel *c = (new Channel("c", 2, this, network));

            Source *src = (new Source( a, "src",this, network)) ->setOracleType(ORACLE_NONDETERMINISTIC);
            new Token_Bucket_Regulator(a, b, sigma, rho, "lb", this, network);

            Queue *xq = new Queue(b,c,queueDepth,"xq",this, network);
            xq->setPacketType(PACKET_DATA);

            Sink *sink = (new Sink(c,"xsink",this, network))
                -> setOracleType(ORACLE_BOUNDED_RESPONSE,sinkBound);
        
        }
};









class Tb_Join : public Hier_Object
{
public:
    Tb_Join(string n, Hier_Object *p, Network *network) : Hier_Object(n,p,network)
        {
            unsigned int depth = 3;

            Channel *a = new Channel("a",  this,network);
            Channel *b = new Channel("b",4,this,network);
            Channel *c = new Channel("c",4,this,network);
            Channel *d = new Channel("d",4,this,network);

            Source *src_a = new Source(a,"src_a",this,network);
            Source *src_c = new Source(c,"src_c",this,network);
            new Queue(c,b,depth,"q0",this,network);

            new Join(a,b,d,"m1",this,network);
            Sink *sink = new Sink(d,"sink",this,network);

            (sink)   -> setOracleType(ORACLE_BOUNDED_RESPONSE, 3); 
            (src_a)  -> setOracleType(ORACLE_EAGER);
            (src_c)  -> setOracleType(ORACLE_EAGER);
        }    
};


class Tb_Fork : public Hier_Object
{
public:
    Tb_Fork(string n, Hier_Object *p, Network *network) : Hier_Object(n,p,network)
        {

            Channel *a = new Channel("a", 2, this,network);
            Channel *b = new Channel("b",    this,network); // credit out
            Channel *c = new Channel("c", 2, this,network); 

            Source *src = new Source(a,"src",this,network);
            Sink *sink_b = new Sink(b,"sink_c",this,network);
            Sink *sink_d = new Sink(c,"sink_d",this,network);
            new Fork(a,b,c,"fork0",this,network);

            (src)    -> setOracleType(ORACLE_EAGER);
            (sink_b) -> setOracleType(ORACLE_EAGER); 
            (sink_d) -> setOracleType(ORACLE_BOUNDED_RESPONSE, 3); 
        }    
};



class Tb_Switch : public Hier_Object
{
public:
    Tb_Switch(string n, Hier_Object *p, Network *network) : Hier_Object(n,p,network)
        {

            Channel *a = (new Channel("a", 4,this,network));
            Channel *b = (new Channel("b", 4,this,network)); 
            Channel *c = (new Channel("c", 4,this,network)); 
            Channel *d = (new Channel("d", 4,this,network));

            Source *src = new Source(a,"src",this,network);
            Sink *sink_b = new Sink(b,"sink_b",this,network);
            Sink *sink_d = new Sink(d,"sink_d",this,network);
            Switch *sw = new Switch(a,b,c,"sw0",this,network);
            new Queue(c,d,2,"q0",this,network);

            (sw) -> setRoutingFunction( 0,0, new Bvconst_Expr(0,1) );
            (src)    -> setOracleType(ORACLE_EAGER);
            (sink_b) -> setOracleType(ORACLE_EAGER); 
            (sink_d) -> setOracleType(ORACLE_BOUNDED_RESPONSE, 3); 
        }    
};






class Tb_Queue : public Hier_Object
{
public:
    Tb_Queue(string n, Hier_Object *p, Network *network, vector <pair <string,string> > &tbArgs)
        : Hier_Object(n,p,network)
        {

            unsigned int queueDepth = 3;
            int           sinkBound = 3; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "queueDepth") queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")  sinkBound  = stoi(tbArgs[i].second);
                else ASSERT(0);
            }
            cout << "queue depth: " << queueDepth << "\n";
        
            Channel *a = new Channel("a",4,this,network);
            Channel *b = new Channel("b",4,this,network);
            Source *src = new Source(a,"src",this,network);
            Sink *sink = new Sink(b,"sink",this,network);       
            (src) ->setOracleType( ORACLE_NONDETERMINISTIC );
            (sink)->setOracleType( ORACLE_BOUNDED_RESPONSE, sinkBound); 

            new Queue(a,b,queueDepth,"q0",this,network);
        }    
};

class Tb_Queue_Chain : public Hier_Object
{
public:
    Tb_Queue_Chain(string n, Hier_Object *p, Network *network, vector <pair <string,string> > &tbArgs)
        : Hier_Object(n,p,network)
        {

            int  numStages = 2;
            int queueDepth = 2;
            int  sinkBound = 3; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "numStages")   numStages = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "queueDepth") queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")  sinkBound  = stoi(tbArgs[i].second);
                else ASSERT(0);
            }

            Channel *in  = (new Channel("a0",2,this, network));
            Channel *out = (new Channel("an",2,this, network));

            new Queue_Chain(in, out, queueDepth, numStages, "xqc", this, network);

            Source *src = new Source(in,"src",this, network);
            Sink *sink  = new Sink(out,"sink",this, network);

            (src) ->setOracleType( ORACLE_NONDETERMINISTIC); 
            (sink)->setOracleType( ORACLE_BOUNDED_RESPONSE, sinkBound);
        }
};




class Tb_Merge_Tree : public Hier_Object
{
public:
    Tb_Merge_Tree(string n, Hier_Object *p, Network *network, vector <pair <string,string> > &tbArgs)
        : Hier_Object(n,p,network)
        {
            
            int numStages  = 2;
            int queueDepth = 2;
            int  sinkBound = 3; 
            for (unsigned i = 0; i< tbArgs.size(); i++) {
                if      (tbArgs[i].first == "numStages")   numStages = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "queueDepth") queueDepth = stoi(tbArgs[i].second);
                else if (tbArgs[i].first == "sinkBound")   sinkBound = stoi(tbArgs[i].second);
                else ASSERT(0);
            }



            vector <Channel *> ca (numStages);
            vector <Channel *> cb (numStages);
            vector <Channel *> cc (numStages);
            Channel *ccc;

            ccc = new Channel("firstStageInput",4,this, network);
            Source *srcFirstStage = new Source(ccc,"srcFirstStage",this, network);

            for (int i = 0; i < numStages; i++) {
                ca[i] = new Channel("a"+itos(i),4,this, network);
                cb[i] = new Channel("b"+itos(i),4,this, network);
                cc[i] = new Channel("c"+itos(i),4,this, network);

                new Queue( ccc ,   ca[i] , queueDepth     ,"q"+itos(i), this, network);
                new Merge( ca[i] , cb[i] , cc[i] ,"m"+itos(i), this, network);
                Source *s = new Source(cb[i], "src"+itos(i), this, network);
                (s) ->setOracleType( ORACLE_NONDETERMINISTIC );
                ccc = cc[i];
            }
                
            Channel * csink = new Channel("csink",4,this, network);
            new Queue( ccc ,  csink , queueDepth ,"qsink", this, network);
            Sink *sinkLastStage = new Sink(csink,"sinkLastStage",this, network);
            (srcFirstStage) ->setOracleType( ORACLE_NONDETERMINISTIC );
            (sinkLastStage) ->setOracleType( ORACLE_BOUNDED_RESPONSE, sinkBound);
        }    
};











int main (int argc, char **argv)
{

    cout << "\n\n\n======================================================\n";
    g_ckt = new Ckt();

    string network = "tb_ring";
    string fnameOut = "dump.v";

    bool buildLemmas = true; //build args
    unsigned tBound = 0;

    vector < pair <string, bool> > tagStates;
    vector < pair <string, string> > tbArgs; // testbench specific args
    
    std::cout << "\n\n" << argv[0];
    for (int i = 1; i < argc; i++) 
    { 
        string s = string(argv[i]);
        cout << "parsing arg: " << argv[i] << "\n";

        if        ( s == "--dump")    { fnameOut = argv[++i];
        } else if ( s == "--network") { network  = argv[++i];

        } else if ( s == "--dont_build_lemmas")   { buildLemmas = false; // 
        } else if ( s == "--t_bound")             { tBound = atoi(argv[++i]); // assert cmd line age bound

        } else if ( s == "--t_range")   { g_ckt->setTRange( atoi(argv[++i]) ); // when does counter roll over
        } else if ( s == "--t_style")   { g_ckt->setTimestampType( argv[++i] ); 
        } else if ( s == "--disable_tag")  { tagStates.push_back( make_pair( argv[++i], false ));
        } else if ( s == "--enable_tag")   { tagStates.push_back( make_pair( argv[++i], true ));
        } else if ( s == "--enable_tag")   { tagStates.push_back( make_pair( argv[++i], true ));
                        
        } else {
            // cmd line args that are specific to the network
            s.erase(std::remove(s.begin(), s.end(), '-'), s.end());
            string v = argv[++i];
            cout << "tbArgs["<<s<< "] = "<< v << "\n";
            tbArgs.push_back(make_pair(s,v));
        }
    }

    
    cout << "\n\nnetwork = " << network << "\n";
    Network *top = new Network();

    // create one of the networks depending on cmd line args. Top level
    // network is within this null root because every object must be
    // contained within one
    Tb_Ring *tb_ring = 0;
    if (0) {}
    else if (network == "tb_join")            {  new Tb_Join(         "tb", top, top ); } 
    else if (network == "tb_fork")            {  new Tb_Fork(         "tb", top, top ); } 
    else if (network == "tb_switch")          {  new Tb_Switch(       "tb", top, top ); } 

    else if (network == "tb_ring")            {  tb_ring = new Tb_Ring( "tb", top, top, tbArgs ); } 
    else if (network == "tb_virtual_channel") {  new Tb_Virtual_Channel( "tb", top, top, tbArgs ); } 
    else if (network == "tb_token_bucket")    {  new Tb_Token_Bucket(    "tb", top, top, tbArgs ); } 
    else if (network == "tb_credit_loop")     {  new Tb_Credit_Loop(     "tb", top, top, tbArgs ); } 
    else if (network == "tb_tree")            {  new Tb_Merge_Tree(      "tb", top, top, tbArgs ); } 
    else if (network == "tb_queue")           {  new Tb_Queue(           "tb", top, top, tbArgs ); } 
    else if (network == "tb_queue_chain")     {  new Tb_Queue_Chain(     "tb", top, top, tbArgs ); } 

    else {  ASSERT(0);  }
 
    top->printNetwork();

    g_ckt -> buildNetworkLogic( top );

    if (tb_ring != 0) { tb_ring->addManualRingLemmas(); }
    if (buildLemmas and (tb_ring == 0)) { top->propagateChannelProperties(); } //ring cant use auto rules

    if (tBound > 0)  { top->buildPhiG(tBound, "phig_x");  }   
    
    g_ckt -> setAssertionStates( tagStates );     
    g_ckt -> printAssertions( "dump.assertions" );     
    g_ckt -> printSignals(  "dump.signals" );     
    g_ckt -> printExpressions(  "dump.expressions" );     

    cout << g_ckt->sprintAssertions( );     

    cout << "dumping verilog\n";
    g_ckt -> dumpAsVerilog( fnameOut );     

    cout << "ending main.cpp normally\n";
    return 0;
};

  
 
  
