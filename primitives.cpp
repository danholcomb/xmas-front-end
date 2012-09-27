

#include <sstream>
#include <vector>
#include <map>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <set>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

#include "main.h"
#include "expressions.h"
#include "primitives.h"
#include "channel.h"




    
void Primitive::printConnectivity(ostream &f) {

  f << "// " << this->name << "\n"; 
  for (vector <Targ_Port*>::iterator port = (this)->targ_ports.begin();port != (this)->targ_ports.end(); port++ )
    f << "//\t" << (*port)->pname << " ---> " << (*port)->channel->name << "\n";
  
  for (vector <Init_Port*>::iterator port = (this)->init_ports.begin();port != (this)->init_ports.end(); port++ )
    f << "//\t" << (*port)->pname << " <--- " << (*port)->channel->name << "\n";
  
  return;
};


void Join::propagateLatencyLemmas( ) {

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




void Join::buildPrimitiveLogic ( ) {

  unsigned int w = o->channel->data->getWidth();

  ASSERT2 (a->channel->getPacketType() == PACKET_CREDIT, "join must be restricted join");
  // second input can be a second credit token or else data packet
  //ASSERT (w == a->channel->data->getWidth() );
  ASSERT (w == b->channel->data->getWidth() );

  //     unsigned int w = o->channel->getDataWidth();
  //     ASSERT (w == a->channel->getDataWidth() );
  //     ASSERT (w == b->channel->getDataWidth() );

  Signal *a_irdy = (new Signal(name+"a_irdy"))->setWidth(1);
  Signal *a_data = (new Signal(name+"a_data"))->setWidth(1);
  Signal *a_trdy = (new Signal(name+"a_trdy"))->setWidth(1);
    
  //   ASSERT(1 == a->channel->irdy->getWidth());
  //   ASSERT(w == a->channel->data->getWidth());
  //   ASSERT(1 == a->channel->trdy->getWidth());

  a_irdy            -> setExpr( new Id_Expr( a->channel->irdy ) );
  a_data            -> setExpr( new Id_Expr( a->channel->data ) );
  a->channel->trdy  -> setExpr( new Id_Expr( a_trdy           ) );

  Signal *b_irdy = new Signal(name+"b_irdy");
  Signal *b_data = new Signal(name+"b_data");
  Signal *b_trdy = new Signal(name+"b_trdy");
    
  b_irdy            -> setExpr( new Id_Expr( b->channel->irdy ) );
  b_data            -> setExpr( new Id_Expr( b->channel->data ) );
    
  Signal *o_irdy = new Signal(name+"o_irdy");
  Signal *o_data = new Signal(name+"o_data");
  Signal *o_trdy = new Signal(name+"o_trdy");

  o_trdy             -> setExpr( new Id_Expr( o->channel->trdy ) );


  // the logic for the join:
  o_data       -> setExpr ( new Id_Expr( b_data) );
  o_irdy       -> setExpr ( new And_Expr( a_irdy , b_irdy) );
  a_trdy       -> setExpr ( new And_Expr( o_trdy , b_irdy) );
  b_trdy       -> setExpr ( new And_Expr( o_trdy , a_irdy) );    		    

  b->channel->trdy  -> setExpr( new Id_Expr( b_trdy           ) );
  o->channel->data   -> setExpr( new Id_Expr( o_data           ) );
  o->channel->irdy   -> setExpr( new Id_Expr( o_irdy           ) );
        
  return;
}




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
        





void Queue::buildPrimitiveLogic ( ) {


  //check the base size (without timestamp), and also the type
  //  ASSERT( i->channel->getDataWidth() == o->channel->getDataWidth() );
  ASSERT( i->channel->data->getWidth() == o->channel->data->getWidth() );
  ASSERT( getPacketType() == i->channel->getPacketType() );
  ASSERT( getPacketType() == o->channel->getPacketType() );
  ASSERT( depth == qslots.size() );

  unsigned int physicalDepth = depth+1; 


  unsigned int wDepth = numBitsRequired(physicalDepth); 
  //  unsigned int wPacket = i->channel->getDataWidth();
  unsigned int wPacket = i->channel->data->getWidth();
  unsigned int wTimestamp = g_ckt->tCurrent->getWidth();
  pair <unsigned int,unsigned int> tBits; //which bits hold timestamp
  

  if (getPacketType() == PACKET_DATA)
    {
      tBits = i->channel->getTBits();
    } 

  

  for (unsigned int j = 0; j<depth; j++) 
    qslots[j] = (new Signal(name+"qslots"+itos(j)) );
    
  Signal *nxt_numItems = (new Signal(name+"nxt_numItems"));

  this->numItems 
    -> setResetExpr( new Bvconst_Expr(0,wDepth) )
    -> setNxtExpr( nxt_numItems );



  Signal *numItemsPlus1 = (new Signal(name+"numItemsPlus1"))
    -> setExpr( new Bvadd_Expr( wDepth, numItems, new Bvconst_Expr(1,wDepth))); 

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
 
  //  o->channel->data     -> setExpr( (new Id_Expr( o_data )) ->setWidth(wPacket)  );
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
  
  Signal *tail_plus_1 = BvIncrExprModM(tail, physicalDepth, name+"tailPlus1ModM");

  
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

  Signal *head_plus_1 = BvIncrExprModM(head, physicalDepth, name+"headPlus1ModM");


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
                                          

  vector <Seq_Signal *> bslots (physicalDepth);
  for (unsigned int i = 0; i<physicalDepth; i++) 
    {
      bslots[i] = new Seq_Signal(name+"bslot"+itos(i));
      bslots[i] ->setWidth(wPacket);

      Signal *nxt_buffer      = (new Signal())
	-> setName(name+"bslot"+itos(i)+"nxt");

      Signal *bvi = (new Signal(name+"const"+itos(i))) 
	-> setExpr( (new Bvconst_Expr(i,wDepth)) );

      Signal *is_tail = (new Signal())
	-> setName(name+"bslot"+itos(i)+"is_tail")
	-> setExpr( new Eq_Expr(bvi, tail));


      Signal *write_bslot = (new Signal())
	-> setName( name+"bslot"+itos(i)+"write" )
	-> setExpr( (new And_Expr())		   
		    -> addInput (writeEnable)
		    -> addInput (is_tail)
		    );
             
      Signal *is_head = (new Signal(name+"bslot"+itos(i)+"is_head") )
	-> setExpr( new Eq_Expr(bvi, head) );


      Signal *read_bslot = (new Signal())
	-> setName(name+"bslot"+itos(i)+"clear")
	-> setExpr( (new And_Expr())
		    -> addInput (is_head)
		    -> addInput (readEnable)
		    );


      nxt_buffer 
	-> setExpr( (new Case_Expr())
		    -> setDefault(bslots[i])
		    -> addCase(read_bslot, bvc_null_pkt)
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
      
      //      for (unsigned int b = 0; b<depth; b++) 
      for (unsigned int b = 0; b<physicalDepth; b++) 
	{
	  // b_eq_q means: should the bth slot in circular buffer map to the qth slot in fifo queue
	  //unsigned int hpos = (depth+b-q)%physicalDepth; //slot q maps to slot b when head=hpos            
	  unsigned int hpos = (physicalDepth+b-q)%physicalDepth; //slot q maps to slot b when head=hpos            
	  Expr * q_eq_b = new Eq_Expr(head, new Bvconst_Expr(hpos,wDepth));
	  e -> addCase(q_eq_b, bslots[b]);
	}
      qslots[q]	-> setExpr( e );
      
    }
  
  ASSERT(qslots[0]->getWidth() == wPacket);
  o_data    ->setExpr( (new Id_Expr( qslots[0]))->setWidth(wPacket) );

  if (g_ckt->voptions->isEnabledPsi) 
    {
      
      Signal *headTailDistance = (new Signal())
	-> setName( name+"headTailDistance" )
	-> setExpr( BvsubExprModM( tail, head, physicalDepth-1, name+"headTailDistance") );
      
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
	  
	  //	  double maxval = pow(double(2),double(g_ckt->weClk))-1;
	  double maxval = pow(double(2),double(wTimestamp))-1;

	  Signal *agex  = (new Signal(name+"ageIfValid"))
	    -> setExpr( 
		       BvsubExprModM( 
				     g_ckt->tCurrent, 
				     qSlotTimestamp , 
				     maxval , 
				     name+"qSlot"+itos(i)+"currentMinusTimestamp" 
				      ) 
			);

	  age[i] = (new Signal())
	    -> setName(name+"qslot"+itos(i)+"_age")
	    -> setExpr( (new Case_Expr())
			-> setDefault( new Bvconst_Expr(0, wTimestamp ) )
			-> addCase( hasData, agex)
			-> setWidth(wTimestamp)
			);

	}

      if (g_ckt->voptions->isEnabledPhiLQueue) 
	{
	  for (unsigned int i = 0; i<depth; i++) 
	    if ( slotQos[i]->isEnabled() and slotQos[i]->hasMaxAge() )
	      {
	  
		Signal *age_bound = (new Signal())
		  -> setName( name+"qslot"+itos(i)+"phiLAgeBound")
		  -> setExpr( (new Bvconst_Expr(slotQos[i]->getMaxAge() + 1)) -> setWidth(wTimestamp) );
	  
		Signal *age_valid = (new Signal())
		  -> setName( name+"qslot"+itos(i)+"phiLValid" )
		  -> setExpr( new Lt_Expr( age[i] , age_bound) );
	  
		age_valid -> assertSignalTrue();
	      }
	}

      if (g_ckt->voptions->isEnabledPhiGQueue) 
	{
	  for (unsigned int i = 0; i<depth; i++) 
	    if ( slotQos[i]->isEnabled() )
	      {

		Signal *age_bound = (new Signal())
		  -> setName(name+"qslot"+itos(i)+"phiGAgebound")
		  -> setExpr( (new Bvconst_Expr( g_ckt->voptions->getTMax())) -> setWidth(wTimestamp) );

		Signal *age_valid = (new Signal())
		  -> setName( name+"qslot"+itos(i)+"phiGValid" )
		  -> setExpr( new Lt_Expr( age[i] , age_bound) );

		age_valid -> assertSignalTrue();
	      }
	}
    }  

  o->channel->data     -> setExpr( (new Id_Expr( o_data ))  );
  o->channel->irdy     -> setExpr( (new Id_Expr( o_irdy )) ->setWidth(1) );
  
  return;
}








Queue::Queue(Channel *in, Channel *out, unsigned int d, const string n, Hier_Object *p) : Primitive(n,p) {
  i = new Targ_Port("i",in, this);
  o = new Init_Port("o",out,this);

  ASSERT (d >= 1); 
  depth = d;
  numItemsMax = depth;
  
  numItems = new Seq_Signal(name+"numItems");
  numItems ->setWidth(numBitsRequired(depth));

  for (int i = 0; i<depth; i++) 
    slotQos.push_back (new Slot_Qos(i,this) );
  qslots.resize(depth);
  type = PACKET_DATA;
  (*g_network).primitives.push_back(this);
  (*g_network).queues.push_back(this);
  // try switching to one-hot encoding for numItems?

}



void Fork::propagateLatencyLemmas( ) {

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





void Fork::buildPrimitiveLogic ( ) {

  unsigned int w = portI->channel->data->getWidth();
  //  ASSERT (w == portA->channel->data->getWidth());
  ASSERT (w == portB->channel->data->getWidth());

  ASSERT2 (portA->channel->getPacketType() == PACKET_CREDIT, "must be restricted fork");
  // second input can be a second credit token or else data packet
  //ASSERT (w == a->channel->data->getWidth() );
  //  ASSERT (w == b->channel->data->getWidth() );


  Signal *i_irdy = new Signal(name+"i_irdy");
  Signal *i_data = new Signal(name+"i_data");
  Signal *i_trdy = new Signal(name+"i_trdy");
    
  i_irdy                -> setExpr( new Id_Expr( portI->channel->irdy ) );
  i_data                -> setExpr( new Id_Expr( portI->channel->data ) );
     
  Signal *a_irdy = new Signal(name+"a_irdy");
  Signal *a_data = new Signal(name+"a_data");
  Signal *a_trdy = new Signal(name+"a_trdy");

  a_trdy                 -> setExpr( new Id_Expr( portA->channel->trdy ) );

  Signal *b_irdy = new Signal(name+"b_irdy");
  Signal *b_data = new Signal(name+"b_data");
  Signal *b_trdy = new Signal(name+"b_trdy");

  b_trdy                 -> setExpr( new Id_Expr( portB->channel->trdy ) );

  // logic for fork:
  a_irdy        -> setExpr ( new And_Expr( i_irdy , b_trdy) );
  b_irdy        -> setExpr ( new And_Expr( i_irdy , a_trdy) );
  //  a_data        -> setExpr ( new Id_Expr( i_data));
  a_data        -> setExpr ( new Bvconst_Expr(1, 1));
  b_data        -> setExpr ( new Id_Expr( i_data));
  i_trdy        -> setExpr ( new And_Expr( a_trdy , b_trdy ));

  portA->channel->data   -> setExpr( new Id_Expr( a_data           ) );
  portA->channel->irdy   -> setExpr( new Id_Expr( a_irdy           ) );
  portB->channel->data   -> setExpr( new Id_Expr( b_data           ) );
  portB->channel->irdy   -> setExpr( new Id_Expr( b_irdy           ) );
  portI->channel->trdy  -> setExpr( new Id_Expr( i_trdy           ) );

  return;
}





void Switch::propagateLatencyLemmas( ) {
  ASSERT(0);
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


void Switch::buildPrimitiveLogic ( ) {

  unsigned int w = portI->channel->data->getWidth();
  ASSERT (w == portA->channel->data->getWidth());
  ASSERT (w == portB->channel->data->getWidth());

    
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








void Merge::propagateLatencyLemmas( ) {
    
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

void Merge::buildPrimitiveLogic ( ) {

  unsigned int w = o->channel->data->getWidth();
  ASSERT (w == a->channel->data->getWidth());
  ASSERT (w == b->channel->data->getWidth());
  
  Signal *a_irdy = (new Signal(name+"a_irdy" ))->setWidth(1);
  Signal *a_data = (new Signal(name+"a_data" ))->setWidth(w);
  Signal *a_trdy = (new Signal(name+"a_trdy" ))->setWidth(1);

  a_irdy             -> setExpr( new Id_Expr(1     , a->channel->irdy) );
  a_data             -> setExpr( new Id_Expr(w , a->channel->data) );
  a->channel->trdy   -> setExpr( new Id_Expr(1     , a_trdy));


  Signal *b_irdy = (new Signal(name+"b_irdy" ))->setWidth(1);
  Signal *b_data = (new Signal(name+"b_data" ))->setWidth(w);
  Signal *b_trdy = (new Signal(name+"b_trdy" ))->setWidth(1);
    
  b_irdy            -> setExpr( new Id_Expr(1     , b->channel->irdy ) );
  b_data            -> setExpr( new Id_Expr(w , b->channel->data ) );
  b->channel->trdy  -> setExpr( new Id_Expr(1     , b_trdy) );
    
  Signal *o_irdy = (new Signal(name+"o_irdy" ))->setWidth(1);
  Signal *o_data = (new Signal(name+"o_data" ))->setWidth(w);
  Signal *o_trdy = (new Signal(name+"o_trdy" ))->setWidth(1);

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
        

void Sink::propagateLatencyLemmas( ) {
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

  if (sink_type == ORACLE_EAGER) { 
    i->channel->qos->updateTargetBound(1);
    //      i->channel->updateInitiatorResponseBound(responseBound, modifiedChannels);
    //i->channel->qos->updateTargetResponseBound(responseBound, modifiedChannels);
    i->channel->qos->updateTimeToSinkBound(1);
  }


  return;
}



void Sink::buildPrimitiveLogic () {

  Signal *oracle_trdy;
  if (    (sink_type == ORACLE_BOUNDED_RESPONSE) 
	  or (sink_type == ORACLE_NONDETERMINISTIC) 
	  or (sink_type == ORACLE_BOUNDED) ) 
    {
      unsigned int lsb = g_ckt->oracleBus->getWidth();
      unsigned int msb = lsb;
      g_ckt->oracleBus->widen(1);
      oracle_trdy = (new Signal(name+"oracle_trdy"))
	-> setExpr( new Extract_Expr(g_ckt->oracleBus, msb, lsb) );
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



void Source::buildPrimitiveLogic ( ) {
    
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
      unsigned int lsb = g_ckt->oracleBus->getWidth();
      unsigned int msb = lsb;
      g_ckt->oracleBus->widen(1);
      oracleIrdy = (new Signal(name+"oracleIrdy"))
	-> setExpr( new Extract_Expr(g_ckt->oracleBus, msb, lsb) );
    }
  else  { ASSERT(0); }


  Signal *blocked = (new Signal(name+"blocked"))
    -> setExpr( new And_Expr( o->channel->irdy, new Not_Expr(o->channel->trdy) )  );

  Signal *preBlocked = (new Seq_Signal(name+"preBlocked"))
    -> setResetExpr( new Bvconst_Expr(0,1)  )
    -> setNxtExpr( blocked );



  //determine the bitwidths to use
  unsigned int wData = o->channel->data->getWidth();
  unsigned int wOracle = o->channel->getDWidth();
  if (o->channel->getPacketType() == PACKET_DATA) {
    ASSERT(wOracle + g_ckt->tCurrent->getWidth() == wData);
    unsigned int lsb = g_ckt->oracleBus->getWidth();
    unsigned msb = lsb + wOracle - 1;
    g_ckt->oracleBus->widen( wOracle );
    Signal *oracle_data = (new Signal(name+"oracle_data"))
      -> setExpr( new Extract_Expr(g_ckt->oracleBus, msb, lsb) );
    

    Expr *oracleCatClk = new Cat_Expr(g_ckt->tCurrent , oracle_data);
    Signal *pre_data = (new Seq_Signal(name+"pre_data"))
      -> setResetExpr( new Bvconst_Expr(0, wData) )
      -> setNxtExpr (oracleCatClk );
    

    o->channel->data
      -> setExpr ( (new Case_Expr())
		   -> setDefault( oracleCatClk )
		   -> addCase (preBlocked, pre_data)
		   );

  } 
  else 
    {
      o->channel->irdy
	-> setExpr( new Or_Expr( oracleIrdy, preBlocked) );

      o->channel->data
	-> setExpr ( (new Bvconst_Expr( 1,1)));

    }    


  o->channel->irdy
    -> setExpr( new Or_Expr( oracleIrdy, preBlocked) );

  return;
}

void Source::propagateLatencyLemmas( ) {

  if (source_type == ORACLE_EAGER)
    {
      o->channel->qos->updateInitiatorBound(0);
    }

  if (o->channel->qos->hasTargetResponseBound())
    {
      o->channel->qos->updateAgeBound(o->channel->qos->getTargetResponseBound() );
    }
  if (o->channel->qos->hasTargetBound())
    {
      o->channel->qos->updateAgeBound(o->channel->qos->getTargetBound());
    }    

  return;
}  




string Slot_Qos::printHeader() {
  std::stringstream out;
  out << "queue: " << setw(16) << parentQueue->name << " slot:" << setw(2) << slotIndexInParentQueue ;
  return out.str();
};

Slot_Qos::Slot_Qos(unsigned int i, Queue *q) {
  enable();
  slotIndexInParentQueue = i;
  parentQueue = q;
  timeToSink = T_PROP_NULL;
  maxAge = T_PROP_NULL;
}

void Slot_Qos::printSlotQos(ostream &f) {
  f    << printHeader() 
       << "  timeToSinkBound: " << setw(3)  << right << validTimeOrDash(getTimeToSink())
       << "  ageBound: "        << setw(3)  << right << validTimeOrDash(getMaxAge())
       << "\n"; 
}

void Slot_Qos::setTimeToSink(unsigned int t)   { 
  timeToSink = min(timeToSink,t);
  g_outQos << printHeader() << "  set timeToSink to " << maxAge << "\n";
}


void Slot_Qos::setMaxAge(unsigned int t)       { 
  ASSERT(t < T_PROP_NULL);
  maxAge = min(maxAge,t);
  g_outQos << printHeader() << "  set maxAge to " << maxAge << " arg was " << t << "\n";
}
