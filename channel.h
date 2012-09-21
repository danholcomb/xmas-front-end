#ifndef CHANNEL_H
#define CHANNEL_H


class Channel_Qos;

/*! \brief connection between ports of two xmas components.*/
class Channel : public Hier_Object {
  unsigned int width;
  PacketType type;
  bool assertIrdyPersistant;
  bool assertTrdyPersistant;
  pair <unsigned int,unsigned int> tBits; //which bits hold timestamp if type is pkt
  pair <unsigned int,unsigned int> dBits; //which bits hold data

public:
  Init_Port *initiator; // (who controls the irdy/data)
  Targ_Port *target; // (who controls the trdy)
    
  Signal *irdy;
  Signal *trdy;
  Signal *data;

  Channel_Qos *qos;
  
  Channel(string n,                 Hier_Object *p) : Hier_Object(n,p) { Init(n,1,p);  } 
  Channel(string n, unsigned int w, Hier_Object *p) : Hier_Object(n,p) { Init(n,w,p);  }

  void Init(string n, unsigned int w, Hier_Object *p);

  void widenForTimestamp( unsigned int w);

  pair <unsigned int,unsigned int> getTBits () {return tBits;}
  pair <unsigned int,unsigned int> getDBits () {return dBits;}
  unsigned int getDWidth () {return dBits.first - dBits.second + 1;}

  void setPacketType(PacketType t) { type = t; return; }
  PacketType getPacketType() { return type; }

 
  void setIrdyPersistant() { assertIrdyPersistant = true; return; }
  void setTrdyPersistant() { assertTrdyPersistant = true; return; }
  void setPersistant() { 
    assertIrdyPersistant = true; 
    assertTrdyPersistant = true;
    return; 
  }
  void unsetPersistant() {
    assertIrdyPersistant = false;
    assertTrdyPersistant = false;
    return;
  }

  void buildChannelLogic ( );
};






class Channel_Qos {
  // unconditionally asserted within time bound (stronger than response)
  unsigned int targetBound;
  unsigned int initiatorBound;

  unsigned int targetResponseBound;
  unsigned int initiatorResponseBound;

  unsigned int timeToSinkBound;
  unsigned int ageBound;
  Channel *ch;

  string printHeader();

 public:
  Channel_Qos(Channel *c);

  bool hasTargetResponseBound ();
  bool hasTargetBound ();
  bool hasInitiatorResponseBound ();
  bool hasInitiatorBound ();
  bool hasTimeToSinkBound ();
  bool hasAgeBound ();

  // if modifiedChannels bound is lower than current bound, add self to modified set
  void updateTargetResponseBound( unsigned int b);
  void updateInitiatorResponseBound( unsigned int b);
  void updateTargetBound( unsigned int b);
  void updateInitiatorBound( unsigned int b);
  void updateTimeToSinkBound( unsigned int t);
  void updateAgeBound( unsigned int t);

  unsigned int getTargetResponseBound ()    ;
  unsigned int getTargetBound ()            ;
  unsigned int getInitiatorResponseBound () ;
  unsigned int getInitiatorBound ()         ;
  unsigned int getTimeToSinkBound ()        ;
  unsigned int getAgeBound ()               ;

  void printChannelQos (ostream &f);

};





#endif
