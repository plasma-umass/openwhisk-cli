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

std::vector<WhiskSequence*> Converter::convert (Command* cmd)
{
  std::vector<WhiskSequence*> toRet;
  
  if (cmd->getType () == Command::SimpleCommandType) {
    WhiskSequence* seq;
    seq = new WhiskSequence ("SEQUENCE_" + gen_random_str(WHISK_SEQ_NAME_LENGTH), 
                             std::vector<WhiskAction*> (1, ((SimpleCommand*)cmd)->convert (toRet)));
    toRet.push_back (seq);
  }
  else if (cmd->getType () == Command::ComplexCommandType) {
    WhiskAction* act;
    act = ((ComplexCommand*)cmd)->convert (toRet);
  }
  else {
    fprintf (stderr, "No conversion for %d specified", cmd->getType ());
  }
  
  return toRet;
}

int main ()
{
  //test1
  {
    // X1 = A1 (input)
    // X2 = A2 (X1)
    std::cout << "Test 1" << std::endl;
    JSONIdentifier X1("X1"), X2("X2");
    Input input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&CallA2);
    ComplexCommand allCmds(v);
    std::vector<WhiskSequence*> seqs = Converter::convert (&allCmds);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl << std::endl;
    }
    seqs[0]->print ();
    std::cout << std::endl;
  }
  //test2
  {
    //X1 = A1 (input)
    //if X1 then X2_1 = A2(X1) else X2_2 = A3(X1)
    //X2 = phi(then, X2_1, else, X2_2)
    std::cout << "If then else "<< std::endl;
    JSONIdentifier X1("X1"), X2("X2"), X2_1("X2_1"), X2_2("X2_2");
    Input input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2_1, "A2", &X1);
    CallAction CallA3(&X2_2, "A3", &X1);
    IfThenElseCommand ifX1(&X1, &CallA2, &CallA3);
    std::vector<std::pair<Command*, JSONIdentifier*> > phi_v;
    phi_v.push_back(std::make_pair (ifX1.getThenBranch (), &X2_1));
    phi_v.push_back(std::make_pair (ifX1.getElseBranch (), &X2_2));
    PHINode X2Phi(phi_v);
    
    ComplexCommand phi_block;
    phi_block.appendSimpleCommand(&X2Phi);
    ifX1.getThenBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    ifX1.getElseBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&ifX1);
    
    ComplexCommand cmd1(v);
    
    std::vector<WhiskSequence*> seqs = Converter::convert (&cmd1);
    
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl << std::endl;
    }
    //seq->print ();
    std::cout << std::endl;
  }
  
  //test3
  {
    //X1 = A1 (input)
    //X1_ptr = store X1
    //while (X1) {
    //  X1 = load X1_ptr
    //  X2 = A2 (X1)
    //  store X4, X2_ptr
    //  X1 = load X2_ptr
    //}
    
    //X1 = A1 (input)
    //X1_ptr = store (X1)
    //
    //if 
    
    std::cout << "While Loop Test" << std::endl;
    JSONIdentifier X1 ("X1"), X2 ("X2"), X3("X3"), X4("X4");
    Pointer X2_ptr ("X2_ptr");
    Input input;
    
    CallAction A1 (&X1, "A1", &input);
    //~ CallAction A2 (&
  }
}
