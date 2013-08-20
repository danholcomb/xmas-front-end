


#include "main.h"
#include "channel.h"
#include "expressions.h"
#include "primitives.h"
#include "network.h"

Channel::Channel(string n,                 Hier_Object *p, Network *network) : Hier_Object(n,p,network)
{
    Init(n,1,PACKET_TOKEN,network);
} 
Channel::Channel(string n, unsigned int w, Hier_Object *p, Network *network) : Hier_Object(n,p,network)
{
    if (w == 1) {
        Init(n,1,PACKET_TOKEN,network);
    } else {
        Init(n,w,PACKET_DATA,network);
    }
}

void Channel::Init(string n, unsigned int w, PacketType t, Network *network)
{
    type = t;

    irdy = new Signal(name+"irdy",1);
    trdy = new Signal(name+"trdy",1);
    data = new Signal(name+"data",w);

    dBits = make_pair(w-1 , 0); //msb is first
    
    initiator = NULL;
    target = NULL;
    
    specifyIrdyPersistant = true;
    specifyTrdyPersistant = true;
    specifyNonBlocking = false;
    breakCycleTrdy = false;
    
    (*network).channels.push_back(this);
}

void Channel::widenForTimestamp( unsigned int w)
{
    unsigned int wOrig = data->getWidth();
    unsigned int wNew = wOrig + w;
    tBits = make_pair(wNew-1 , wOrig); //msb is first
    cout << "widening channel " << name << " from width " << wOrig << " to " << wNew << "\n";
    data -> setWidth(wNew);
    return;
}


void Channel::setBreakCycleTrdy ( bool x )
{
    breakCycleTrdy = x;
    if (x) { specifyNonBlocking = true; }
}

void Channel::setNonBlocking ( bool x )
{
    specifyNonBlocking = x;
}




void Channel::buildChannelLogic ( )
{

    cout << "building channel " << name << "\n";
    ASSERT(this->target!=0);    
    ASSERT(this->initiator!=0);    

    
    //build signals for each port (initiator and targ) normally just
    // wire together but having separate signals is needed for the
    // case of breaking combinational loops

    target->irdy = new Signal(target->getName()+"__irdy", 1);
    target->trdy = new Signal(target->getName()+"__trdy", 1);
    target->data = new Signal(target->getName()+"__data", data->getWidth() );

    initiator->irdy = new Signal(initiator->getName()+"__irdy", 1);
    initiator->trdy = new Signal(initiator->getName()+"__trdy", 1);
    initiator->data = new Signal(initiator->getName()+"__data", data->getWidth() );

    irdy -> setExpr( new Id_Expr( initiator->irdy ));
    data -> setExpr( new Id_Expr( initiator->data ));
    target->irdy -> setExpr( new Id_Expr( irdy ));
    target->data -> setExpr( new Id_Expr( data ));

    if (breakCycleTrdy) {
        // drive initiator trdy with constant 1 and check to make sure
        // that targets trdy is asserted whenever initiator tries to
        // send.
        this->trdy      -> setExpr( new Id_Expr(target->trdy) );
        initiator->trdy -> setExpr( new Bvconst_Expr(1,1) );
        assert(specifyNonBlocking);

    } else {
        this->trdy      -> setExpr( new Id_Expr( target->trdy ));
        initiator->trdy -> setExpr( new Id_Expr( this->trdy ));
    }

    // derived public signals for channel
    blocked = new Signal(name+"blocked", new And_Expr(this->irdy,               new Not_Expr(this->trdy)));
    waiting = new Signal(name+"waiting", new And_Expr(this->trdy,               new Not_Expr(this->irdy)));
    idle    = new Signal(name+"idle",    new And_Expr(new Not_Expr(this->irdy), new Not_Expr(this->trdy)));
    xfer    = new Signal(name+"xfer",    new And_Expr(this->trdy,               this->irdy ));


    if (specifyNonBlocking) {

        Signal *nb = (new Signal(name+"isNonBlocking", new Implies_Expr(this->irdy, this->trdy) ))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("nonblock")
            -> addAssertionClassTag("breakcycle")
            -> addAssertionClassTag("psi");

    }

    if (type == PACKET_DATA) {
        timestamp = new Timestamp_Signal(name+"timestamp", new Extract_Expr(data, tBits.first, tBits.second));
    } else {
        timestamp = new Timestamp_Signal(name+"timestamp", new Bvconst_Expr(0,g_ckt->wClk) );
    }

    
    
    if (specifyIrdyPersistant) { 
        Signal *preBlocked = (new Seq_Signal(name+"preBlocked",1 ))
            ->setResetExpr( new Bvconst_Expr(0,1) )
            ->setNxtExpr(this->blocked);
        
        Signal *irdyPersistant = (new Signal(name+"irdyPersistant" , new Or_Expr( irdy, new Not_Expr(preBlocked) ) ))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("psi")
            -> addAssertionClassTag("persistance");
    }

    if (specifyTrdyPersistant) { 
        Signal *preWaiting = (new Seq_Signal(name+"preWaiting",1))
            -> setResetExpr( new Bvconst_Expr(0,1) )
            -> setNxtExpr (this->waiting);
        
        Signal *trdyPersistant = (new Signal(name+"trdyPersistant", new Or_Expr( trdy, new Not_Expr(preWaiting) )))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("psi")
            -> addAssertionClassTag("persistance");
    }
    
    return;
}




void Channel::buildAgeChecker ( unsigned n, string tag )
{
    this->buildAgeChecker(new Bvconst_Expr(1,1), n, tag);
    return;
}


void Channel::buildAgeChecker ( Expr *en, unsigned n, string tag )
{
    if (type == PACKET_DATA) {
        Expr *e = new And_Expr(this->irdy, en);
        timestamp->assertAgeBound(e,n,tag);        
    } else {
        cout << "channel " << name << " is credits and has no age bounds\n";
    }
    return;
}


 

