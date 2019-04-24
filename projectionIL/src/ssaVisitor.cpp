#include "ssa.h"

void IRNodeVisitor::visit (Identifier* id, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (id);
}

void IRNodeVisitor::visit (Array* array, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (array);
}

void IRNodeVisitor::visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (arrayIdx);
}

void IRNodeVisitor::visit (Assignment* assign, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (assign);
}

void IRNodeVisitor::visit (BackwardBranch* backBr, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (backBr);
}

void IRNodeVisitor::visit (BasicBlock* basicBlock, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (basicBlock);
}

void IRNodeVisitor::visit (Boolean* boolean, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (boolean);
}

void IRNodeVisitor::visit (Call* call, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (call);
}

void IRNodeVisitor::visit (Conditional* cond, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (cond);
}

void IRNodeVisitor::visit (ConditionalBranch* condBr, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (condBr);
}

void IRNodeVisitor::visit (Constant* constant, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (constant);
}

void IRNodeVisitor::visit (DirectBranch* directBr, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (directBr);
}

void IRNodeVisitor::visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (fieldGet);
}

void IRNodeVisitor::visit (Input* in, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (in);
}

void IRNodeVisitor::visit (Instruction* instr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (instr);
}

void IRNodeVisitor::visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (jsonKV);
}

void IRNodeVisitor::visit (JSONObject* jsonObj, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (jsonObj);
}

void IRNodeVisitor::visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (keyGet);
}

void IRNodeVisitor::visit (Let* let, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (let);
}

void IRNodeVisitor::visit (LoadPointer* ldPtr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ldPtr);
}

void IRNodeVisitor::visit (Number* num, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (num);
}

void IRNodeVisitor::visit (PHI* phi, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (phi);
}

void IRNodeVisitor::visit (Pattern* pt, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (pt);
}

void IRNodeVisitor::visit (PatternApplication* ptApp, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ptApp);
}

void IRNodeVisitor::visit (Patterns* pts, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (pts);
}

void IRNodeVisitor::visit (Pointer* ptr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ptr);
}

void IRNodeVisitor::visit (Program* prog, IRNodeVisitorArg arg) 
{
  throwInvalidVisitorForClass (prog);
}

void IRNodeVisitor::visit (Return* ret, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ret);
}

void IRNodeVisitor::visit (StorePointer* strPtr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (strPtr);
}

void IRNodeVisitor::visit (String* str, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (str);
}

void GetAllInputIdentifierVisitor::visit (Identifier* id, IRNodeVisitorArg arg)
{
  argToIds(arg)->push_back (id);
}

void GetAllInputIdentifierVisitor::visit (Array* array, IRNodeVisitorArg arg)
{
  for (auto expr : array->getExpressions ()) {
    expr->accept (this, arg);      
  }
}

void GetAllInputIdentifierVisitor::visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg)
{
  //No Identifier
}

void GetAllInputIdentifierVisitor::visit (Assignment* assign, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (assign);
}

void GetAllInputIdentifierVisitor::visit (BackwardBranch* backBr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (backBr);
}

void GetAllInputIdentifierVisitor::visit (BasicBlock* basicBlock, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (basicBlock);
}

void GetAllInputIdentifierVisitor::visit (Boolean* boolean, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (boolean);
}

void GetAllInputIdentifierVisitor::visit (Call* call, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (call);
}

void GetAllInputIdentifierVisitor::visit (Conditional* cond, IRNodeVisitorArg arg)
{
  cond->getOp1 ()->accept (this, arg);
  cond->getOp2 ()->accept (this, arg);
}

void GetAllInputIdentifierVisitor::visit (ConditionalBranch* condBr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (condBr);
}

void GetAllInputIdentifierVisitor::visit (Constant* constant, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (constant);
}

void GetAllInputIdentifierVisitor::visit (DirectBranch* directBr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (directBr);
}

void GetAllInputIdentifierVisitor::visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (fieldGet);
}

void GetAllInputIdentifierVisitor::visit (Input* in, IRNodeVisitorArg arg)
{
  visit ((Identifier*)in, arg);
}

void GetAllInputIdentifierVisitor::visit (Instruction* instr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (instr);
}

void GetAllInputIdentifierVisitor::visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg)
{
  jsonKV->getValue ()->accept (this, arg);
}

void GetAllInputIdentifierVisitor::visit (JSONObject* jsonObj, IRNodeVisitorArg arg)
{
  for (auto kvpair : jsonObj->getKeyValuePairs ()) {
    kvpair->accept (this, arg);
  }
}

void GetAllInputIdentifierVisitor::visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (keyGet);
}

void GetAllInputIdentifierVisitor::visit (Let* let, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (let);
}

void GetAllInputIdentifierVisitor::visit (LoadPointer* ldPtr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ldPtr);
}

void GetAllInputIdentifierVisitor::visit (Number* num, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (num);
}

void GetAllInputIdentifierVisitor::visit (PHI* phi, IRNodeVisitorArg arg)
{
}

void GetAllInputIdentifierVisitor::visit (Pattern* pt, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (pt); 
}

void GetAllInputIdentifierVisitor::visit (PatternApplication* patApp, IRNodeVisitorArg arg)
{
  patApp->getIdentifier ()->accept (this, arg);
}

void GetAllInputIdentifierVisitor::visit (Patterns* pts, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (pts);
}

void GetAllInputIdentifierVisitor::visit (Pointer* ptr, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (ptr);
}

void GetAllInputIdentifierVisitor::visit (Program* prog, IRNodeVisitorArg arg)
{
  throwInvalidVisitorForClass (prog);
}

void GetAllInputIdentifierVisitor::visit (StorePointer* strPtr, IRNodeVisitorArg arg)
{
  strPtr->getInputExpr ()->accept (this, arg);
}

GetAllInputIdentifierVisitor::Identifiers GetAllInputIdentifierVisitor::getAllInputIds (IRNode* start)
{
  Identifiers ids;
  IRNodeVisitorArg arg (&ids);
  start->accept (this, arg);
  
  return ids;
}

UseDef UseDefVisitor::getAllUseDef (IRNode* start)
{
  UseDef useDef;
  IRNodeVisitorArg arg (&useDef);
  
  start->accept (this, arg);
  
  return useDef;
}

void UseDefVisitor::visit (Identifier* id, IRNodeVisitorArg arg)
{
}
  
void UseDefVisitor::visit (Array* array, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Assignment* assign, IRNodeVisitorArg arg)
{
  GetAllInputIdentifierVisitor idsVisitor;
  
  std::vector<Identifier*> ids = idsVisitor.getAllInputIds(assign->getInput ());
  
  for (auto id : ids) {
    argToUseDef (arg)->addUse (id->getIDWithVersion (), assign);
  }
  
  argToUseDef (arg)->setDef (assign->getOutput ()->getIDWithVersion (), assign);
}

void UseDefVisitor::visit (BackwardBranch* backBr, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (BasicBlock* basicBlock, IRNodeVisitorArg arg)
{
  for (auto instr : basicBlock->getInstructions ()) {
    instr->accept (this, arg);
  }
}

void UseDefVisitor::visit (Boolean* boolean, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Call* call, IRNodeVisitorArg arg)
{
  GetAllInputIdentifierVisitor idsVisitor;
  
  std::vector<Identifier*> ids = idsVisitor.getAllInputIds(call->getArgument ());
  
  for (auto id : ids) {
    argToUseDef (arg)->addUse (id->getIDWithVersion (), call);
  }
  
  argToUseDef (arg)->setDef (call->getReturnValue ()->getIDWithVersion (), call);
}

void UseDefVisitor::visit (Conditional* cond, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (ConditionalBranch* condBr, IRNodeVisitorArg arg)
{
  GetAllInputIdentifierVisitor idsVisitor;
  
  std::vector<Identifier*> ids = idsVisitor.getAllInputIds(condBr->getCondition ());
  
  for (auto id : ids) {
    argToUseDef (arg)->addUse (id->getIDWithVersion (), condBr);
  }
}

void UseDefVisitor::visit (Constant* constant, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (DirectBranch* directBr, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Input* in, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Instruction* instr, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (JSONObject* jsonObj, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Let* let, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (LoadPointer* ldPtr, IRNodeVisitorArg arg)
{
  argToUseDef (arg)->setDef (ldPtr->getRetVal ()->getIDWithVersion (), ldPtr);
}

void UseDefVisitor::visit (Number* num, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (PHI* phi, IRNodeVisitorArg arg)
{
  for (auto cmdExprPair : phi->getCommandExprVector ()) {
    GetAllInputIdentifierVisitor idsVisitor;
  
    std::vector<Identifier*> ids = idsVisitor.getAllInputIds (cmdExprPair.second);
    for (auto id : ids) {
      argToUseDef (arg)->addUse (id->getIDWithVersion (), phi);
    }
  }
  
  argToUseDef (arg)->setDef (phi->getOutput ()->getIDWithVersion (), phi);
}

void UseDefVisitor::visit (Pattern* pt, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (PatternApplication* ptApp, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Patterns* pts, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Pointer* ptr, IRNodeVisitorArg arg)
{
}

void UseDefVisitor::visit (Program* prog, IRNodeVisitorArg arg)
{
  for (auto basicBlock : prog->getBasicBlocks ()) {
    basicBlock->accept (this, arg);
  }
}

void UseDefVisitor::visit (Return* ret, IRNodeVisitorArg arg)
{
  GetAllInputIdentifierVisitor idsVisitor;
  
  std::vector<Identifier*> ids = idsVisitor.getAllInputIds (ret->getReturnExpr ());
  for (auto id : ids) {
    argToUseDef (arg)->addUse (id->getIDWithVersion (), ret);
  }
}

void UseDefVisitor::visit (StorePointer* strPtr, IRNodeVisitorArg arg)
{
  
}

void UseDefVisitor::visit (String* str, IRNodeVisitorArg arg) {}

void UseDef::printDefs () 
{
  std::cout << "Defs: ---- " <<std::endl;
  for (auto iter : defMap) {
    std::cout << iter.first << " " << iter.second << std::endl;
  }
}

void UseDef::printUses ()
{
  std::cout << "Uses:-----" << std::endl;
  for (auto iter : useMap) {
    std::cout << iter.first << ": ";
    for (auto iter2 : iter.second) {
      std::cout << iter2 << " ";
    }
    std::cout << std::endl;
  }
}
