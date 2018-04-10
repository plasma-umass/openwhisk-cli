#include "ast.h"

int CallAction::callID = 0;

void JSONIdentifier::setCallStmt(CallAction* _callStmt) 
{
  if (callStmt != NULL) {
    fprintf (stderr, "Identifier %s already assigned to action %s, cannot be assigned to action %s",
             identifier.c_str(), callStmt->getActionName ().c_str(), _callStmt->getActionName ().c_str());
    abort ();
  }
  
  callStmt = _callStmt;
}

std::string JSONIdentifier::convert ()
{
  if (callStmt == nullptr) {
    fprintf (stderr, "Cannot transform identifier '%s' as no action is assigned to it\n", 
             identifier.c_str());
  }
  
  return ".saved.output_"+callStmt->getForkName ();
}
