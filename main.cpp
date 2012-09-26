
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


 

#include "main.h"
#include "expressions.h"
#include "primitives.h"
#include "channel.h"
using namespace std;

// globals
Network *g_network; 
Ckt *g_ckt;
ofstream g_outQos;

    
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

// for printing
string validTimeOrDash (unsigned int n) {
  return (n == T_PROP_NULL) ? "-" : itos(n); 
}



//root not has no name... (to help keep flat names shorter)
Hier_Object::Hier_Object( ) {
  name = "";        
};
Hier_Object::Hier_Object( const string n, Hier_Object *parent) {
  name = parent->name + n + HIER_SEPARATOR;
  parent->addChild(this);
};

void Hier_Object::addChild (Hier_Object *x) { children.push_back(x); }






Init_Port::Init_Port(string n, Channel *c, Primitive *p) : Port(n, c, p) {
  ASSERT2(c->initiator == 0, "channel "+(c)->name+ " cannot have a second initiator");
  p->init_ports.push_back(this);
  c->initiator = this;
};

Targ_Port::Targ_Port(string n, Channel *c, Primitive *p) : Port(n, c, p) {
  ASSERT2(c->target == 0, "channel "+(c)->name+ " cannot have a second target. Prev target=");
  p->targ_ports.push_back(this);
  c->target = this;
};










// a counter that asserts an output if t cycles elapse between a and b.
// used for enforcing bounds on sources and sinks, and for checking
// channel bounds.

Signal * intervalMonitor( Expr * a, Expr * b, unsigned int t, string name) {

  unsigned int w = numBitsRequired(t+1); 
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





        







void Ckt::buildNetworkLogic(Network *n) {


  // infer how many bits are needed for the clock 
  unsigned int maxLatency = 0;
  for (vector <Queue*>::iterator it = n->queues.begin(); it != n->queues.end(); it++ ) 
    for (int i = (*it)->qslots.size()-1 ; i >= 0 ; i--) 
      maxLatency = max(maxLatency, (*it)->slotQos[i]->getMaxAge() ); 


  if (g_ckt->voptions->isEnabledPhiLQueue and not g_ckt->voptions->isEnabledPhiGQueue)
    {
      cout << "no global tMax was entered, using " << maxLatency << "\n";
      voptions->setTMax(maxLatency); 
    }

  // using phiG but have not manually assigned it
  if (g_ckt->voptions->isEnabledPhiGQueue and not g_ckt->voptions->hasTMax())
    g_ckt->voptions->setTMax(maxLatency);

  if (g_ckt->voptions->isEnabledPhiLQueue and g_ckt->voptions->isEnabledPhiGQueue)
    ASSERT2(maxLatency <= voptions->getTMax(),"tmax was set smaller than largest lemma time"); 


  if (voptions->hasTMax()) 
    maxLatency = max(maxLatency, voptions->getTMax() );

  unsigned int wClk = numBitsRequired( maxLatency );
      
  cout << "\nglobal latency bound (tMax) is        " << voptions->getTMax() << "\n";
  cout << "largest time from latency lemmas is     " << maxLatency << "\n";
  cout << "clock will use                          " << wClk << " bits\n\n";



  this->oracleBus = new Oracle_Signal("oracles");
  this->tCurrent = new Seq_Signal("tCurrent");

  Signal *tCurrentNxt = (new Signal("tCurrentNxt"));
  tCurrent->setWidth(wClk);
  tCurrentNxt->setWidth(wClk);

  tCurrent 
    -> setResetExpr( new Bvconst_Expr(0,wClk ) )
    -> setNxtExpr( tCurrentNxt );

  Expr *e = ( new Bvadd_Expr( tCurrent , new Bvconst_Expr(1,wClk) )) 
    -> setWidth(wClk);
  
  tCurrentNxt
    -> setExpr( e );



  // if channel carries packet, widen it for the timestamp
  for (vector <Channel*>::iterator it = n->channels.begin(); it != n->channels.end(); it++ )
    {
      if ((*it)->getPacketType() == PACKET_DATA) 
	{
	  (*it)->widenForTimestamp(wClk);
	}
    }



  
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







// returns expression for (a - b) % maxval
Expr * BvsubExprModM (Expr *a, Expr *b , unsigned int maxval, string name) {

  unsigned int w = a->getWidth();
  ASSERT(w == b->getWidth());
   
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

  // this is needed to rule bad states in induction
  Signal *withinRange = (new Signal(name+"withinRange"))
    -> setExpr( new Lt_Expr (a, new Bvconst_Expr(m,w)));
  
  if (g_ckt->voptions->isEnabledPsi)
    withinRange->assertSignalTrue();
		


  Expr *rollover = new Eq_Expr( a , new Bvconst_Expr(m-1,w) );
  Expr *plus1 = new Bvadd_Expr(w, a , new Bvconst_Expr(1,w));  
  
  Signal *incrModM = (new Signal(name+"incrModM"))
    -> setExpr( (new Case_Expr())		 
		-> setDefault(plus1)
		-> addCase(rollover, new Bvconst_Expr(0,w) ) -> setWidth(w)
		);


  return incrModM;
};









// print the QoS for all the channels and queue slots
void Network::printQos( ostream &f) 
{

  for (vector <Channel*>::iterator it = (this)->channels.begin(); it != (this)->channels.end(); it++ ) 
    f << (*it)->name << " w= " << (*it)->data->getWidth() << "\n"; 


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

  // initially only the sources/sinks need to be updated with own qos guarantees
  for (vector <Sink*>::iterator it = sinks.begin(); it != sinks.end(); it++ ) 
    (*it)->propagateLatencyLemmas();
  for (vector <Source*>::iterator it = sources.begin(); it != sources.end(); it++ ) 
    (*it)->propagateLatencyLemmas();
    

  // modifiedChannels keeps track of what signals channels need to propagate
  
  unsigned int cnt = 0;
  while (g_network->modifiedChannels.size() > 0) 
    {

      g_outQos << "\n on iter " << cnt
	       << " modifiedChannels.size = " << g_network->modifiedChannels.size(); 
      for (set<Channel*>::iterator it = g_network->modifiedChannels.begin(); 
	   it != g_network->modifiedChannels.end(); it++) 
	{
	  g_outQos << "\n\t" << (*it)->name ;
	}
      g_outQos << "\n";


      set<Channel*>::const_iterator c  = g_network->modifiedChannels.begin();
      g_network->modifiedChannels.erase(*c);
        
      Primitive *iprim = (*c)->initiator->owner;
      Primitive *tprim = (*c)->target->owner;
      iprim->propagateLatencyLemmas( );
      tprim->propagateLatencyLemmas( );

      if (cnt++ >1000) ASSERT(0); 
    }

  //printQos(cout);
  
  printQos(g_outQos);

  cout << "\n";
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



// void Numeric_Invariant ( Signal  ) {

//   unsigned int w = o->channel->data->getWidth();

//   ASSERT2 (a->channel->getPacketType() == PACKET_CREDIT, "join must be restricted join");
//   // second input can be a second credit token or else data packet
//   //ASSERT (w == a->channel->data->getWidth() );
//   ASSERT (w == b->channel->data->getWidth() );

//   //     unsigned int w = o->channel->getDataWidth();
//   //     ASSERT (w == a->channel->getDataWidth() );
//   //     ASSERT (w == b->channel->getDataWidth() );

//   Signal *a_irdy = (new Signal(name+"a_irdy"))->setWidth(1);
//   Signal *a_data = (new Signal(name+"a_data"))->setWidth(1);
//   Signal *a_trdy = (new Signal(name+"a_trdy"))->setWidth(1);
    
//   //   ASSERT(1 == a->channel->irdy->getWidth());
//   //   ASSERT(w == a->channel->data->getWidth());
//   //   ASSERT(1 == a->channel->trdy->getWidth());

//   a_irdy            -> setExpr( new Id_Expr( a->channel->irdy ) );
//   a_data            -> setExpr( new Id_Expr( a->channel->data ) );
//   a->channel->trdy  -> setExpr( new Id_Expr( a_trdy           ) );

//   Signal *b_irdy = new Signal(name+"b_irdy");
//   Signal *b_data = new Signal(name+"b_data");
//   Signal *b_trdy = new Signal(name+"b_trdy");
    
//   b_irdy            -> setExpr( new Id_Expr( b->channel->irdy ) );
//   b_data            -> setExpr( new Id_Expr( b->channel->data ) );
    
//   Signal *o_irdy = new Signal(name+"o_irdy");
//   Signal *o_data = new Signal(name+"o_data");
//   Signal *o_trdy = new Signal(name+"o_trdy");

//   o_trdy             -> setExpr( new Id_Expr( o->channel->trdy ) );


//   // the logic for the join:
//   o_data       -> setExpr ( new Id_Expr( b_data) );
//   o_irdy       -> setExpr ( new And_Expr( a_irdy , b_irdy) );
//   a_trdy       -> setExpr ( new And_Expr( o_trdy , b_irdy) );
//   b_trdy       -> setExpr ( new And_Expr( o_trdy , a_irdy) );    		    

//   b->channel->trdy  -> setExpr( new Id_Expr( b_trdy           ) );
//   o->channel->data   -> setExpr( new Id_Expr( o_data           ) );
//   o->channel->irdy   -> setExpr( new Id_Expr( o_irdy           ) );
        
//   return;
// }














class Credit_Counter : public Composite {
public:
  unsigned int depth;
  Queue *oc; //available outside of construct for use in numeric invariants

  Credit_Counter(Channel *in, Channel *out, unsigned int de, string n, Hier_Object *p) :  Composite(n,p) {

    depth = de;
    Channel *a = new Channel("a",this);
    Channel *b = new Channel("b",this);
    Channel *c = new Channel("c",this);
    Channel *d = new Channel("d",this);
    a->setPacketType(PACKET_CREDIT);
    b->setPacketType(PACKET_CREDIT);
    c->setPacketType(PACKET_CREDIT);
    d->setPacketType(PACKET_CREDIT);

    Source *token_src = new Source(a,"token_src",this);
    (token_src)->setTypeEager();

    Sink *token_sink = new Sink(b,"token_sink",this);
    (token_sink)->setTypeEager();
	 
    new Fork(a,out,c,"f1",this);
    oc = new Queue(c,d,depth,"outstanding_credits",this);
    oc->setPacketType(PACKET_CREDIT);
    new Join(d,in,b,"j1",this);        
  }
};

class Credit_Loop : public Composite {
public:
  Credit_Loop(string n, Hier_Object *p, bool useNumInv = false) : Composite(n,p) {
 
    Channel *a = new Channel("a",2,this);
    Channel *b = new Channel("b",2,this);
    Channel *c = new Channel("c",2,this);
    Channel *d = new Channel("d",2,this);
    a -> setPacketType(PACKET_DATA);
    b -> setPacketType(PACKET_DATA);
    c -> setPacketType(PACKET_DATA);
    d -> setPacketType(PACKET_DATA);

    Channel *e = new Channel("e",1,this);
    Channel *f = new Channel("f",1,this);
    Channel *g = new Channel("g",1,this);

    e -> setPacketType(PACKET_CREDIT);
    f -> setPacketType(PACKET_CREDIT);
    g -> setPacketType(PACKET_CREDIT);

    Source *pkt_src = new Source(a,"pkt_src",this);
    pkt_src->setTypeNondeterministic();
    Queue *tokens = new Queue(f,g,2,"available_tokens",this);
    tokens -> setPacketType(PACKET_CREDIT);

    new Join(g,a,b,"j",this);

    Queue *packets = new Queue(b,c,2,"packets",this);
    packets->setPacketType(PACKET_DATA);

    new Fork(c,e,d,"f",this);
    Sink *pkt_sink = new Sink(d,"pkt_sink",this);
    //(pkt_sink)->setTypeEager();
    (pkt_sink)->setTypeBounded(2);
    Credit_Counter *cc = new Credit_Counter(e, f, 2, "cc", this);

    // numeric invariant for credit loop -- Chatterjee et al CAV'10
    if (useNumInv) 
      {
	Signal *numInv = new Signal();
	numInv->setName("creditLoopNumInv");
	ASSERT(packets->numItems->getWidth() == tokens->numItems->getWidth());
	Expr *pktsPlusTokens = (new Bvadd_Expr(packets->numItems, tokens->numItems)) -> setWidth(packets->numItems->getWidth() );
	numInv->setExpr( new Eq_Expr(pktsPlusTokens, cc->oc->numItems));
	numInv->assertSignalTrue();
      }
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

    (sink_d)->setTypeBoundedResponse(3);
    (src_a)->setTypeEager();
    (src_c)->setTypeEager();
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










class Ex_Queue : public Composite {
  int queue_size;
public:
  Ex_Queue(string n, Hier_Object *p, int q=3) : Composite(n,p) {
    int queue_size = q;
    int lb_sink = 2; 
    cout << "queue size: " << queue_size << "\n";
    
    Channel *a = new Channel("a",4,this);
    Channel *b = new Channel("b",4,this);
    a->setPacketType(PACKET_DATA);
    b->setPacketType(PACKET_DATA);

    Source *src_a = new Source(a,"src_a",this);
    new Queue(a,b,queue_size,"q1",this);
    
    Sink *sink_b = new Sink(b,"sink_b",this);
    (sink_b)->setTypeBoundedResponse(lb_sink);
    //(sink_b)->setTypeBounded(lb_sink);
    (src_a)->setTypeNondeterministic();
  }
    
};




class Ex_Tree : public Composite {
public:
  Ex_Tree(string n, Hier_Object *p, int numStages = 2) : Composite(n,p) {
    vector <Channel *> ca (numStages);
    vector <Channel *> cb (numStages);
    vector <Channel *> cc (numStages);
    Channel *ccc;

    ccc = new Channel("firstStageInput",4,this);
    Source *srcFirstStage = new Source(ccc,"srcFirstStage",this);

    for (int i = 0; i < numStages; i++) 
      {
	ca[i] = new Channel("a"+itos(i),4,this);
	cb[i] = new Channel("b"+itos(i),4,this);
	cc[i] = new Channel("c"+itos(i),4,this);

	new Queue( ccc , ca[i] , 2     ,"q"+itos(i) , this);
	new Merge( ca[i] , cb[i] , cc[i] ,"m"+itos(i),this);
	Source *s = new Source(cb[i] , "src"+itos(i), this);
	(s)->setTypeNondeterministic();
	ccc = cc[i];
      }
                
    Sink *sinkLastStage = new Sink(ccc,"sinkLastStage",this);
    (sinkLastStage)->setTypeBoundedResponse(1);
    (srcFirstStage)->setTypeNondeterministic();
  }    
};



class Ex_Queue_Chain : public Composite {
public:
  Ex_Queue_Chain(string n, Hier_Object *p, int numQueues = 1) : Composite(n,p) {

    cout << "numQueues: " << numQueues << "\n";

    vector < Channel *> ch (numQueues+1);
    for (int i=0; i< numQueues+1; i++) 
      ch[i] = new Channel("ch"+itos(i) ,4,this);
    
    for (int i=0; i< numQueues; i++) 
      new Queue(ch[i], ch[i+1] , 2 , "q"+itos(i) , this);
        
    Source *src_a = new Source(ch[0],"src_a",this);
    (src_a)->setTypeNondeterministic();
                    
    Sink *sink_x = new Sink(ch[numQueues],"sink_x",this);
    (sink_x)->setTypeBoundedResponse(1);        
  }
};




class Two_Queues : public Composite {
public:
  Two_Queues(string n, Hier_Object *p) 
    : Composite(n,p) {
    
    Channel *a = new Channel("a",4,this);
    Channel *b = new Channel("b",4,this);
    Channel *c = new Channel("c",4,this);
    a->setPacketType(PACKET_DATA);
    b->setPacketType(PACKET_DATA);
    c->setPacketType(PACKET_DATA);

    Source *src = new Source(a,"src",this);
    new Queue(a,b,2,"q1",this);
    new Queue(b,c,2,"q2",this);
    Sink *snk = new Sink(c,"snk",this);
    (snk)->setTypeBoundedResponse(2);
    (src)->setTypeNondeterministic();
  }    
};






int main (int argc, char **argv)
{

  cout << "\n\n\n======================================================\n";
  g_ckt = new Ckt();

  string network = "credit_loop";
  string fnameOut = "dump.v";


  std::cout << argv[0];
  for (int i = 1; i < argc; i++) 
    { 
      string s = string(argv[i]);
      cout << "\nparsing arg: " << argv[i] << "\n";

      if        ( s == "--dump")    { fnameOut = argv[++i];
      } else if ( s == "--network") { network  = argv[++i];
      } else if ( s == "--t_max")                          { g_ckt->voptions->setTMax( atoi(argv[++i]) );
      } else if ( s ==  "--enable_persistance")            { g_ckt->voptions->enablePersistance();
      } else if ( s == "--disable_persistance")            { g_ckt->voptions->disablePersistance();
      } else if ( s ==  "--enable_lemmas")                 { g_ckt->voptions->enablePhiLQueue();
      } else if ( s == "--disable_lemmas")                 { g_ckt->voptions->disablePhiLQueue();
      } else if ( s ==  "--enable_phig")                   { g_ckt->voptions->enablePhiGQueue();
      } else if ( s == "--disable_phig")                   { g_ckt->voptions->disablePhiGQueue();
      } else if ( s ==  "--enable_psi")                    { g_ckt->voptions->enablePsi() ;
      } else if ( s == "--disable_psi")                    { g_ckt->voptions->disablePsi(); 
      } else if ( s == "--enable_bound_channel")           { g_ckt->voptions->enableBoundChannel();
      } else if ( s == "--disable_bound_channel")          { g_ckt->voptions->disableBoundChannel();
      } else if ( s == "--enable_response_bound_channel")  { g_ckt->voptions->enableResponseBoundChannel();
      } else if ( s == "--disable_response_bound_channel") { g_ckt->voptions->disableResponseBoundChannel();
      } else {
	ASSERT2(0,"cmd line argument "+s+" is not understood\n");
      }
    }

  g_ckt -> voptions->printSettings();

  cout << "\n\nnetwork = " << network << "\n";
  g_network = new Network();


  // create one of the networks depending on cmd line args. Top level
  // network is within this null root because every object must be
  // contained within one
  Composite *hier_root = new Composite();
  if      (network == "credit_loop")        {  new Credit_Loop(     "top",hier_root ); } 
  else if (network == "credit_loop_numinv") {  new Credit_Loop(     "top",hier_root , true ); } 
  else if (network == "two_queues")         {  new Two_Queues(      "top",hier_root ); } 
  else if (network == "ex_join")            {  new Ex_Join(         "top",hier_root ); } 
  else if (network == "ex_fork")            {  new Ex_Fork(         "top",hier_root ); } 

  else if (network == "ex_tree")            {  new Ex_Tree(         "top",hier_root ); } 
  else if (network == "ex_tree1")           {  new Ex_Tree(         "top",hier_root , 1 ); } 
  else if (network == "ex_tree2")           {  new Ex_Tree(         "top",hier_root , 2 ); } 
  else if (network == "ex_tree3")           {  new Ex_Tree(         "top",hier_root , 3 ); } 
  else if (network == "ex_tree4")           {  new Ex_Tree(         "top",hier_root , 4 ); } 
  else if (network == "ex_tree5")           {  new Ex_Tree(         "top",hier_root , 5 ); } 
  else if (network == "ex_tree6")           {  new Ex_Tree(         "top",hier_root , 6 ); } 
  else if (network == "ex_tree7")           {  new Ex_Tree(         "top",hier_root , 7 ); } 
  else if (network == "ex_tree8")           {  new Ex_Tree(         "top",hier_root , 8 ); } 
  else if (network == "ex_tree9")           {  new Ex_Tree(         "top",hier_root , 9 ); } 

  else if (network == "ex_queue")           {  new Ex_Queue(        "top",hier_root , 2 ); } 
  else if (network == "ex_queue2")          {  new Ex_Queue(        "top",hier_root , 2 ); } 
  else if (network == "ex_queue3")          {  new Ex_Queue(        "top",hier_root , 3 ); } 
  else if (network == "ex_queue4")          {  new Ex_Queue(        "top",hier_root , 4 ); } 
  else if (network == "ex_queue5")          {  new Ex_Queue(        "top",hier_root , 5 ); } 
  else if (network == "ex_queue6")          {  new Ex_Queue(        "top",hier_root , 6 ); } 
  else if (network == "ex_queue7")          {  new Ex_Queue(        "top",hier_root , 7 ); } 
  else if (network == "ex_queue8")          {  new Ex_Queue(        "top",hier_root , 8 ); } 
  else if (network == "ex_queue9")          {  new Ex_Queue(        "top",hier_root , 9 ); } 

  else if (network == "ex_queue_chain")     {  new Ex_Queue_Chain(  "top",hier_root ); } 
  else if (network == "ex_queue_chain1")    {  new Ex_Queue_Chain(  "top",hier_root , 1 ); } 
  else if (network == "ex_queue_chain2")    {  new Ex_Queue_Chain(  "top",hier_root , 2 ); } 
  else if (network == "ex_queue_chain3")    {  new Ex_Queue_Chain(  "top",hier_root , 3 ); } 
  else if (network == "ex_queue_chain4")    {  new Ex_Queue_Chain(  "top",hier_root , 4 ); } 
  else if (network == "ex_queue_chain5")    {  new Ex_Queue_Chain(  "top",hier_root , 5 ); } 
  else if (network == "ex_queue_chain6")    {  new Ex_Queue_Chain(  "top",hier_root , 6 ); } 
  else if (network == "ex_queue_chain7")    {  new Ex_Queue_Chain(  "top",hier_root , 7 ); } 
  else if (network == "ex_queue_chain8")    {  new Ex_Queue_Chain(  "top",hier_root , 8 ); } 
  else if (network == "ex_queue_chain9")    {  new Ex_Queue_Chain(  "top",hier_root , 9 ); } 
  else {  ASSERT(0);  }

  g_outQos.open("dump.qos");


  g_network->printNetwork();
  g_network->addLatencyLemmas();    // propagate assertions

  g_ckt -> voptions->printSettings();
  g_ckt -> buildNetworkLogic( g_network );
  g_ckt -> voptions->printSettings();
  //  delete g_network;

  g_ckt -> dumpAsVerilog( fnameOut );     
  //g_ckt -> dumpAsUclid( fnameOut );     

  cout << "ending main.cpp normally\n";
  g_outQos.close();

  return 0;
};

  
