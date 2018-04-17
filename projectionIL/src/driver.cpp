#include "driver.h"

#include <vector>
#include <string>
#include <iostream>
#include <typeinfo>

#define MAX_SEQ_NAME_SIZE 10

std::string gen_random_str(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string str = "";
    
    for (int i = 0; i < len; ++i) {
        str += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return str;
}

WhiskSequence* Converter::convert (Command* cmd)
{
  WhiskSequence* seq;
  
  if (cmd->getType () == Command::SimpleCommandType) {
    seq = new WhiskSequence ("SEQUENCE_" + gen_random_str(WHISK_SEQ_NAME_LENGTH), 
                             std::vector<WhiskAction*> (1, ((SimpleCommand*)cmd)->convert ()));
  }
  else if (cmd->getType () == Command::ComplexCommandType) {
    WhiskAction* act;
    
    act = ((ComplexCommand*)cmd)->convert ();
    seq = (WhiskSequence*)act;
  }
  else {
    fprintf (stderr, "No conversion for %d specified", cmd->getType ());
  }
  
  return seq;
}

int main ()
{
  JSONIdentifier X1("X1"), X2("X2");
  Input input;
  CallAction CallA1(&X1, "A1", &input);
  CallAction CallA2(&X2, "A2", &X1);
  std::vector<SimpleCommand*> v;
  v.push_back(&CallA1);
  v.push_back(&CallA2);
  ComplexCommand allCmds(v);
  WhiskSequence* seq = Converter::convert (&allCmds);
  seq->generateCommand (std::cout);
  //seq->print ();
}
