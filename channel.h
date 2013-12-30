#ifndef CHANNEL_H
#define CHANNEL_H


class Hier_Object;


/*! \brief connection between ports of two xmas components.*/
class Channel : public Hier_Object {
//    unsigned int width;
    PacketType type;
    bool specifyIrdyPersistant;
    bool specifyTrdyPersistant;
    bool specifyNonBlocking;

    bool breakCycleTrdy;
    pair <unsigned int,unsigned int> tBits; //which bits hold timestamp if type is pkt
    pair <unsigned int,unsigned int> dBits; //which bits hold data

    /* the channel builds these signals and uses them in various
       assertions etc. The reason for creating signal instead of
       expressions is to have the waveforms in debugging. */
    Signal *xfer;
    Signal *blocked;
    Signal *waiting;
    Signal *idle;

public:

    Port *initiator; // controller of irdy/data
    Port *target;    // controller of trdy
    
    // the channel doesn't build these signals, they are built by the primitives
    Signal *irdy;
    Signal *trdy;
    Signal *data;
    
    Timestamp_Signal *timestamp;
    
    Channel(string n,                 Hier_Object *p, Network *network);
    Channel(string n, unsigned int w, Hier_Object *p, Network *network); 

    void Init(string n, unsigned int w, PacketType t, Network *p);

    void widenForTimestamp( unsigned int w);

    pair <unsigned int,unsigned int> getTBits () {return tBits;}
    pair <unsigned int,unsigned int> getDBits () {return dBits;}
    unsigned int getDWidth () {return dBits.first - dBits.second + 1;}

    PacketType getPacketType() { return type; }

    void setBreakCycleTrdy(bool x);
    void setNonBlocking (bool x);
    bool getNonBlocking () {return specifyNonBlocking;};

    void setIrdyPersistant(bool x) { specifyIrdyPersistant = x; return; }
    void setTrdyPersistant(bool x) { specifyTrdyPersistant = x; return; }

    void buildChannelLogic ( );

    void buildAgeChecker(           unsigned n, string tag );
    void buildAgeChecker( Expr *en, unsigned n, string tag );
};


#endif
