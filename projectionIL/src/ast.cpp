#include "ast.h"

int CallAction::callID = 0;

void JSONIdentifier::addCallStmt(CallAction* _callStmt) 
{
  if (callStmts.find(_callStmt) != callStmts.end()) {
    fprintf (stderr, "Identifier %s already assigned to action %s, cannot be assigned to action again\n",
             identifier.c_str(), _callStmt->getActionName ().c_str());
    abort ();
  }
  
  callStmtsToInt[(uint32_t)callStmtsToInt.size()] =_callStmt;
  callStmts.insert (_callStmt);
}

std::string JSONIdentifier::convert ()
{
  if (callStmts.size() == 0) {
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
    return ".saved.output_"+(*callStmts.begin())->getForkName ();
  
  /*else {
    std::string output_key = "output_"+_callStmt;
    std::string output = ".saved."+output_key;
    return ". * {\"saved\": { \"" + identifier +"\":"+output + "}}";
  }*/
}
