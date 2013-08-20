

#include "main.h"
#include "expressions.h"
#include "primitives.h"
#include "channel.h"
#include "network.h"



Sink::Sink(Channel *in, string n, Hier_Object *p, Network *network) : Primitive(n,p,network)
{
    portI = new Port("portI",in,this, PORT_TARGET);
    sink_type = ORACLE_NONDETERMINISTIC;
    (*network).primitives.push_back(this);
    (*network).sinks.push_back(this);
}

Source::Source(Channel *out, string n, Hier_Object *p, Network *network) : Primitive(n,p,network)
{
    portO = new Port("portO",out,this, PORT_INITIATOR);
    source_type = ORACLE_NONDETERMINISTIC;
    (*network).primitives.push_back(this);
    (*network).sources.push_back(this);
}

// special source that can only choose from a finite set of
// values. Used in ring network to ensure that source doesn't choose
// non-existent destinations
Source::Source(Channel *out, string n, Hier_Object *p, Network *network, vector <Expr*> &dataChoices)
    : Primitive(n,p,network)
{
    portO = new Port("portO",out,this, PORT_INITIATOR);
    source_type = ORACLE_NONDETERMINISTIC;
    datas = dataChoices;
    (*network).primitives.push_back(this);
    (*network).sources.push_back(this);
}



int mpPlusInt(int a, int b) {
    unsigned s = a+b;
    unsigned x = g_ckt->tCurrentModulus;
    unsigned r = min(x , s); 
    return r;
}







bool checkQueueConstraintConsistency( vector < Queue_Constraint*> & c)
{
    vector <Queue_Constraint*> *filtered = new vector<Queue_Constraint*>;
    bool feasible = true;
    for (vector <Queue_Constraint *>::iterator it1 = c.begin(); it1 != c.end(); it1++) {
        Queue         *q1 = (*it1)->queue;
        unsigned       n1 = (*it1)->num;
        ConstraintType c1 = (*it1)->constraint;
        bool useless = false;
        for (vector <Queue_Constraint *>::iterator it2 = c.begin(); it2 != c.end(); it2++) {
            Queue         *q2 = (*it2)->queue;
            unsigned       n2 = (*it2)->num;
            ConstraintType c2 = (*it2)->constraint;
            if ((q1 == q2) and (it1 != it2)) {;
                if      ((c1 == CONSTRAINT_EQ)  and (c2 == CONSTRAINT_EQ)  and (n1 != n2)) { feasible = false; }
                else if ((c1 == CONSTRAINT_NEQ) and (c2 == CONSTRAINT_EQ)  and (n1 == n2)) { feasible = false; }
                else if ((c1 == CONSTRAINT_EQ)  and (c2 == CONSTRAINT_NEQ) and (n1 == n2)) { feasible = false; }
                
                if ((c1 == CONSTRAINT_NEQ) and (c2 == CONSTRAINT_EQ)  and (n1 != n2)) { useless = true; }
                // check order to avoid discarding both as redundant
                if ((c1 == CONSTRAINT_EQ)  and (c2 == CONSTRAINT_EQ)  and (n1 == n2) and (it1 < it2)) { useless = true; }
                if ((c1 == CONSTRAINT_NEQ) and (c2 == CONSTRAINT_NEQ) and (n1 == n2) and (it1 < it2)) { useless = true; }
                
            }
        }
        if (not useless) filtered->push_back(*it1);
        
    }
    c = *filtered;
    return feasible;
}


Delay_Eq * Delay_ITE(Queue_Constraint *i, Delay_Eq *t, Delay_Eq *e)
{

    vector < Delay_Eq_Case* > cases;

    for (vector< Delay_Eq_Case*>::iterator it = t->cases.begin(); it !=t->cases.end(); it++) { 
        vector< Queue_Constraint*> cond  ((*it)->condition); //copy 
        cond.push_back( i );
        if (checkQueueConstraintConsistency(cond))
            cases.push_back( new Delay_Eq_Case(cond, (*it)->value ));
    }

    for (vector< Delay_Eq_Case*>::iterator it = e->cases.begin(); it !=e->cases.end(); it++) { 
        vector< Queue_Constraint*> cond  ((*it)->condition); //copy 
        cond.push_back( i->invertQueueConstraint() );
        if (checkQueueConstraintConsistency(cond))
            cases.push_back( new Delay_Eq_Case(cond, (*it)->value ));
    }

    Delay_Eq *eq = new Delay_Eq(cases);
    //cout << "ITE has " << eq->cases.size() << " cases\n";
    return eq; 
}



Delay_Eq * crossProduct(Delay_Eq *x, Delay_Eq *y, LatencyOperationType op)
{
    ASSERT( x->cases.size() > 0);
    ASSERT( y->cases.size() > 0);

    vector < Delay_Eq_Case* > cases;
    for (unsigned ix =0; ix < x->cases.size(); ix++) { 
        for (unsigned iy =0; iy < y->cases.size(); iy++) { 

            unsigned aval =  x->cases[ix]->value;
            unsigned bval =  y->cases[iy]->value;

            int val;
            if        (op == LATENCY_OPERATION_MAX)      { val = min(g_ckt->tCurrentModulus,max(aval,bval)); 
            } else if (op == LATENCY_OPERATION_PLUS)     { val = mpPlusInt( aval, bval);
            } else if (op == LATENCY_OPERATION_MULTIPLY) { val = min(g_ckt->tCurrentModulus,aval*bval);
            } else { ASSERT(0);}

            vector <Queue_Constraint *> c (x->cases[ix]->condition); //copy
            c.insert (c.end(), y->cases[iy]->condition.begin(), y->cases[iy]->condition.end() );

            if (checkQueueConstraintConsistency(c))
                cases.push_back(new Delay_Eq_Case(c, val));
        }
    }
    ASSERT( cases.size() > 0);
    Delay_Eq *eq = new Delay_Eq(cases);
    return eq;
}




void Delay_Eq::buildSignals(string s)
{
    vector <Signal*> conditions;
    vector <Signal*> values;
    
    for (unsigned i =0; i < cases.size(); i++) { 
        And_Expr *e = new And_Expr();
        for (vector< Queue_Constraint*>::iterator it = cases[i]->condition.begin(); it != cases[i]->condition.end(); it++) { 
            Queue         *q = (*it)->queue;
            unsigned       n = (*it)->num;
            ConstraintType c = (*it)->constraint;
            unsigned w = q->numItems->getWidth();
            if       (c == CONSTRAINT_EQ)  { e->addInput(new  Eq_Expr(q->numItems, new Bvconst_Expr(n,w))); 
            } else if(c == CONSTRAINT_NEQ) { e->addInput(new Neq_Expr(q->numItems, new Bvconst_Expr(n,w))); 
            } else {ASSERT(0); }
        }
        Signal *cond = new Signal(s+"__cond"+itos(i), e);
        conditions.push_back( cond );
    }
    for (unsigned i =0; i < cases.size(); i++) { 
        Signal *val = new Signal(s+"__val" +itos(i), new Bvconst_Expr(cases[i]->value,g_ckt->wClk) );
        values.push_back( val );
    }

    unsigned m = 0;
    for (unsigned i =0; i < cases.size(); i++) { 
        if ((cases[i]->value > m) and (cases[i]->value < g_ckt->tCurrentModulus)) {
            m = cases[i]->value;
        }
    }
    this->intMaxFiniteBound = m;
    ASSERT(intMaxFiniteBound < g_ckt->tCurrentModulus);
    this->maxFiniteBound   = new Signal(s+"__maxFiniteBound",   new Bvconst_Expr(intMaxFiniteBound,g_ckt->wClk));

    Or_Expr *_condFiniteBound   = (new Or_Expr())->addInput(new Bvconst_Expr(0,1));
    Or_Expr *_condInfiniteBound = (new Or_Expr())->addInput(new Bvconst_Expr(0,1));
    for (unsigned i =0; i < cases.size(); i++) {        
        if (cases[i]->value == g_ckt->tCurrentModulus) {
            _condInfiniteBound ->addInput(conditions[i]);
        } else {
            _condFiniteBound ->addInput(conditions[i]);
        }
    }
    this->condFiniteBound   = new Signal(s+"__condFiniteBound",   new Id_Expr(_condFiniteBound));
    this->condInfiniteBound = new Signal(s+"__condInfiniteBound", new Id_Expr(_condInfiniteBound));

    
    Case_Expr *_currentBound = new Case_Expr(g_ckt->wClk);
    _currentBound->setDefault(new Bvconst_Expr(g_ckt->tCurrentModulus,g_ckt->wClk) );
    for (unsigned i =0; i < cases.size(); i++) { 
        _currentBound->addCase( conditions[i], values[i] ); 
    }
    this->currentBound = new Signal(s,_currentBound);
    return;
}



string Delay_Eq::sprint() {
	stringstream out;

    for (unsigned i =0; i < cases.size(); i++) { 
        out << setw(4) << i << ": " << setw(4) << cases[i]->value << "     ";
        for (vector< Queue_Constraint*>::iterator it = cases[i]->condition.begin(); it != cases[i]->condition.end(); it++) { 
            Queue         *q = (*it)->queue;
            unsigned       n = (*it)->num;
            ConstraintType c = (*it)->constraint;
            string s = (c == CONSTRAINT_EQ)? " == " : " != ";
            out << "(" << q->name << s << n << ") ";        
        }
        out << "\n";
    }

    return out.str();
}






Delay_Eq * Primitive::propagateDelayWrapper( Port *p, set <Port*> & visitedPorts)
{
    
    if ( visitedPorts.count(p) ) {
        return new Delay_Eq(g_ckt->tCurrentModulus); 
    } else if ( p->dir == PORT_TARGET and p->channel->getNonBlocking()) {
        // don't propagate through non-blocking channels since bound is 0
        cout << " targ channel: " << p->channel->name << " is non blocking \n";
        return new Delay_Eq(0);
    } else {
        set <Port*>  vp (visitedPorts); //copy constructor
        vp.insert(p);
        Delay_Eq *x = propagateDelay(p,vp);
        unsigned m = 0;

        for (unsigned i =0; i < x->cases.size(); i++) { 
            if ((x->cases[i]->value > m) and (x->cases[i]->value < g_ckt->tCurrentModulus)) {
                m = x->cases[i]->value;
            }
        }

        // cout << setw(2*visitedPorts.size()) << " "
        //      << setw(30) << left << p->getName()
        //      << "  ch:" << setw(10) << left << p->channel->name
        //      << " cases:" << setw(3) << x->cases.size() << " "
        //      << " finiteBound " << m << "\n";
        return x;
    }
}




// c: the port of this primitive that is being visited
// age: the delay to this port, according to previous port
// ageBound records the max age at each channel for packets from particular source
// delayBound is what we already know
void Primitive::propagateAgeWrapper( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound ) { 
    cout << "propagateAge at prim " << this->name << " channel " << (c)->name << " = " << age << "\n";
    ASSERT(delayBound.count(c)==1);
    ASSERT(c->getPacketType() == PACKET_DATA);
    if (ageBounds->channelBounds.count(c)) {
        cout << "channel name " << c->name << " is in circular path \n";
        return;
    } else {
        ageBounds->channelBounds[c] = age;
        propagateAge(c,age,ageBounds,delayBound);
    }

    return;
}



Delay_Eq * Join::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if (p == portA) {
        Port      *nxtPort1 = portB->channel->initiator;
        Primitive *nxtPrim1 = portB->channel->initiator->owner;
        Port      *nxtPort2 = portO->channel->target;
        Primitive *nxtPrim2 = portO->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); //recurse
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); //recurse
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX);
    } else if (p == portB) {
        Port      *nxtPort1 = portA->channel->initiator;
        Primitive *nxtPrim1 = portA->channel->initiator->owner;
        Port      *nxtPort2 = portO->channel->target;
        Primitive *nxtPrim2 = portO->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); //recurse
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); //recurse
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX);
    } else if (p == portO) {
        Port      *nxtPort1 = portA->channel->initiator;
        Primitive *nxtPrim1 = portA->channel->initiator->owner;
        Port      *nxtPort2 = portB->channel->initiator;
        Primitive *nxtPrim2 = portB->channel->initiator->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); //recurse
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); //recurse
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX);
    } 
    ASSERT(0);
}

void Join::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT (c == portB->channel);
    Primitive *nxtPrim = portO->channel->target->owner;
    Channel   *nxtChan = portO->channel;
    nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds, delayBound);
    return;
}


void Fork::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT(c == portI->channel);
    ASSERT(portB->channel->getPacketType() == PACKET_DATA);
    Primitive *nxtPrim = portB->channel->target->owner;
    Channel   *nxtChan = portB->channel;
    nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds,delayBound);
    return;
}


void Queue::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT(c == portI->channel);

    Primitive *nxtPrim = portO->channel->target->owner;
    Channel   *nxtChan = portO->channel;
    ASSERT(delayBound.count(nxtChan) == 1);
    
    int age2 = age;
    for (int i = this->depth-1; i >= 0; i--) {
        // e.g. if block bound is 3, there is xfer every 4 cycles and we can age 4 here
        age2 = mpPlusInt(age2, 1+delayBound[nxtChan]);
        ageBounds->slotBounds[make_pair(this,i)] = age2;
    }

    nxtPrim->propagateAgeWrapper(nxtChan, age2, ageBounds, delayBound);
    return;
}


void Sink::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    return;
}
    

void Source::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT (c == portO->channel);
    Primitive *nxtPrim = portO->channel->target->owner;
    Channel   *nxtChan = portO->channel;
    nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds,delayBound);
    return;
}

void Merge::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    if ((c == portA->channel) or (c == portB->channel)) {
        Primitive *nxtPrim = portO->channel->target->owner;
        Channel   *nxtChan = portO->channel;
        nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds,delayBound);
        return;
    } else {
        ASSERT(0);
    }
}

void Pri_Merge::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    if ((c == portA->channel) or (c == portB->channel)) {
        Primitive *nxtPrim = portO->channel->target->owner;
        Channel   *nxtChan = portO->channel;
        nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds,delayBound);
        return;
    } else {
        ASSERT(0);
    }
}



void Function::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT(c == portI->channel);
    Primitive *nxtPrim = portO->channel->target->owner;
    Channel   *nxtChan = portO->channel;
    nxtPrim->propagateAgeWrapper(nxtChan, age, ageBounds, delayBound);
    return;
}


void Switch::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT(c == portI->channel);

    Primitive *nxtPrimA = portA->channel->target->owner;
    Channel   *nxtChanA = portA->channel;
    nxtPrimA->propagateAgeWrapper(nxtChanA, age, ageBounds,delayBound);

    Primitive *nxtPrimB = portB->channel->target->owner;
    Channel   *nxtChanB = portB->channel;
    nxtPrimB->propagateAgeWrapper(nxtChanB, age, ageBounds,delayBound);
    return;
}

void Demux::propagateAge( Channel *c, int age, Age_Bounds_One_Source *ageBounds, map<Channel*,int> &delayBound )
{
    ASSERT(c == portI->channel);

    Primitive *nxtPrimA = portA->channel->target->owner;
    Channel   *nxtChanA = portA->channel;
    nxtPrimA->propagateAgeWrapper(nxtChanA, age, ageBounds,delayBound);

    Primitive *nxtPrimB = portB->channel->target->owner;
    Channel   *nxtChanB = portB->channel;
    nxtPrimB->propagateAgeWrapper(nxtChanB, age, ageBounds,delayBound);
    return;
}



Delay_Eq * Merge::propagateDelay( Port *p, set <Port*> & visitedPorts)
{
    if        ((p == portA) or (p == portB)){
        Port      *nxtPort = portO->channel->target;
        Primitive *nxtPrim = portO->channel->target->owner;

        // new: trying 2x+1
        Delay_Eq *x0 = nxtPrim->propagateDelayWrapper(nxtPort,visitedPorts); //recurse
        Delay_Eq *x1 = crossProduct(x0, new Delay_Eq(2), LATENCY_OPERATION_MULTIPLY); //multiply by 2
        Delay_Eq *x2 = crossProduct(x1, new Delay_Eq(1), LATENCY_OPERATION_PLUS); 
        return x2;

    } else if (p == portO) {
        Port      *nxtPort1 = portA->channel->initiator;
        Primitive *nxtPrim1 = portA->channel->initiator->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX); 
    } 
    ASSERT(0);
}

Delay_Eq * Pri_Merge::propagateDelay( Port *p, set <Port*> & visitedPorts)
{
    if        (p == portA) {
        Port      *nxtPort = portO->channel->target;
        Primitive *nxtPrim = portO->channel->target->owner;
        Delay_Eq *x = nxtPrim->propagateDelayWrapper(nxtPort,visitedPorts); //recurse
        return crossProduct(x, new Delay_Eq(2), LATENCY_OPERATION_MULTIPLY); //multiply by 2
    } else if (p == portB) {
        Port      *nxtPort = portO->channel->target;
        Primitive *nxtPrim = portO->channel->target->owner;
        Delay_Eq *x = nxtPrim->propagateDelayWrapper(nxtPort,visitedPorts); //recurse
        return crossProduct(x, new Delay_Eq(2), LATENCY_OPERATION_MULTIPLY); //multiply by 2
    } else if (p == portO) {
        Port      *nxtPort1 = portA->channel->initiator;
        Primitive *nxtPrim1 = portA->channel->initiator->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX); 
    } 
    ASSERT(0);
}

Delay_Eq * Fork::propagateDelay( Port *p, set <Port*> & visitedPorts)
{
    if        (p == portA) {
        Port      *nxtPort1 = portI->channel->initiator;
        Primitive *nxtPrim1 = portI->channel->initiator->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX); 
    } else if (p == portB) {
        Port      *nxtPort1 = portI->channel->initiator;
        Primitive *nxtPrim1 = portI->channel->initiator->owner;
        Port      *nxtPort2 = portA->channel->target;
        Primitive *nxtPrim2 = portA->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX); 
    } else if (p == portI) {
        Port      *nxtPort1 = portA->channel->target;
        Primitive *nxtPrim1 = portA->channel->target->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX); 
    } 
    ASSERT(0);
}

Delay_Eq * Switch::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if        (p == portA) {
        // can never guarantee switch because it is data dependent and
        // we don't assume anything about data during this propagation
        Delay_Eq *x = new Delay_Eq(g_ckt->tCurrentModulus); // ie infty
        return x;
    } else if (p == portB) {
        Delay_Eq *x = new Delay_Eq(g_ckt->tCurrentModulus);
        return x;
    } else if (p == portI) {
        Port      *nxtPort1 = portA->channel->target;
        Primitive *nxtPrim1 = portA->channel->target->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX);  
    }
    ASSERT(0);
}

Delay_Eq * Demux::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if        (p == portA) {
        // can never guarantee switch because it is data dependent and
        // we don't assume anything about data during this propagation
        Delay_Eq *x = new Delay_Eq(g_ckt->tCurrentModulus);
        return x;
    } else if (p == portB) {
        Delay_Eq *x = new Delay_Eq(g_ckt->tCurrentModulus);
        return x;
    } else if (p == portI) {
        Port      *nxtPort1 = portA->channel->target;
        Primitive *nxtPrim1 = portA->channel->target->owner;
        Port      *nxtPort2 = portB->channel->target;
        Primitive *nxtPrim2 = portB->channel->target->owner;
        Delay_Eq *x1 = nxtPrim1->propagateDelayWrapper(nxtPort1,visitedPorts); 
        Delay_Eq *x2 = nxtPrim2->propagateDelayWrapper(nxtPort2,visitedPorts); 
        return crossProduct(x1, x2, LATENCY_OPERATION_MAX);  
    }
    ASSERT(0);
}



Delay_Eq * Queue::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if (p == portI) {
        Port      *faninPort = portO->channel->target;
        Primitive *faninPrim = portO->channel->target->owner;
        Delay_Eq *e = faninPrim->propagateDelayWrapper(faninPort,visitedPorts); //recurse
        Delay_Eq *x = crossProduct(e, new Delay_Eq(1), LATENCY_OPERATION_PLUS); 
        Queue_Constraint *notFull = new Queue_Constraint(this, this->depth, CONSTRAINT_NEQ);
        Delay_Eq *xx = Delay_ITE(notFull, new Delay_Eq(0), x); 
        return xx;

    } else if (p == portO) {
        Port      *faninPort = portI->channel->initiator;
        Primitive *faninPrim = portI->channel->initiator->owner;
        Delay_Eq *e = faninPrim->propagateDelayWrapper(faninPort,visitedPorts); //recurse
        Delay_Eq *x = crossProduct(e, new Delay_Eq(1), LATENCY_OPERATION_PLUS); 
        Queue_Constraint *notEmpty = new Queue_Constraint(this, 0, CONSTRAINT_NEQ);
        Delay_Eq *xx = Delay_ITE(notEmpty, new Delay_Eq(0), x); 
        return xx;
    }
    ASSERT(0);
}

Delay_Eq * Function::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if (p == portI) {
        Port      *faninPort = portO->channel->target;
        Primitive *faninPrim = faninPort->owner;
        return faninPrim->propagateDelayWrapper(faninPort,visitedPorts); //recurse
    } else if (p == portO) {
        Port      *faninPort = portI->channel->initiator;
        Primitive *faninPrim = faninPort->owner;
        return faninPrim->propagateDelayWrapper(faninPort,visitedPorts); //recurse
    } 
    ASSERT(0);
}

Delay_Eq * Source::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    ASSERT (p == portO);
    if (source_type == ORACLE_EAGER) {
        return new Delay_Eq(0); 
    } else if (source_type == ORACLE_NONDETERMINISTIC) {
        return new Delay_Eq(g_ckt->tCurrentModulus);
    }
    ASSERT(0);
}

Delay_Eq * Sink::propagateDelay( Port *p, set <Port*> & visitedPorts )
{
    if (p == portI) {
        if (sink_type == ORACLE_BOUNDED_RESPONSE) {

            Port      *faninPort = portI->channel->initiator;
            Primitive *faninPrim = portI->channel->initiator->owner;
            Delay_Eq *x = faninPrim->propagateDelayWrapper(faninPort,visitedPorts); //recurse
            return crossProduct(x, new Delay_Eq(this->bound), LATENCY_OPERATION_PLUS); 

        } else if (sink_type == ORACLE_EAGER) {
            return new Delay_Eq(0);
        }
    }
    ASSERT(0);
}






// type of input a must be credit, type of input b must match output
void Join::buildPrimitiveLogic ( )
{
    cout << "starting build join " << name << "\n";
    ASSERT2 (portA->channel->getPacketType() == PACKET_TOKEN, "first join input must have type credit");
    ASSERT  (portB->channel->getPacketType()  == portO->channel->getPacketType());
    ASSERT  (portB->channel->data->getWidth() == portO->channel->data->getWidth() );

    Expr *e = new Id_Expr( portB->data); //data can be function of a and b
    portA->trdy   -> setExpr ( new And_Expr( portO->trdy , portB->irdy )  );
    portB->trdy   -> setExpr ( new And_Expr( portO->trdy , portA->irdy )  );    		    
    portO->data   -> setExpr ( new Id_Expr( e )                   );
    portO->irdy   -> setExpr ( new And_Expr( portA->irdy , portB->irdy )  ); 
    return;
}




// function is to tag the n msbs with expr(v)
Function * Function::setFunction( FunctionType f, unsigned n, unsigned v)
{
    ASSERT((f==FUNCTION_TAG) or (f==FUNCTION_ASSIGN));
    functionType = f;
    tagWidth = n;
    tagValue = v;
    return this;
}

// function is to strip the n msbs
Function * Function::setFunction( FunctionType f, unsigned n)
{
    ASSERT(f==FUNCTION_UNTAG);
    functionType = f;
    tagWidth = n;
    return this;    
}

void Function::buildPrimitiveLogic ( )
{
    cout << "starting build Function " << name << "\n";

    portI->trdy   -> setExpr ( new Id_Expr( portO->trdy));
    portO->irdy   -> setExpr ( new Id_Expr( portI->irdy));

    if (functionType == FUNCTION_ASSIGN) { 
        portO->data   -> setExpr ( new Bvconst_Expr(tagValue, tagWidth));
    } else if (functionType == FUNCTION_TAG) { // add expr as LSBs 
        portO->data   -> setExpr ( new Cat_Expr( portI->data, new Bvconst_Expr(tagValue, tagWidth) ));
    } else if (functionType == FUNCTION_UNTAG ) { // strip LSBs
        unsigned w = portI->data->getWidth();
        ASSERT(w  == tagWidth + portO->data->getWidth() );        
        portO->data   -> setExpr ( new Extract_Expr( portI->data, w-1, tagWidth ));
    } else {
        ASSERT(0);
    }
    return;
}






void Queue::buildPrimitiveLogic ( )
{
    
    //check the base size (without timestamp), and also the type
    ASSERT( portI->channel->data->getWidth() == portO->channel->data->getWidth() );
    ASSERT( getPacketType() == portI->channel->getPacketType() );
    ASSERT( getPacketType() == portO->channel->getPacketType() );
    ASSERT( depth == qslots.size() );

    // extra slot is to differentiate from 0 and depth numItems
    unsigned physicalDepth = depth+1; 
    unsigned wDepth = numItems->getWidth();
    ASSERT(wDepth >= numBitsRequired(physicalDepth)); 
    unsigned wPacket = portI->channel->data->getWidth();
    unsigned wTimestamp = g_ckt->wClk;
    
    cout << "starting to build queue " << name << "\n";
    cout << "depth = " << wDepth << "\twPacket = " << wPacket << "\twTimestamp = " << wTimestamp << "\n";

    
    Signal *nxt_numItems = new Signal(name+"nxt_numItems", wDepth);

    this->numItems
        -> setResetExpr( new Bvconst_Expr(0,wDepth) )
        -> setNxtExpr( nxt_numItems );

    
    isEmpty = new Signal(  name+"isEmpty" , new Eq_Expr( new Bvconst_Expr(0,wDepth)     , numItems ));
    isFull  = new Signal(  name+"isFull" ,  new Eq_Expr( new Bvconst_Expr(depth,wDepth) , numItems ));
    
    portI->trdy -> setExpr(new Not_Expr(isFull)); //output
    portO->irdy -> setExpr(new Not_Expr(isEmpty)); //output 

    Signal *readEnable  = new Signal(name+"readEnable",   new And_Expr(portO->irdy, portO->trdy));
    Signal *writeEnable = new Signal(name+"writeEnable",  new And_Expr(portI->irdy, portI->trdy));


    ////////////////////////////////////////////////////
    // keep track of the number of items in the queue //
    ////////////////////////////////////////////////////
    Signal *incrNumItems   = new Signal( name+"incrNumItems",   new And_Expr( writeEnable, new Not_Expr(readEnable) ) );
    Signal *decrNumItems   = new Signal( name+"decrNumItems",   new And_Expr( readEnable,  new Not_Expr(writeEnable) ) );
    Signal *numItemsMinus1 = new Signal( name+"numItemsMinus1", new Bvsub_Expr( numItems,  new Bvconst_Expr(1,wDepth) ) );
    Signal *numItemsPlus1  = new Signal( name+"numItemsPlus1" , new Bvadd_Expr( numItems,  new Bvconst_Expr(1,wDepth) ) );

    nxt_numItems-> setExpr(  (new Case_Expr(numItems->getWidth()))
                             -> setDefault(numItems)
                             -> addCase(decrNumItems, numItemsMinus1)
                             -> addCase(incrNumItems, numItemsPlus1)     );
    



    //////////////////////////////////
    // manage the tail of the queue //
    //////////////////////////////////
    Signal *nxt_posTail = new Signal(name+"nxtTail", wDepth);

    Signal *posTail = (new Seq_Signal(name+"posTail",wDepth))
        -> setResetExpr( new Bvconst_Expr(0,wDepth) )
        -> setNxtExpr (nxt_posTail);
    
    Expr *posTailPlus1 = BvIncrExprModM(posTail, physicalDepth);
    
    nxt_posTail -> setExpr( (new Case_Expr(wDepth))
                            -> setDefault(posTail)
                            -> addCase(writeEnable, posTailPlus1)  );


    //////////////////////////////////
    // manage the head of the queue //
    //////////////////////////////////
    Signal *nxt_posHead     = new Signal(name+"nxtHead",wDepth); 

    Signal *posHead = (new Seq_Signal(name+"posHead",wDepth))
        -> setResetExpr( new Bvconst_Expr(0,wDepth) )
        -> setNxtExpr ( nxt_posHead );

    // e.g for depth 2 head can be at 0,1,2
    Expr *posHeadPlus1 = BvIncrExprModM(posHead, physicalDepth);

    nxt_posHead  -> setExpr ( (new Case_Expr(wDepth))
                              -> setDefault(posHead)
                              -> addCase(readEnable, posHeadPlus1)   );


		  

    ///////////////////////////////////////////
    // manage the physical queue entries     //
    ///////////////////////////////////////////
    
    Signal *nullPkt = new Signal(name+"nullPkt", new Bvconst_Expr(0,wPacket) );
    Signal *nullSlot = new Signal(name+"nullSlot", new Cat_Expr( nullPkt, new Bvconst_Expr(0,1)));

    vector <Seq_Signal *> bslots (physicalDepth);
    for (unsigned i = 0; i<physicalDepth; i++) {
        bslots[i] = new Seq_Signal(name+"bslot"+itos(i), wPacket+1);
        
        Signal *bvi           = new Signal( name+"const"+itos(i)       ,    new Bvconst_Expr(i,wDepth));
        Signal *is_tail       = new Signal( name+"bslot"+itos(i)+"is_tail", new Eq_Expr( bvi, posTail) );
        Signal *is_head       = new Signal( name+"bslot"+itos(i)+"is_head", new Eq_Expr( bvi, posHead) );
        Signal *write_bslot   = new Signal( name+"bslot"+itos(i)+"write",   new And_Expr( writeEnable, is_tail) );
        Signal *read_bslot    = new Signal( name+"bslot"+itos(i)+"clear",   new And_Expr( readEnable,  is_head) );

        Signal *nxt_buffer    = new Signal( name+"bslot"+itos(i)+"nxt"
                                            , (new Case_Expr(wPacket+1))
                                            -> setDefault(bslots[i])
                                            -> addCase(read_bslot, nullSlot)
                                            -> addCase(write_bslot, new Cat_Expr(portI->data, new Bvconst_Expr(1,1))) );
        
        //update timestamp by extract and then recat
        if (type == PACKET_DATA) {  
            pair <unsigned,unsigned> tBits = portI->channel->getTBits(); //which bits hold timestamp
            pair <unsigned,unsigned> dBits = portI->channel->getDBits(); //which bits hold data

            Signal *_timestamp = new Signal(name+"bslot"+itos(i)+"FIELD_TIMESTAMP" ,
                                            new Extract_Expr(nxt_buffer, tBits.first+1, tBits.second+1 )); //msb..lsb
            Signal *_data      = new Signal(name+"bslot"+itos(i)+"FIELD_DATA" ,
                                            new Extract_Expr(nxt_buffer, dBits.first+1, dBits.second+1 )); //msb..lsb
            Signal *_foo       = new Signal(name+"bslot"+itos(i)+"FIELD_USED" ,
                                            new Extract_Expr(nxt_buffer, 0, 0 )); //msb..lsb

            Expr *isNullSlot = new Eq_Expr(nullSlot, nxt_buffer); //dont increment the null in empty slot
            Expr *nxt_buffer2 = (new Case_Expr(wPacket+1))
                -> setDefault ( new Cat_Expr( updatedTimestamp(_timestamp), new Cat_Expr(_data,_foo))  )
                -> addCase( isNullSlot, nullSlot ); 
            
            bslots[i]
                -> setResetExpr(nullSlot)
                -> setNxtExpr(nxt_buffer2);
            
        } else {
            ASSERT(type == PACKET_TOKEN);

            bslots[i]
                -> setResetExpr(nullSlot)
                -> setNxtExpr(nxt_buffer);
        }

    }

    // map the circular buffer slots to their fifo indexed slots.
    for (unsigned q = 0; q<depth; q++) {
        Case_Expr *e = new Case_Expr(wPacket+1);
        e -> setDefault(nullSlot);

        for (unsigned b = 0; b<physicalDepth; b++) {
            // b_eq_q means: should the bth slot in circular buffer map to the qth slot in fifo queue
            unsigned hpos = (physicalDepth+b-q)%physicalDepth; //slot q maps to slot b when head=hpos            
            Expr * q_eq_b = new Eq_Expr(posHead, new Bvconst_Expr(hpos,wDepth));
            e -> addCase(q_eq_b, bslots[b]);
        }
        Signal *ee = new Signal(name+"qslot"+itos(q)+"__ee", e);
        qslots[q]          = new Signal( name+"slot"+itos(q),            new Extract_Expr(ee, wPacket, 1)); 
        qslotIsOccupied[q] = new Signal( name+"slot"+itos(q)+"_hasdata", new Extract_Expr(ee, 0,       0));
    }
    portO->data-> setExpr( new Id_Expr( qslots[0])  );

    
    if (type == PACKET_DATA) {
        // build the ages for latency checking
        pair <unsigned,unsigned> tBits = portI->channel->getTBits(); //which bits hold timestamp
        for (unsigned i = 0; i<depth; i++) {
            Expr *_timestamp = new Extract_Expr(qslots[i], tBits.first, tBits.second );
            qslotTimestamps[i] = new Timestamp_Signal(name+"slot"+itos(i)+"Timestamp", _timestamp); 
        }
    }    


    //////////////////////////////////////////////////////////////
    // create auxiliary queue invariants to help with induction //
    //////////////////////////////////////////////////////////////
    
    // "B" slots are between head and tail iff they have data
    // "Q" slots have data iff q >= numItems 
    for (unsigned i = 0; i<physicalDepth; i++) {
        // 3 ways for slot to be between head and tail
        // case1: t < x <= h
        // case2: x <= h < t
        // case3: h < t < x
        Expr *x = new Bvconst_Expr(i,wDepth);
        Expr *case1 = new And_Expr( new Lte_Expr(posHead,x), new Lt_Expr(x,posTail));
        Expr *case2 = new And_Expr( new Lte_Expr(posHead,x), new Lt_Expr(posTail,posHead));
        Expr *case3 = new And_Expr( new  Lt_Expr(x,posTail), new Lt_Expr(posTail,posHead));
        Expr *isInRange = new Or_Expr(case1,case2,case3);
        Expr *isNullSlot = new Eq_Expr(nullSlot, bslots[i]);

        Signal *inv = (new Signal(name+"bslot"+itos(i)+"__inv", new Eq_Expr(isNullSlot, new Not_Expr(isInRange) )))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("psi");
    }
    
    for (unsigned q = 0; q<depth; q++) {
        Expr *isInRange = new Lt_Expr( new Bvconst_Expr(q,wDepth), numItems);
        Signal *inv = (new Signal(name+"qslot"+itos(q)+"__inv", new Eq_Expr( qslotIsOccupied[q], isInRange )))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("psi");
    }

    Signal *headTailDistance = new Signal(name+"headTailDistance", BvsubExprModM( posTail, posHead, depth+1) );
    Signal *headTailDistanceValid = (new Signal(name+"headTailDistanceValid", new Eq_Expr(headTailDistance, numItems)))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("psi");

    Expr *maxPos = new Bvconst_Expr(depth,wDepth);
    Signal *headRangeValid = (new Signal(name+"posHead__rangeValid", new Lte_Expr( posHead, maxPos)))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("psi");
    
    Signal *tailRangeValid = (new Signal(name+"posTail__rangeValid", new Lte_Expr( posTail, maxPos)))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("psi");
        
    cout << "done building queue " << name << "\n";
    return;
}




// check this age against all slots
void Queue::buildSlotAgeAssertions( unsigned b, string tag )
{
    for (unsigned i = 0; i<depth; i++) {
        buildSlotAgeAssertions(i, new Bvconst_Expr(1,1), b, tag);
    }
    return;
}


// check integer bound on slot
void Queue::buildSlotAgeAssertions( unsigned idx, unsigned b, string tag )
{
    buildSlotAgeAssertions(idx, new Bvconst_Expr(1,1), b, tag);
    return;
}


// with enable signals, all other versions wrap this with default args.
void Queue::buildSlotAgeAssertions( unsigned idx, Expr* en, unsigned b, string tag )
{
    if (type == PACKET_DATA) {
        ASSERT(idx < this->depth);
        ASSERT(qslotTimestamps[idx] != 0);
        ASSERT(b!=0);
        Expr *e = new And_Expr(qslotIsOccupied[idx], en);
        qslotTimestamps[idx]->assertAgeBound(e,b,tag);
    }
    return;
}




Queue::Queue(Channel *in, Channel *out, unsigned d, const string n, Hier_Object *p, Network *network)
    : Primitive(n,p,network)
{
    portI = new Port("portI",in, this, PORT_TARGET);
    portO = new Port("portO",out,this, PORT_INITIATOR);
    
    ASSERT (d >= 1); 
    depth = d;
    unsigned physicalDepth = d+1;
    numItemsMax = depth;
  
    // set signal 10 bits wide to avoid aliasing when summing in num invariants
    numItems = new Seq_Signal(name+"numItems",10);

    qslots.resize(depth);
    qslotIsOccupied.resize(depth);
    qslotTimestamps.resize(depth);

    type = PACKET_DATA;
    (*network).primitives.push_back(this);
    (*network).queues.push_back(this);

}











void Fork::buildPrimitiveLogic ( )
{
    unsigned w = portI->channel->data->getWidth();
    ASSERT (w == portB->channel->data->getWidth());
    ASSERT2 (portA->channel->getPacketType() == PACKET_TOKEN, "must be restricted fork");
    portA->irdy        -> setExpr ( new And_Expr( portI->irdy , portB->trdy) );
    portB->irdy        -> setExpr ( new And_Expr( portI->irdy , portA->trdy) );
    portA->data        -> setExpr ( new Bvconst_Expr(1, 1));
    portB->data        -> setExpr ( new Id_Expr( portI->data));
    portI->trdy        -> setExpr ( new And_Expr( portA->trdy , portB->trdy ));
    return;
}




// set the condition for routing to output A
Switch * Switch::setRoutingFunction( unsigned msb, unsigned lsb, Expr *e)
{
    this->routeBits = make_pair(msb,lsb);
    this->routeExpr = e;
    return this;
}

void Switch::buildPrimitiveLogic ( )
{
    unsigned w = portI->channel->data->getWidth();
    ASSERT (w == portA->channel->data->getWidth());
    ASSERT (w == portB->channel->data->getWidth());
    
    Signal *field = new Signal(name+"field", new Extract_Expr(portI->data, routeBits.first, routeBits.second));
    Signal *routeToA = new Signal(name+"routeToA", new And_Expr(portI->irdy, new Eq_Expr( field , routeExpr )));
    Signal *routeToB = new Signal(name+"routeToB", new And_Expr(portI->irdy, new Not_Expr( routeToA)) );

    portA->irdy        -> setExpr ( new And_Expr( routeToA , portI->irdy) );
    portB->irdy        -> setExpr ( new And_Expr( routeToB , portI->irdy) );

    portA->data        -> setExpr ( (new Case_Expr(w))
                                    -> setDefault( new Bvconst_Expr(0,w))
                                    -> addCase(routeToA, portI->data));
      
    portB->data        -> setExpr ( (new Case_Expr(w))
                                    -> setDefault( new Bvconst_Expr(0,w))
                                    -> addCase( routeToB, portI->data));

    portI->trdy        -> setExpr ( new Or_Expr( new And_Expr(routeToB, portB->trdy),
                                                 new And_Expr(routeToA, portA->trdy)  ) );
    return;
}



void Demux::buildPrimitiveLogic ( )
{
    unsigned w = portI->channel->data->getWidth();
    ASSERT (w == portA->channel->data->getWidth());
    ASSERT (w == portB->channel->data->getWidth());
    
    Signal *routeToA = new Signal(name+"routeToA", new Id_Expr(demux_ctrl)); 
    Signal *routeToB = new Signal(name+"routeToB", new Not_Expr(demux_ctrl)); 

    portA->irdy        -> setExpr ( new And_Expr( routeToA , portI->irdy) );
    portB->irdy        -> setExpr ( new And_Expr( routeToB , portI->irdy) );

    portA->data        -> setExpr ( (new Case_Expr(w))
                                    -> setDefault( new Bvconst_Expr(0,w))
                                    -> addCase(routeToA, portI->data));
      
    portB->data        -> setExpr ( (new Case_Expr(w))
                                    -> setDefault( new Bvconst_Expr(0,w))
                                    -> addCase( routeToB, portI->data));

    portI->trdy        -> setExpr ( new Or_Expr( new And_Expr(routeToB, portB->trdy),
                                                 new And_Expr(routeToA, portA->trdy)  ) );
    return;
}













void Merge::buildPrimitiveLogic ( )
{

    unsigned w = portO->channel->data->getWidth();
    ASSERT (w == portA->channel->data->getWidth());
    ASSERT (w == portB->channel->data->getWidth());
  
    Signal *u_nxt = new Signal(name+"u_nxt",1);
        
    u = (new Seq_Signal(name+"u",1))
        -> setResetExpr( new Bvconst_Expr(0,1) )
        -> setNxtExpr( u_nxt );
     
    // at = ai & ot & ( u | ~bi)
    // bt = bi & ot & (~u | ~ai)
    Signal *u_or_nbi  = new Signal(name+"u_or_nbi",  new Or_Expr( u, new Not_Expr(portB->irdy) ) );
    Signal *nu_or_nai = new Signal(name+"nu_or_nai", new Or_Expr( new Not_Expr(u), new Not_Expr(portA->irdy) ) );
    
    Signal *grantA = new Signal(name+"grantA", (new And_Expr())
                                -> addInput (u_or_nbi)
                                -> addInput (portO->trdy)
                                -> addInput (portA->irdy)     );
    
    Signal *grantB = new Signal(name+"grantB", (new And_Expr())
                                -> addInput (nu_or_nai)
                                -> addInput (portO->trdy)
                                -> addInput (portB->irdy)     );


    portA->trdy   -> setExpr( new Id_Expr(grantA) );
    portB->trdy   -> setExpr( new Id_Expr(grantB) );

    portO->irdy   -> setExpr( new Or_Expr(portA->irdy , portB->irdy ));
    portO->data   -> setExpr( (new Case_Expr(w))
                              -> setDefault( new Bvconst_Expr(0,w) )
                              -> addCase( grantB , portB->data)
                              -> addCase( grantA , portA->data)                );

    u_nxt    -> setExpr( (new Case_Expr(1))
                         -> setDefault( u )
                         -> addCase( grantB , new Bvconst_Expr(1,1) )
                         -> addCase( grantA , new Bvconst_Expr(0,1) )       );

    return;
}


void Pri_Merge::buildPrimitiveLogic ( )
{

    unsigned w = portO->channel->data->getWidth();
    ASSERT (w == portA->channel->data->getWidth());
    ASSERT (w == portB->channel->data->getWidth());
  
    u = (new Seq_Signal(name+"u",1))
        -> setResetExpr( new Bvconst_Expr(1,1) )
        -> setNxtExpr( new Bvconst_Expr(1,1) );

    // at = ai & ot & ( u | ~bi)
    // bt = bi & ot & (~u | ~ai)
    Signal *u_or_nbi  = new Signal(name+"u_or_nbi",  new Or_Expr( u, new Not_Expr(portB->irdy) ) );
    Signal *nu_or_nai = new Signal(name+"nu_or_nai", new Or_Expr( new Not_Expr(u), new Not_Expr(portA->irdy) ) );
    
    portA->trdy   -> setExpr( (new And_Expr())
                              -> addInput (u_or_nbi)
                              -> addInput (portO->trdy)
                              -> addInput (portA->irdy)     );
    
    portB->trdy   -> setExpr( (new And_Expr())
                              -> addInput (nu_or_nai)
                              -> addInput (portO->trdy)
                              -> addInput (portB->irdy)     );
		   
    portO->irdy   -> setExpr( new Or_Expr(portA->irdy , portB->irdy) );
    portO->data   -> setExpr( (new Case_Expr(w))
                              -> setDefault( new Bvconst_Expr(0,w) )
                              -> addCase( portB->trdy , portB->data)
                              -> addCase( portA->trdy , portA->data)     );
			  
    return;
}






        
        


Sink * Sink::setOracleType( OracleType x )
{
    ASSERT((x == ORACLE_DEAD) or (x == ORACLE_EAGER) or (x == ORACLE_NONDETERMINISTIC));
    sink_type = x;
    return this;
}

Sink * Sink::setOracleType( OracleType x, unsigned n )
{
    ASSERT(x == ORACLE_BOUNDED_RESPONSE);
    sink_type = x;
    bound = n;
    return this;
}

Source * Source::setOracleType( OracleType x )
{
    ASSERT((x == ORACLE_DEAD) or (x == ORACLE_EAGER) or (x == ORACLE_NONDETERMINISTIC));
    source_type = x;
    return this;
}

Source * Source::setOracleType( OracleType x, unsigned n )
{
    ASSERT(x == ORACLE_BOUNDED_RESPONSE);
    source_type = x;
    bound = n;
    return this;
}
























void Sink::buildPrimitiveLogic ()
{

    Signal *waiting = new Signal( name+"waiting", new And_Expr( portI->trdy , new Not_Expr(portI->irdy) ) );

    Signal *pre = (new Seq_Signal(name+"preWaiting",1))
        -> setResetExpr( new Bvconst_Expr(0,1) )
        -> setNxtExpr ( waiting );

    Signal *oracleTrdy;
    if ( (sink_type == ORACLE_BOUNDED_RESPONSE) or (sink_type == ORACLE_NONDETERMINISTIC)) { 
        unsigned lsb = g_ckt->oracleBus->getWidth();
        unsigned msb = lsb;
        g_ckt->oracleBus->widen(1);
        oracleTrdy = new Signal(name+"oracleTrdy", new Extract_Expr(g_ckt->oracleBus, msb, lsb));
    }
    

    // the counter to enforce bounded non-det
    if (sink_type == ORACLE_BOUNDED_RESPONSE) 
    {
        Signal *trdyUnforced = new Signal(name+"trdyUnforced", new Or_Expr(oracleTrdy, pre) );

        // this monitor computes what will happen if oracle drives trdy
        // if this oracle trdy causes monitor to fail, then the real trdy is forced

        Channel_Monitor *oracleMon = new Channel_Monitor(portI->irdy, trdyUnforced, bound, name+"oracleMon", false);

        Signal *forceTrdy = new Signal( name+"forceTrdy", BvIsZero(oracleMon->tFutureTarg));

        portI->trdy -> setExpr( new Or_Expr( trdyUnforced, forceTrdy ));

        // the real mon that sets tFutureTarg for channel. One cannot
        // use the other monitor because it does not count based on
        // i->channel->trdy. Trying to use one monitor for both causes
        // a combination loop through trdy (counter depends on trdy
        // but also determines trdy)

    }
    else if (sink_type == ORACLE_NONDETERMINISTIC) {
        ASSERT(0);
        portI->trdy -> setExpr( new Or_Expr( oracleTrdy , pre ));
    }
    else if (sink_type == ORACLE_EAGER) {
        portI->trdy -> setExpr( new Bvconst_Expr(1,1) );
    } 
    else if (sink_type == ORACLE_DEAD) {
        ASSERT(0);
        portI->trdy -> setExpr( new Bvconst_Expr(0,1) );
    } 
    else { ASSERT(0); }

    return;
}



void Source::buildPrimitiveLogic ( )
{
    cout << "building source " << name << "\n";

    // for persistance
    Signal *blocked = new Signal(  name+"blocked", new And_Expr( portO->irdy, new Not_Expr(portO->trdy) )  );
    Signal *preBlocked = (new Seq_Signal(name+"preBlocked",1))
        -> setResetExpr( new Bvconst_Expr(0,1)  )
        -> setNxtExpr( blocked );


    Signal *oracleIrdy;
    if ( (source_type == ORACLE_NONDETERMINISTIC) or (source_type == ORACLE_BOUNDED_RESPONSE) )
    {
        unsigned lsb = g_ckt->oracleBus->getWidth();
        unsigned msb = lsb;
        g_ckt->oracleBus->widen(1);
        oracleIrdy = new Signal(name+"oracleIrdy",new Extract_Expr(g_ckt->oracleBus, msb, lsb) );
    }



    
    if (source_type == ORACLE_EAGER) {
        portO->irdy -> setExpr( new Bvconst_Expr(1,1));
    } 
    else if (source_type == ORACLE_DEAD) {
        portO->irdy -> setExpr( new Bvconst_Expr(0,1));
    } 
    else if (source_type == ORACLE_NONDETERMINISTIC) {
        portO->irdy -> setExpr( new Or_Expr( oracleIrdy, preBlocked) );
    }
    else if (source_type == ORACLE_BOUNDED_RESPONSE) {

        Signal *irdyUnforced = new Signal(name+"irdyUnforced", new Or_Expr(oracleIrdy, preBlocked) );

        // this monitor computes what will happen if oracle drives trdy
        // if this oracle trdy causes monitor to fail, then the real trdy is forced
        Channel_Monitor *oracleMon = new Channel_Monitor(portO->trdy, irdyUnforced, bound, name+"oracleMon", false);
        Signal *forceIrdy = new Signal( name+"forceIrdy", BvIsZero(oracleMon->tFutureTarg));
        portO->irdy -> setExpr( new Or_Expr( irdyUnforced, forceIrdy ));

        // the real mon that sets tFutureTarg for channel. One cannot
        // use the other monitor because it does not count based on
        // i->channel->trdy. Trying to use one monitor for both causes
        // a combination loop through trdy (counter depends on trdy
        // but also determines trdy)
        Channel_Monitor *mon = new Channel_Monitor(portO->trdy, portO->irdy, bound, name+"sourceMon");

    }
    else { ASSERT(0); }



    ////////////////////////////
    // create the data signal //
    ////////////////////////////
    unsigned wOracle = portO->channel->getDWidth();
    unsigned wData   = portO->data->getWidth();
    if (portO->channel->getPacketType() == PACKET_DATA) 
    {
        Signal *oracle_data = new Signal( name+"oracle_data", wOracle); 

        if (datas.size() == 0) {
            // use pure non-deterministic assignment (ie chose any value in 2^bits)
            ASSERT(wOracle + g_ckt->wClk == wData);
            unsigned lsb = g_ckt->oracleBus->getWidth();
            unsigned msb = lsb + wOracle - 1;
            g_ckt->oracleBus->widen( wOracle );
            Expr * e = new Extract_Expr(g_ckt->oracleBus, msb, lsb);
            oracle_data -> setExpr( e );
            
        } else {
            
            // non-determinstically choose value from fixed list of choices datas
            unsigned n = numBitsRequired(datas.size());
            unsigned lsb = g_ckt->oracleBus->getWidth();
            unsigned msb = lsb + n - 1;
            g_ckt->oracleBus->widen( wOracle );
            Signal *oracle_chooser = new Signal( name+"oracle_data", new Extract_Expr(g_ckt->oracleBus, msb, lsb) );
            
            Case_Expr *e = (new Case_Expr(wOracle))
                -> setDefault( datas[0] );
            for (unsigned i = 1; i < datas.size(); i++) {
                e -> addCase( new Eq_Expr( oracle_chooser, new Bvconst_Expr(i, oracle_chooser->getWidth())), datas[i]);
            }
            oracle_data -> setExpr( e );
        }
        portO->data  -> setExpr ( new Cat_Expr( initialTimestamp() , oracle_data) );
        
    }
    else 
    {
        ASSERT(portO->channel->getPacketType() == PACKET_TOKEN); 
        portO->data  -> setExpr ( (new Bvconst_Expr(1,1) ) );
    }    


    return;
}









Merge::Merge(Channel *in1, Channel *in2, Channel *out, const string n, Hier_Object *p, Network *network) 
    : Primitive(n,p,network)
{
    portA = new Port("portA",in1, this, PORT_TARGET);
    portB = new Port("portB",in2, this, PORT_TARGET);
    portO = new Port("portO",out, this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}

Pri_Merge::Pri_Merge(Channel *in1, Channel *in2, Channel *out, const string n, Hier_Object *p, Network *network) 
    : Primitive(n,p,network)
{
    portA = new Port("portA",in1, this, PORT_TARGET);
    portB = new Port("portB",in2, this, PORT_TARGET);
    portO = new Port("portO",out, this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}


Switch::Switch(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network) 
    : Primitive(n,p,network)
{
    portI = new Port("portI",in,  this, PORT_TARGET);
    portA = new Port("portA",out1,this, PORT_INITIATOR);
    portB = new Port("portB",out2,this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}

Demux::Demux(Signal *s, Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network) 
    : Primitive(n,p,network)
{
    portI = new Port("portI",in,  this, PORT_TARGET);
    portA = new Port("portA",out1,this, PORT_INITIATOR);
    portB = new Port("portB",out2,this, PORT_INITIATOR);
    demux_ctrl = s;
    (*network).primitives.push_back(this);
}

Fork::Fork(Channel *in, Channel *out1, Channel *out2, string n, Hier_Object *p, Network *network) 
    : Primitive(n,p,network)
{
    portI = new Port("portI",in,this, PORT_TARGET);
    portA = new Port("portA",out1,this, PORT_INITIATOR);
    portB = new Port("portB",out2,this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}


Join::Join(Channel *in1, Channel *in2, Channel *out, string n, Hier_Object *p, Network *network)
    : Primitive(n,p,network)
{
    portA = new Port("portA",in1,this, PORT_TARGET);
    portB = new Port("portB",in2,this, PORT_TARGET);
    portO = new Port("portO",out,this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}

Function::Function(Channel *in, Channel *out, string n, Hier_Object *p, Network *network)
    : Primitive(n,p,network)
{
    portI = new Port("portI",  in, this, PORT_TARGET);
    portO = new Port("portO", out, this, PORT_INITIATOR);
    (*network).primitives.push_back(this);
}





















/// Note that the signals of interest are combinational. This is done
/// so that they are in sync with channel being monitored. For
/// example, isTriggered corresponds exactly to irdy

// invariants are turned off when using monitor source/sink because an
// invariant fails when compelling source/sink to act
Channel_Monitor::Channel_Monitor( Expr * trig, Expr * targ, Expr * t, string name, bool assertInvariant)
{
    Seq_Signal *tPrev = (new Seq_Signal(name+"__tPrev",t->getWidth()))
        -> setResetExpr( new Id_Expr(t) )
        -> setNxtExpr( new Id_Expr(t) );

    Signal *tIsConst = (new Signal(name+"__tIsConst", new Eq_Expr( t, tPrev)))
        -> setSignalIsAssertion(true)
        -> addAssertionClassTag("const");

    init(trig,targ,  t,  name,  assertInvariant);
    return;
}

Channel_Monitor::Channel_Monitor( Expr * trig, Expr * targ, unsigned t, string name, bool assertInvariant)
{
    unsigned w = numBitsRequired(t);
    Signal *tt = new Signal(name+"t", new Bvconst_Expr(t, w));
    init( trig, targ, tt,  name,  assertInvariant);
    return;
}

void Channel_Monitor::init( Expr * trig, Expr * targ, Expr * t, string name, bool assertInvariant)
{
    cout << "starting build Channel_Monitor " << name << "\n";
    ASSERT(trig != 0);
    ASSERT(targ != 0);
    ASSERT(trig->getWidth() == 1);
    ASSERT(targ->getWidth() == 1);

    Seq_Signal *tFutureTargValidPrev = new Seq_Signal(name+"__tFutureTargValidPrev",1);

    Expr * stayTriggered  = new And_Expr( trig, new Not_Expr(targ), tFutureTargValidPrev );
    Expr * newlyTriggered = new And_Expr( trig, new Not_Expr(targ), new Not_Expr(tFutureTargValidPrev) );
    Expr * clearTrigger   = new And_Expr( trig, targ);

    tFutureTargValid = new Signal(name+"__tFutureTargValid", new Or_Expr( stayTriggered, newlyTriggered, clearTrigger) );

    tFutureTargValidPrev
        -> setResetExpr(new Bvconst_Expr(0,1))
        -> setNxtExpr( new Or_Expr(stayTriggered, newlyTriggered) );
	     
    unsigned w = t->getWidth();
    Signal *tt = new Signal(name+"__t", new Id_Expr(t) );
    Signal *tFutureTargPrevMinus1 = new Signal(name+"__tFutureTargPrevMinus1",w);
    Seq_Signal *tFutureTargPrev = new Seq_Signal(name+"__tFutureTargPrev",w);

    // assuming that targ is persistant, we can wait in the 0 state
    Signal *waiting = new Signal(name+"__waiting", new And_Expr(targ, new Not_Expr(trig) ));

    tFutureTarg = new Signal( name+"__tFutureTarg"
                              , (new Case_Expr(w))
                              -> setDefault( tt )
                              -> addCase( newlyTriggered, tt )
                              -> addCase( stayTriggered, tFutureTargPrevMinus1 )
                              -> addCase( targ, new Bvconst_Expr(0,w) ));

    tFutureTargPrev
        -> setResetExpr(  tt  )
        -> setNxtExpr (tFutureTarg);
        
    tFutureTargPrevMinus1 -> setExpr( BvDecrExprFloor0( tFutureTargPrev ) );

    Signal *tFutureTargEqT  = new Signal(name+"__tFutureTargEqT",  new Eq_Expr( tFutureTarg , tt ));
    Signal *idle            = new Signal(name+"__idle",            new And_Expr( new Not_Expr(trig), new Not_Expr(targ) ));
    Signal *zeroImpliesTarg = new Signal(name+"__zeroImpliesTarg", new Implies_Expr( new And_Expr(tFutureTargValid, BvIsZero(tFutureTarg)), targ ));
    Signal *idleImpliesMax  = new Signal(name+"__idleImpliesMax",  new Implies_Expr( idle, tFutureTargEqT ));
    Signal *rangeValid      = new Signal(name+"__rangeValid",      new Lte_Expr( tFutureTarg, tt));
    
    // turn off invariants for the src/sink monitor since here a violation just means compelled to act
    if (assertInvariant) {

        Expr *e = (new And_Expr())
            ->addInput(zeroImpliesTarg)
            ->addInput(idleImpliesMax)
            ->addInput(rangeValid);
            
        Signal *inv  = (new Signal(name, e))
            -> setSignalIsAssertion(true)
            -> addAssertionClassTag("psil")
            -> addAssertionClassTag("psil_timing")
            -> addAssertionClassTag("cm")
            -> addAssertionClassTag("cm_inv");

    }
}



