#include <typeinfo>

#ifndef __SSA_VISITOR_H__
#define __SSA_VISITOR_H__

class InvalidVisitorForClass : public std::exception
{
private:
  std::string visitorName;
  std::string className;

public:
  
  InvalidVisitorForClass (std::string _visitorName, std::string _className) 
    : visitorName(_visitorName), className(_className)
  {}
  
  const char * what () const throw () {
      return (std::string("Invalid ") + className + std::string(" for ") + visitorName).c_str();
   }
};

class IRNodeVisitorArg
{
private:
  void* value;

public:
  IRNodeVisitorArg (void* v) : value(v) {}
  void setValue (void* v) {value = v;}
  void* getValue () {return value;}
};

class IRNodeVisitor
{
protected:
  void throwInvalidVisitorForClass (IRNode* _class)
  {
    //throw new InvalidVisitorForClass (typeid(*this).name(), typeid(_class).name ());
  }
  
public:
  virtual void visit (Identifier* id, IRNodeVisitorArg arg);
  virtual void visit (Array* array, IRNodeVisitorArg arg);
  virtual void visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg);
  virtual void visit (Assignment* assign, IRNodeVisitorArg arg);
  virtual void visit (BackwardBranch* backBr, IRNodeVisitorArg arg);
  virtual void visit (BasicBlock* basicBlock, IRNodeVisitorArg arg);
  virtual void visit (Boolean* boolean, IRNodeVisitorArg arg);
  virtual void visit (Call* call, IRNodeVisitorArg arg);
  virtual void visit (Conditional* cond, IRNodeVisitorArg arg);
  virtual void visit (ConditionalBranch* condBr, IRNodeVisitorArg arg);
  virtual void visit (Constant* constant, IRNodeVisitorArg arg);
  virtual void visit (DirectBranch* directBr, IRNodeVisitorArg arg);
  virtual void visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg);
  virtual void visit (Input* in, IRNodeVisitorArg arg);
  virtual void visit (Instruction* instr, IRNodeVisitorArg arg);
  virtual void visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg);
  virtual void visit (JSONObject* jsonObj, IRNodeVisitorArg arg);
  virtual void visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg);
  virtual void visit (Let* let, IRNodeVisitorArg arg);
  virtual void visit (LoadPointer* ldPtr, IRNodeVisitorArg arg);
  virtual void visit (Number* num, IRNodeVisitorArg arg);
  virtual void visit (PHI* phi, IRNodeVisitorArg arg);
  virtual void visit (Pattern* pt, IRNodeVisitorArg arg);
  virtual void visit (PatternApplication* ptApp, IRNodeVisitorArg arg);
  virtual void visit (Patterns* pts, IRNodeVisitorArg arg);
  virtual void visit (Pointer* ptr, IRNodeVisitorArg arg);
  virtual void visit (Program* prog, IRNodeVisitorArg arg);
  virtual void visit (Return* ret, IRNodeVisitorArg arg);
  virtual void visit (StorePointer* strPtr, IRNodeVisitorArg arg);
  virtual void visit (String* str, IRNodeVisitorArg arg);
};

class GetAllInputIdentifierVisitor : public IRNodeVisitor
{
private:
  typedef std::vector<Identifier*> Identifiers;
  
  Identifiers* argToIds (IRNodeVisitorArg arg)
  {
    return (Identifiers*) arg.getValue ();
  }
  
public:
  
  Identifiers getAllInputIds (IRNode* start);
  
  virtual void visit (Identifier* id, IRNodeVisitorArg arg);  
  virtual void visit (Array* array, IRNodeVisitorArg arg);
  virtual void visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg);
  virtual void visit (Assignment* assign, IRNodeVisitorArg arg) ;
  virtual void visit (BackwardBranch* backBr, IRNodeVisitorArg arg) ;
  virtual void visit (BasicBlock* basicBlock, IRNodeVisitorArg arg) ;
  virtual void visit (Boolean* boolean, IRNodeVisitorArg arg) ;
  virtual void visit (Call* call, IRNodeVisitorArg arg) ;
  virtual void visit (Conditional* cond, IRNodeVisitorArg arg) ;
  virtual void visit (ConditionalBranch* condBr, IRNodeVisitorArg arg) ;
  virtual void visit (Constant* constant, IRNodeVisitorArg arg) ;
  virtual void visit (DirectBranch* directBr, IRNodeVisitorArg arg) ;
  virtual void visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg) ;
  virtual void visit (Input* in, IRNodeVisitorArg arg) ;
  virtual void visit (Instruction* instr, IRNodeVisitorArg arg) ;
  virtual void visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg) ;
  virtual void visit (JSONObject* jsonObj, IRNodeVisitorArg arg) ;
  virtual void visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg) ;
  virtual void visit (Let* let, IRNodeVisitorArg arg) ;
  virtual void visit (LoadPointer* ldPtr, IRNodeVisitorArg arg) ;
  virtual void visit (Number* num, IRNodeVisitorArg arg) ;
  virtual void visit (PHI* phi, IRNodeVisitorArg arg) ;
  virtual void visit (Pattern* pt, IRNodeVisitorArg arg) ;
  virtual void visit (PatternApplication* ptApp, IRNodeVisitorArg arg) ;
  virtual void visit (Patterns* pts, IRNodeVisitorArg arg) ;
  virtual void visit (Pointer* ptr, IRNodeVisitorArg arg) ;
  virtual void visit (Program* prog, IRNodeVisitorArg arg) ;
  virtual void visit (Return* ret, IRNodeVisitorArg arg) 
  {std::cout <<__FILE__ << ":" << __LINE__ << ": Not Implemented" << std::endl;}
  
  virtual void visit (StorePointer* strPtr, IRNodeVisitorArg arg);
};

class UseDef
{
public:
  typedef std::unordered_map <std::string, std::unordered_set <Instruction*>> IdentifierToUseMap;
  typedef std::unordered_map <std::string, Instruction*> IdentifierToDefMap;
  
private:
  IdentifierToUseMap useMap;
  IdentifierToDefMap defMap;

public:
  UseDef () {}
  
  void setDef (std::string id, Instruction* def)
  {
    assert (defMap.count (id) == 0);
    defMap [id] = def;
  }
  
  void addUse (std::string id, Instruction* use)
  {
    assert (useMap[id].count (use) == 0);
    useMap[id].insert (use);
  }
  
  void printDefs ();
  void printUses ();
  
  IdentifierToUseMap& getUses () {return useMap;}
  IdentifierToDefMap& getDefs () {return defMap;}
};

class UseDefVisitor : public IRNodeVisitor
{
private:  
  UseDef* argToUseDef (IRNodeVisitorArg arg)
  {
    return (UseDef*) arg.getValue ();
  }
  
public:
  virtual UseDef getAllUseDef (IRNode* start);
  
  virtual void visit (Identifier* id, IRNodeVisitorArg arg);  
  virtual void visit (Array* array, IRNodeVisitorArg arg) ;
  virtual void visit (ArrayIndexPattern* arrayIdx, IRNodeVisitorArg arg) ;
  virtual void visit (Assignment* assign, IRNodeVisitorArg arg) ;
  virtual void visit (BackwardBranch* backBr, IRNodeVisitorArg arg) ;
  virtual void visit (BasicBlock* basicBlock, IRNodeVisitorArg arg) ;
  virtual void visit (Boolean* boolean, IRNodeVisitorArg arg) ;
  virtual void visit (Call* call, IRNodeVisitorArg arg) ;
  virtual void visit (Conditional* cond, IRNodeVisitorArg arg) ;
  virtual void visit (ConditionalBranch* condBr, IRNodeVisitorArg arg) ;
  virtual void visit (Constant* constant, IRNodeVisitorArg arg) ;
  virtual void visit (DirectBranch* directBr, IRNodeVisitorArg arg) ;
  virtual void visit (FieldGetPattern* fieldGet, IRNodeVisitorArg arg) ;
  virtual void visit (Input* in, IRNodeVisitorArg arg) ;
  virtual void visit (Instruction* instr, IRNodeVisitorArg arg) ;
  virtual void visit (JSONKeyValuePair* jsonKV, IRNodeVisitorArg arg) ;
  virtual void visit (JSONObject* jsonObj, IRNodeVisitorArg arg) ;
  virtual void visit (KeyGetPattern* keyGet, IRNodeVisitorArg arg) ;
  virtual void visit (Let* let, IRNodeVisitorArg arg) ;
  virtual void visit (LoadPointer* ldPtr, IRNodeVisitorArg arg) ;
  virtual void visit (Number* num, IRNodeVisitorArg arg) ;
  virtual void visit (PHI* phi, IRNodeVisitorArg arg) ;
  virtual void visit (Pattern* pt, IRNodeVisitorArg arg) ;
  virtual void visit (PatternApplication* ptApp, IRNodeVisitorArg arg) ;
  virtual void visit (Patterns* pts, IRNodeVisitorArg arg) ;
  virtual void visit (Pointer* ptr, IRNodeVisitorArg arg) ;
  virtual void visit (Program* prog, IRNodeVisitorArg arg) ;
  virtual void visit (Return* ret, IRNodeVisitorArg arg);
  virtual void visit (StorePointer* strPtr, IRNodeVisitorArg arg) ;
  virtual void visit (String* str, IRNodeVisitorArg arg);
};

#endif
