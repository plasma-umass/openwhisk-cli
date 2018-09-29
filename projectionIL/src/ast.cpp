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

JSONConditional& JSONExpression::operator== (std::string value)
{
  return *(new JSONConditional (this, ConditionalOperator::EQ, new StringExpression (value)));
}

JSONConditional& JSONExpression::operator== (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::EQ, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator== (int value)
{
  return *(new JSONConditional (this, ConditionalOperator::EQ, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator== (bool value)
{
  return *(new JSONConditional (this, ConditionalOperator::EQ, new BooleanExpression (value)));
}

JSONConditional& JSONExpression::operator== (JSONExpression& value)
{
  return *(new JSONConditional (this, ConditionalOperator::EQ, &value));
}

JSONConditional& JSONExpression::operator!= (std::string value)
{
  return *(new JSONConditional (this, ConditionalOperator::NE, new StringExpression (value)));
}

JSONConditional& JSONExpression::operator!= (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::NE, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator!= (int value)
{
  return *(new JSONConditional (this, ConditionalOperator::NE, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator!= (bool value)
{
  return *(new JSONConditional (this, ConditionalOperator::NE, new BooleanExpression (value)));
}

JSONConditional& JSONExpression::operator!= (JSONExpression& value)
{
  return *(new JSONConditional (this, ConditionalOperator::NE, &value));
}

JSONConditional& JSONExpression::operator>= (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::GE, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator> (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::GT, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator<= (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::LE, new NumberExpression (value)));
}

JSONConditional& JSONExpression::operator< (float value)
{
  return *(new JSONConditional (this, ConditionalOperator::LT, new NumberExpression (value)));
}

void printConditionalOperator (std::ostream& os, ConditionalOperator op)
{
  os << conditionalOpConvert (op);
}

std::string conditionalOpConvert (ConditionalOperator op)
{
  switch (op) {
    case ConditionalOperator::EQ:
      return "==";
    case ConditionalOperator::NE:
      return "!=";
    case ConditionalOperator::GT:
      return ">";
    case ConditionalOperator::GE:
      return ">=";
    case ConditionalOperator::LT:
      return "<";
    case ConditionalOperator::LE:
      return "<=";
    default:
      assert (false);
  }
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
