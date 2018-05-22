#include "ssa.h"

#include <unordered_map>

int BasicBlock::numberOfBasicBlocks = 0;
std::unordered_map <std::string, std::vector <Identifier*> > Identifier::identifiers;

void Identifier::setCallStmt(Call* _callStmt) 
{
  if (callStmt != nullptr) {
    fprintf (stderr, "Identifier '%s' already assigned to action '%s', cannot be assigned to action '%s'\n",
             identifier.c_str(), callStmt->getActionName().c_str (), _callStmt->getActionName ().c_str());
    abort ();
  }
  
  callStmt = _callStmt;
}

std::string Identifier::convert ()
{
  if (callStmt == nullptr) {
    fprintf (stderr, "Cannot transform identifier '%s' as no action is assigned to it\n", 
             identifier.c_str());
    abort ();
  }
  
  /*if (callStmts.find(_callStmt) == callStmts.end()) {
    fprintf (stderr, "Identifier %s not assigned to action %s\n", 
             identifier, _callStmt->getActionName ().c_str ());
    abort ();
  }*/
  
  //if (callStmts.size() == 1) {
    return ".saved.output_"+callStmt->getForkName ();
  
  /*else {
    std::string output_key = "output_"+_callStmt;
    std::string output = ".saved."+output_key;
    return ". * {\"saved\": { \"" + identifier +"\":"+output + "}}";
  }*/
}
