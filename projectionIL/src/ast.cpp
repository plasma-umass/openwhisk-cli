#include "ast.h"

int CallAction::callID = 0;

void JSONIdentifier::setCallStmt(CallAction* _callStmt) 
{
  if (callStmt != nullptr) {
    fprintf (stderr, "JSONIdentifier %s already assigned to action %s, cannot be assigned to action again\n",
             identifier.c_str(), _callStmt->getActionName ().c_str());
    abort ();
  }
  
  callStmt = _callStmt;
}

std::string JSONIdentifier::convert ()
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

CallAction* Action::operator () (JSONIdentifier* out, JSONIdentifier* in)
{
  return new CallAction (out, name, in);
}

JSONPatternApplication& JSONExpression::getField (std::string fieldName)
{
  return *(new JSONPatternApplication (this, new FieldGetJSONPattern (fieldName)));
}

JSONPatternApplication& JSONExpression::operator [] (std::string key)
{
  return *(new JSONPatternApplication (this, new KeyGetJSONPattern (key)));
}

JSONPatternApplication& JSONExpression::operator [] (const char* key)
{
  return *(new JSONPatternApplication (this, new KeyGetJSONPattern (std::string(key))));
}

JSONPatternApplication& JSONExpression::operator [] (int index)
{
  return *(new JSONPatternApplication (this, new ArrayIndexJSONPattern (index)));
}

//~ JSONAssignment* JSONIdentifier::operator= (JSONExpression& exp)
//~ {
  //~ return new JSONAssignment (this, exp);
//~ }
  
//~ WhiskAction* JSONTransformation::convert(std::vector<WhiskSequence*>& basicBlockCollection)
//~ {
  //~ std::string code;
  
  //~ code = transformation->convert ();
  
  //~ return new WhiskProjection (name, code);
//~ }

//~ void JSONTransformation::print (std::ostream& os)
//~ {
  //~ out->print(os);
  //~ os << " = " ;
  //~ transformation->print (os);
  //~ os << "(";
  //~ in->print (os);
  //~ os << ")" << std::endl; 
//~ }
