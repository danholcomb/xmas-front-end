

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
#include "channel.h"
#include "expressions.h"
#include "primitives.h"

void Channel::Init(string n, unsigned int w, Hier_Object *p) {
  setPacketType( PACKET_DATA ); // by default -- overwrite by setting

  irdy = (new Signal(name+"irdy"))->setWidth(1);
  trdy = (new Signal(name+"trdy"))->setWidth(1);

  data = (new Signal(name+"data")) 
    -> setExpr((new Bvconst_Expr(1))->setWidth(w) );
    

  dBits = make_pair(w-1 , 0); //msb is first
    
  initiator = NULL;
  target = NULL;
  assertIrdyPersistant = true;
  assertTrdyPersistant = true;
  qos = new Channel_Qos(this);
  (*g_network).channels.push_back(this);
}

void Channel::widenForTimestamp( unsigned int w) {
  unsigned int wOrig = data->getWidth();
  unsigned int wNew = wOrig + w;
  tBits = make_pair(wNew-1 , wOrig); //msb is first
  data -> setWidth(wNew);
  cout << "widened " << data->getName() << " to width = " << data->getWidth() << "\n";
  return;
}

void Channel::buildChannelLogic ( ) {
    
  ASSERT(this->target!=0);    
  ASSERT(this->initiator!=0);    

  Signal *xfer = (new Signal(name+"xfer"))
    -> setExpr( (new And_Expr(irdy,trdy))  );

  Signal *blocked = (new Signal(name+"blocked"))
    -> setExpr( new And_Expr( irdy, new Not_Expr(trdy) )  );
         
  if (assertIrdyPersistant & g_ckt->voptions->isEnabledPersistance) 
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

  if (assertTrdyPersistant & g_ckt->voptions->isEnabledPersistance) 
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

  if ( qos->hasTargetResponseBound() & g_ckt->voptions->isEnabledResponseBoundChannel) 
    {
      aImpliesBoundedFutureB(irdy, trdy, qos->getTargetResponseBound(), name+"TRB");
    }

  if ( qos->hasTargetBound() & g_ckt->voptions->isEnabledBoundChannel ) 
    {
      aImpliesBoundedFutureB(bvtrue, trdy, qos->getTargetBound(), name+"TB");
    }

  if ( qos->hasInitiatorResponseBound() & g_ckt->voptions->isEnabledResponseBoundChannel) 
    {
      aImpliesBoundedFutureB(trdy, irdy, qos->getInitiatorResponseBound(), name+"IRB");
    }

  if ( qos->hasInitiatorBound() & g_ckt->voptions->isEnabledBoundChannel ) 
    {
      aImpliesBoundedFutureB(bvtrue, irdy, qos->getInitiatorBound(), name+"IB");
    }

  return;
}
// };







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
  f << setw(25) << right << "  timeToSinkBound: "         << setw(3) << right << validTimeOrDash(timeToSinkBound) ;
  f << right << "  ageBound: "                << setw(3) << right << validTimeOrDash(ageBound) ;
  f << "\n" ;
  f << setw(36) << right << "  targetBound (response): " ;  
  f << setw(3) << right << validTimeOrDash(targetBound) ;
  f << " ("     << setw(3) << right << validTimeOrDash(targetResponseBound) << ")" ;
  f << "\n";
  f << setw(36) << right << " initiatorBound (repsonse): " ;
  f << setw(3) << right << validTimeOrDash(initiatorBound) ;
  f << " ("  << setw(3) << right << validTimeOrDash(initiatorResponseBound) << ")";
  f << "\n";
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
      g_outQos << printHeader() << " updated targetResponseBound to: " << targetResponseBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateInitiatorResponseBound( unsigned int b) {
  if (b < initiatorResponseBound) 
    {
      initiatorResponseBound = b;
      g_outQos << printHeader() << " updated initiatorResponseBound to: " << initiatorResponseBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateTargetBound( unsigned int b) {
  if (b < targetBound) 
    {
      targetBound = b;
      g_outQos << printHeader() << " updated targetBound to: " << targetBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateInitiatorBound( unsigned int b) {
  if (b < initiatorBound) 
    {
      initiatorBound = b;
      g_outQos << printHeader() << " updated initiatorBound to: " << initiatorBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateTimeToSinkBound( unsigned int t) {
  if (t < timeToSinkBound) 
    {
      timeToSinkBound = t;
      g_outQos << printHeader() << " updated timeToSinkBound to: " << timeToSinkBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};

void Channel_Qos::updateAgeBound( unsigned int t) {
  if (t < ageBound) 
    {
      ageBound = t;
      g_outQos << printHeader() << " updated ageBound to: " << ageBound << "\n";
      g_network->modifiedChannels.insert(ch);
    } 
  return;
};
