#include "driver.h"
#include "ssa.h"

#include <vector>
#include <string>
#include <iostream>
#include <typeinfo>
#include <set>
#include <utility>
#include <queue>
#include <algorithm>

#define MAX_SEQ_NAME_SIZE 10
#define READ_VERSION -1
#define WRITE_VERSION -2

#if (READ_VERSION == WRITE_VERSION)
  #error "READ_VERSION and WRITE_VERSION are same."
#endif

typedef std::unordered_map <std::string, int> VersionMap;
typedef std::unordered_map <BasicBlock*, std::unordered_map <std::string, int> > BasicBlockVersionMap;
typedef std::unordered_map <Identifier*, std::vector<std::pair <BasicBlock*, Identifier*>>> PHINodePair;

int getProjectionTempFile (char* file, size_t size)
{
  char temp[] = "/tmp/wsk-proj-XXXXXX";
  if (mkstemp(&temp[0]) == -1) {
    fprintf (stderr, "Cannot create temporary file '%s'", temp);
    abort ();
    return -1;
  }
  
  if (size < strlen (temp)) {
    return -1;
  }
  
  strcpy (file, temp);
  return 0;
}

std::string gen_random_str(const int len)
{
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
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

static std::unordered_map <std::string, Identifier*> identifiers; // <id>_<version> to Identifier*
Identifier* createNewIdentifier (std::string id, int version)
{
  std::string to_search = id+"_"+std::to_string(version);
  if (identifiers.find (to_search) == identifiers.end ()) {
    identifiers[to_search] = new Identifier(id, version);
  }
  
  return identifiers[to_search];
}

int latestVersion (std::string id, VersionMap& versionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  }
  
  return versionMap[id];
}

Identifier* identifierForVersion (std::string id, VersionMap& versionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  }
  
  return createNewIdentifier (id, versionMap[id]);
}

int updateVersionNumber (std::string id, VersionMap& versionMap, 
                         VersionMap& bbVersionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  } else {
    versionMap [id] += 1;
  }
  
  bbVersionMap[id] = versionMap[id];
  
  return versionMap[id];
}

void allPredsDefiningID (BasicBlock* currBasicBlock, Identifier* id,
                         BasicBlockVersionMap& bbVersionMap, 
                         PHINodePair& phiPair, 
                         std::unordered_set<BasicBlock*>& visited)
{
  if (visited.count (currBasicBlock) != 0) {
    return;
  }
  
  auto preds = currBasicBlock->getPredecessors();
  //std::cout << "preds.size () " << currBasicBlock->getBasicBlockName () <<  " " <<  preds.size () << std::endl;
  visited.insert (currBasicBlock);
  for (auto pred : preds) {
    if (visited.count (pred) == 0 and 
        bbVersionMap[pred].find (id->getID()) != bbVersionMap[pred].end ()) {
      Identifier* newID = identifierForVersion (id->getID(), bbVersionMap[pred]);
      phiPair[id].push_back (std::make_pair (pred, newID));
    } else {
      allPredsDefiningID (pred, id, bbVersionMap, phiPair, visited);      
    }
  }
}

void updateVersionNumberInSSA (IRNode* irNode, BasicBlock* basicBlock, 
                               VersionMap& idVersions, 
                               BasicBlockVersionMap& bbVersionMap,
                               PHINodePair& phiNodePair)
{
  if (dynamic_cast <Identifier*> (irNode) != nullptr) {
    Identifier* id;
    PHINodePair phiNodePairForID;
    std::unordered_set <BasicBlock*> visited;
    id = dynamic_cast <Identifier*> (irNode);
    allPredsDefiningID (basicBlock, id, bbVersionMap, phiNodePairForID,
                        visited);
    
    if (phiNodePairForID.size () > 0) {
      for (auto iter : phiNodePairForID) {
        if (iter.second.size () >= 2) {
          updateVersionNumber (iter.first->getID (), idVersions, 
                              bbVersionMap[basicBlock]);
          phiNodePair[iter.first] = iter.second; 
        }
      }
    }
    
    id->setVersion (latestVersion (id->getID (), idVersions));
  }
}

void updateVersionNumberInBB (BasicBlock* basicBlock, 
                              VersionMap& idVersions, 
                              BasicBlockVersionMap& bbVersionMap)
{
  std::queue <BasicBlock*> basicBlockQueue;
  std::unordered_set <BasicBlock*> visited;
  
  basicBlockQueue.push (basicBlock);
  while (basicBlockQueue.empty () == false) {
    std::vector<Instruction*> instsToPrepend;
    basicBlock = basicBlockQueue.front ();
    basicBlockQueue.pop ();
    
    if (visited.count (basicBlock) == 1) {
      continue;
    }
    
    visited.insert (basicBlock);
    for (auto instr : basicBlock->getInstructions ()) {
      PHINodePair phiNodePair;
      if (dynamic_cast <Call*> (instr) != nullptr) {
        Call* call;
        Identifier* arg;
        Identifier* retVal;
        
        call = dynamic_cast <Call*> (instr);
        arg = dynamic_cast<Identifier*> (call->getArgument ());
        retVal = call->getReturnValue ();
        updateVersionNumberInSSA (call->getArgument (), basicBlock, 
                                  idVersions, bbVersionMap, phiNodePair);
        if (phiNodePair.size () > 0) {
          for (auto iter : phiNodePair) {
            if (iter.second.size () > 1) {
              instsToPrepend.push_back (new PHI (iter.first, iter.second));
            }
          }
        }
        
        retVal->setVersion (updateVersionNumber (retVal->getID (), 
                                                 idVersions, 
                                                 bbVersionMap[basicBlock]));
      } else if (dynamic_cast <ConditionalBranch*> (instr) != nullptr) {
        ConditionalBranch* condBr;
        
        condBr = dynamic_cast <ConditionalBranch*> (instr);
        updateVersionNumberInSSA (condBr->getCondition (), basicBlock,
                                  idVersions, bbVersionMap, phiNodePair);
        if (phiNodePair.size () > 0) {
          for (auto iter : phiNodePair) {
            if (iter.second.size () > 1) {
              instsToPrepend.push_back (new PHI (iter.first, iter.second));
            }
          }
        }

        basicBlockQueue.push (condBr->getThenBranch ());
        basicBlockQueue.push (condBr->getElseBranch ());
        
        //assert (*condBr->getThenBranch ()->getSuccessors().begin () ==
        //        *condBr->getElseBranch ()->getSuccessors().begin ());
        //~ updateVersionNumberInBB (*condBr->getThenBranch ()->getSuccessors().begin (),
                                 //~ idVersions, bbVersionMap);
      } else if (dynamic_cast <DirectBranch*> (instr) != nullptr) {
        DirectBranch* branch;
        
        branch = dynamic_cast <DirectBranch*> (instr);
        basicBlockQueue.push (branch->getTarget ());
      }
    }
    
    for (auto instr: instsToPrepend) {
      basicBlock->prependInstruction (instr);
    }
  }
}

IRNode* convertToSSAIR (ASTNode* astNode, BasicBlock* currBasicBlock,
                        VersionMap& idVersions, 
                        BasicBlockVersionMap& bbVersionMap);
                        
//Takes a JSONExpression Exp, adds a statement like temp = Exp, and
//returns temp
Identifier* convertInputToSSAIR (JSONExpression* astNode, BasicBlock* currBasicBlock,
                                 VersionMap& idVersions, 
                                 BasicBlockVersionMap& bbVersionMap)
{
  if (dynamic_cast <JSONIdentifier*> (astNode) != nullptr) {
    IRNode* to_ret = convertInputToSSAIR (astNode, currBasicBlock, idVersions, 
                                          bbVersionMap);
    assert (dynamic_cast<Identifier*> (to_ret) != nullptr);
    return (Identifier*)to_ret;
  }
  
  IRNode* exp = convertToSSAIR (astNode, currBasicBlock, idVersions, bbVersionMap);
  assert (dynamic_cast<Expression*> (exp) != nullptr);
  static int tempVersion = 0;
  Identifier* out = new Identifier ("temp"+std::to_string(tempVersion++), 0, nullptr);
  currBasicBlock->appendInstruction (new Assignment (out, dynamic_cast<Expression*> (exp)));
  
  return out;
}

IRNode* convertToSSAIR (ASTNode* astNode, BasicBlock* currBasicBlock,
                        VersionMap& idVersions, 
                        BasicBlockVersionMap& bbVersionMap)
{
  if (dynamic_cast <JSONIdentifier*> (astNode) != nullptr) {
    JSONIdentifier* jsonId;
    
    jsonId = dynamic_cast <JSONIdentifier*> (astNode);
    if (true or idVersions.count (jsonId->getIdentifier ()) == 0) {
      //return createNewIdentifier (identifierForVersion (jsonId->getIdentifier (), idVersions));
      auto to_ret = new Identifier (jsonId->getIdentifier ());
      currBasicBlock->insertRead (to_ret);
      return to_ret;
    } else {
      //~ PHINodePair phiPair;
      //~ std::string id = jsonId->getIdentifier ();
      //~ allPredsDefiningID (currBasicBlock, createNewIdentifier(id), bbVersionMap, phiPair);
      //~ if (phiPair.size () > 0) {
        //~ id = updateVersionNumber (id, idVersions, bbVersionMap[currBasicBlock]);
        //~ Identifier* newId = createNewIdentifier (id);
        //~ std::cout << "newInputID " << newId << std::endl;
        //~ for (auto iter : phiPair) {
          //~ std::cout << "iter.first = " << iter.first << std::endl;
          //~ currBasicBlock->appendInstruction (new PHI (newId, iter.second));
        //~ }
        
        //~ return newId;
        //~ for (auto iter : phiPair) {
          //~ std::cout << iter.first << std::endl;
          //~ for (auto vecIter : iter.second) {
            //~ std::cout << vecIter.first << " " << vecIter.second->getID () << std::endl;
          //~ }
        //~ }
      //~ } else {
        //~ return createNewIdentifier (identifierForVersion (jsonId->getIdentifier (), idVersions));
      //~ }
    }
  } else if (dynamic_cast <ReturnJSON*> (astNode) != nullptr) {
    ReturnJSON* retJson;
    
    retJson = dynamic_cast <ReturnJSON*> (astNode);
    abort (); //return new Return ((Expression*)convertToSSAIR (retJson->getReturnExpr ()));
  } else if (dynamic_cast <CallAction*> (astNode) != nullptr) {
    CallAction* callAction;
    std::string newOutputID;
    Identifier* newOutput;
    Identifier* newInput;
    std::string newInputID;
    std::string inputID;
    PHINodePair phiPair;
    JSONIdentifier* input;
    
    callAction = dynamic_cast <CallAction*> (astNode);
    input = ((JSONIdentifier*)callAction->getArgument ());

    if (dynamic_cast <JSONInput*> (input) != nullptr) {
      //There can only be one Input
      newInput = new Input ();
    } else {
      inputID = input->getIdentifier ();
    
      //~ allPredsDefiningID (currBasicBlock, createNewIdentifier(inputID), bbVersionMap, phiPair);
      if (false and phiPair.size () > 0) {
        //~ newInputID = updateVersionNumber (inputID, idVersions, bbVersionMap[currBasicBlock]);
        //~ newInput = createNewIdentifier (newInputID);
        //~ std::cout << "newInputID " << newInputID << std::endl;
        //~ for (auto iter : phiPair) {
          //~ std::cout << "iter.first = " << iter.first << std::endl;
          //~ currBasicBlock->appendInstruction (new PHI (newInput, iter.second));
        //~ }
        
        //~ for (auto iter : phiPair) {
          //~ std::cout << iter.first << std::endl;
          //~ for (auto vecIter : iter.second) {
            //~ std::cout << vecIter.first << " " << vecIter.second->getID () << std::endl;
          //~ }
        //~ }
      } else {
        //newInputID = identifierForVersion (inputID, idVersions);
        newInput = (Identifier*)convertToSSAIR (input, currBasicBlock, idVersions, 
                                                bbVersionMap);
      }
    }
    
    newOutput = new Identifier (callAction->getReturnValue()->getIdentifier ());
    currBasicBlock->insertWrite (newOutput);
    //~ updateVersionNumber (,
                                       //~ idVersions, 
                                       //~ bbVersionMap[currBasicBlock]);
    
    //std::cout << newOutputID << " " <<  callAction->getActionName () << " " << newInputID << std::endl;
    return new Call (newOutput, callAction->getActionName (),
                     newInput);
                     //TODO: (Expression*)convertToSSAIR (callAction->getArgument ()));
  } else if (dynamic_cast <JSONTransformation*> (astNode) != nullptr) {
    JSONTransformation* trans;
    
    trans = dynamic_cast <JSONTransformation*> (astNode);
    return new Transformation ((Identifier*) convertToSSAIR (trans->getOutput (), currBasicBlock, idVersions, bbVersionMap),
                               (Identifier*) convertToSSAIR (trans->getInput (), currBasicBlock, idVersions, bbVersionMap),
                               (Expression*) convertToSSAIR (trans->getTransformation (), currBasicBlock, idVersions, bbVersionMap));
  } else if (dynamic_cast <LetCommand*> (astNode) != nullptr) {
    abort ();
  } else if (dynamic_cast <NumberExpression*> (astNode) != nullptr) {
      return new Number ((dynamic_cast <NumberExpression*> (astNode))->getNumber ());
  } else if (dynamic_cast <StringExpression*> (astNode) != nullptr) {
    return new String ((dynamic_cast <StringExpression*> (astNode))->getString ());
  } else if (dynamic_cast <BooleanExpression*> (astNode) != nullptr) {
    return new Boolean ((dynamic_cast <BooleanExpression*> (astNode))->getBoolean ());
  } else if (dynamic_cast <JSONArrayExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <KeyValuePair*> (astNode) != nullptr) {
    KeyValuePair* kv = dynamic_cast <KeyValuePair*> (astNode);
    //TODO: call convertInputToSSAIR?
    IRNode* node = convertToSSAIR (kv->getValue (), currBasicBlock, idVersions, bbVersionMap);
    assert (dynamic_cast <JSONKeyValuePair*> (node) != nullptr);
    return new JSONKeyValuePair (kv->getKey (), (Expression*)node);
  } else if (dynamic_cast <JSONObjectExpression*> (astNode) != nullptr) {
    std::vector<JSONKeyValuePair*> jsonkvpairs;
    JSONObjectExpression* e = (JSONObjectExpression*) (astNode);
    for (KeyValuePair* kv : e->getKVPairs ()) {
      //TODO: call convertInputToSSAIR?
      IRNode* node = convertToSSAIR (kv, currBasicBlock, idVersions, bbVersionMap);
      assert (dynamic_cast <JSONKeyValuePair*> (node) != nullptr);
      jsonkvpairs.push_back ((JSONKeyValuePair*)node);
    }
    
    return new JSONObject (jsonkvpairs);
  } else if (dynamic_cast <JSONInput*> (astNode) != nullptr) {
    return new Input ();
  } else if (dynamic_cast <JSONPatternApplication*> (astNode) != nullptr) {
    JSONPatternApplication* patapp = (JSONPatternApplication*) astNode;
    //TODO: call convertInputToSSAIR?
    IRNode* exp = convertToSSAIR (patapp->getExpression (), currBasicBlock, idVersions, bbVersionMap);
    IRNode* pat = convertToSSAIR (patapp->getPattern (), currBasicBlock, idVersions, bbVersionMap);
    assert (dynamic_cast <Pattern*> (pat) != nullptr);
    assert (dynamic_cast <Identifier*> (exp) != nullptr);
    return new PatternApplication ((Identifier*)exp, (Pattern*)pat);
  } else if (dynamic_cast <FieldGetJSONPattern*> (astNode) != nullptr) {
    FieldGetJSONPattern* pat = (FieldGetJSONPattern*)astNode;
    return new FieldGetPattern (pat->getFieldName ());
  } else if (dynamic_cast <ArrayIndexJSONPattern*> (astNode) != nullptr) {
    ArrayIndexJSONPattern* pat = (ArrayIndexJSONPattern*) astNode;
    return new ArrayIndexPattern (pat->getIndex ());
  } else if (dynamic_cast <KeyGetJSONPattern*> (astNode) != nullptr) {
    KeyGetJSONPattern* pat = (KeyGetJSONPattern*) astNode;
    return new KeyGetPattern (pat->getKeyName ());
  } else {
    fprintf (stderr, "Invalid AstNode type\n");
    abort ();
  }
  
  abort ();
}

//~ void insertStoresInPredsForLoads (BasicBlock* basicBlock)
//~ {
  //~ for (auto pred : basicBlock->getPredecessors ()) {
    //~ for (auto 
  //~ }
//~ }

BasicBlock* convertToBasicBlock (ComplexCommand* complexCmd, 
                                 std::vector<BasicBlock*>& basicBlocks, 
                                 VersionMap& idVersions,
                                 BasicBlockVersionMap& bbVersionMap,
                                 BasicBlock** exitBlock)
{
  //This function takes one ComplexCommand containing all other
  //simple commands.
  
  //First convert ComplexCommand to a set of Basic Blocks connected
  //by branches. Also inserts Phi Branches. 
  //Algorithm to insert Phi Branches:
  //1. Every block stores what are the identifiers it has and their
  // latest version number
  //2. Get the Identifier for which to look for previous accesses.
  //3. Go through all the previous basic blocks of current basic block.
  //4. If a basic block along one path has that identifier then return its
  //   latest version number.
  //Above algorithm is naive and may lead to O(N^2) complexity. We 
  //can use dominators based minimal SSA.
  
  BasicBlock* firstBasicBlock = new BasicBlock ();
  BasicBlock* currBasicBlock = firstBasicBlock;
  basicBlocks.push_back (firstBasicBlock);
  
  for (auto cmd : complexCmd->getSimpleCommands()) {    
    if (dynamic_cast <IfThenElseCommand*> (cmd) != nullptr) {
      BasicBlock* thenBasicBlock;
      BasicBlock* elseBasicBlock;
      IfThenElseCommand* ifThenElsecmd;
      ConditionalBranch *condBr;
      Expression* cond;
      BasicBlock* target;
      PHINodePair phiPair;
      
      ifThenElsecmd = dynamic_cast <IfThenElseCommand*> (cmd);
      thenBasicBlock = convertToBasicBlock (ifThenElsecmd->getThenBranch (), 
                                            basicBlocks, idVersions, 
                                            bbVersionMap, nullptr);
      elseBasicBlock = convertToBasicBlock (ifThenElsecmd->getElseBranch (), 
                                            basicBlocks, idVersions, 
                                            bbVersionMap, nullptr);
      cond = (Expression*) convertToSSAIR (ifThenElsecmd->getCondition (), 
                                           currBasicBlock, idVersions, 
                                           bbVersionMap);
      condBr = new ConditionalBranch (cond, thenBasicBlock, 
                                      elseBasicBlock, currBasicBlock);
      currBasicBlock->appendInstruction (condBr);
      target = new BasicBlock ();
      thenBasicBlock->appendInstruction (new DirectBranch (target, thenBasicBlock));
      elseBasicBlock->appendInstruction (new DirectBranch (target, elseBasicBlock));
      currBasicBlock = target;
      if (exitBlock != nullptr)
        *exitBlock = target;
      basicBlocks.push_back (target);
      
    } else if (dynamic_cast <WhileLoop*> (cmd) != nullptr) {
      WhileLoop* loop;
      BasicBlock* testBB;
      Expression* cond;
      BasicBlock* loopBody;
      BasicBlock* loopExit;
      ConditionalBranch* condBr;
      BasicBlock* innerExitBlock;
      
      innerExitBlock = nullptr;
      loop = dynamic_cast <WhileLoop*> (cmd);
      testBB = new BasicBlock ();
      basicBlocks.push_back (testBB);
      loopBody = convertToBasicBlock (loop->getBody (), basicBlocks, 
                                      idVersions, bbVersionMap, &innerExitBlock);
      
      //TODO: Make sure that for every load there is a store from every path coming
      //to this block
      for (auto iter : loopBody->getReads ()) {
        LoadPointer* ld;
        
        ld = new LoadPointer (iter.first, new Pointer (iter.first->getID ()));
        loopBody->insertRead (iter.first, ld);
        loopBody->prependInstruction (ld);
      }
      
      for (auto iter : loopBody->getWrites ()) {
        StorePointer* str;
        
        str = new StorePointer (iter.first, new Pointer (iter.first->getID ()));
        loopBody->insertWrite (iter.first, str);
        loopBody->appendInstruction (str);
      }
      
      loopExit = new BasicBlock ();
      if (exitBlock != nullptr)
        *exitBlock = loopExit;
      basicBlocks.push_back (loopExit);
      
      //testBB will have loopBody as one its predecessors
      if (innerExitBlock != nullptr)
        innerExitBlock->appendInstruction (new BackwardBranch (testBB, innerExitBlock)); 
      else
        loopBody->appendInstruction (new BackwardBranch (testBB, loopBody)); 
      //testBB will have currBasicBlock as one of its predecessors
      currBasicBlock->appendInstruction (new DirectBranch (testBB, 
                                                           currBasicBlock));
      cond = (Expression*) convertToSSAIR (loop->getCondition (),
                                           testBB, idVersions,
                                           bbVersionMap);
      for (auto iter : testBB->getReads ()) {
        LoadPointer* ptr;
        
        ptr = new LoadPointer (iter.first, new Pointer (iter.first->getID()));
        testBB->insertRead (iter.first, ptr);
        testBB->prependInstruction (ptr);
      }
      
      assert (testBB->getWrites ().size () == 0);
      condBr = new ConditionalBranch (cond, loopBody, loopExit, testBB);
      testBB->appendInstruction (condBr);
      currBasicBlock = loopExit;
    } else {
      PHINodePair phiPair;
      Instruction* ssaInstr;
      
      ssaInstr = (Instruction*)convertToSSAIR (cmd, currBasicBlock, 
                                               idVersions, bbVersionMap);
      currBasicBlock->appendInstruction (ssaInstr);
    }
  }
  
  return firstBasicBlock;
}

Program* convertToSSA (ComplexCommand* cmd)
{
  BasicBlockVersionMap bbVersionMap;
  VersionMap idVersions;
  BasicBlock* firstBasicBlock;
  std::vector<BasicBlock*> basicBlocks;
  
  firstBasicBlock = convertToBasicBlock (cmd, basicBlocks, idVersions, 
                                         bbVersionMap, nullptr);
  updateVersionNumberInBB (firstBasicBlock, idVersions, bbVersionMap);
  auto it = std::find (basicBlocks.begin(), basicBlocks.end(), firstBasicBlock);
  basicBlocks.erase (it);
  basicBlocks.insert (basicBlocks.begin(), firstBasicBlock);
  for (auto bb : basicBlocks) {
    bb->print (std::cout);
  }
  
  return new Program (basicBlocks);
}

int main ()
{
  //test1
  {
    //~ // X1 = A1 (input)
    //~ // X2 = A2 (X1)
    std::cout << "Test 1" << std::endl;
    JSONIdentifier X1("X1"), X2("X2");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&CallA2);
    ComplexCommand allCmds(v);
    
    Program* program = convertToSSA (&allCmds);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::vector<WhiskSequence*> seqs;
    WhiskProgram* p = (WhiskProgram*)program->convert (seqs);
    p->generateCommand (std::cout);
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  //~ //test2
  identifiers.clear ();
   {
    //X1 = A1 (input)
    //if X1 then X2 = A2(X1) else X2 = A3(X1)
    //X3 = A4 (X2)
    std::cout << "If then else "<< std::endl;
    JSONIdentifier X1("X1"), X2("X2"), X3 ("X3");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    CallAction CallA3(&X2, "A3", &X1);
    IfThenElseCommand ifX1(&X1, &CallA2, &CallA3);
    CallAction CallA4(&X3, "A4", &X2);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&ifX1);
    v.push_back(&CallA4);
    
    ComplexCommand cmd1(v);
    
    Program* program = convertToSSA (&cmd1);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::cout << std::endl;
    
    std::vector<WhiskSequence*> seqs;
    WhiskAction * p = program->convert (seqs);
    p->generateCommand (std::cout);
    
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  
  //~ {
    //~ //X1 = A1 (input)
    //~ //if X1 then X2_1 = A2(X1) else X2_2 = A3(X1)
    //~ //X2 = phi(then, X2_1, else, X2_2)
    //~ std::cout << "If then else "<< std::endl;
    //~ JSONIdentifier X1("X1"), X2("X2"), X2_1("X2_1"), X2_2("X2_2");
    //~ Input input;
    //~ CallAction CallA1(&X1, "A1", &input);
    //~ CallAction CallA2(&X2_1, "A2", &X1);
    //~ CallAction CallA3(&X2_2, "A3", &X1);
    //~ IfThenElseCommand ifX1(&X1, &CallA2, &CallA3);
    //~ std::vector<std::pair<Command*, JSONIdentifier*> > phi_v;
    //~ phi_v.push_back(std::make_pair (ifX1.getThenBranch (), &X2_1));
    //~ phi_v.push_back(std::make_pair (ifX1.getElseBranch (), &X2_2));
    //~ PHINode X2Phi(phi_v);
    
    //~ ComplexCommand phi_block;
    //~ phi_block.appendSimpleCommand(&X2Phi);
    //~ ifX1.getThenBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    //~ ifX1.getElseBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    
    //~ std::vector<SimpleCommand*> v;
    //~ v.push_back(&CallA1);
    //~ v.push_back(&ifX1);
    
    //~ ComplexCommand cmd1(v);
    
    //~ std::vector<WhiskSequence*> seqs = Converter::convert (&cmd1);
    
    //~ for (auto seq : seqs) {
        //~ seq->generateCommand (std::cout);
        //~ std::cout << std::endl << std::endl;
    //~ }
    //~ //seq->print ();
    //~ std::cout << std::endl;
  //~ }
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test3
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  X1 = A2 (X1)
    //~ //}
    
    std::cout << "While Loop Test" << std::endl;
    JSONIdentifier X1 ("X1");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop (&X1, new CallAction (&X1, "A2", &X1));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* firstBlock = convertToSSA (&cmd1);
    std::vector<WhiskSequence*> seqs;
    firstBlock->convert (seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test3
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  while (X1) {
    //~ //    X1 = A2 (X1)
    //~ //  }
    //~ //}
    
    std::cout << "Nested While Loop Test" << std::endl;
    JSONIdentifier X1 ("X1"), X2("X2");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop2 (&X1, new CallAction (&X1, "A2", &X1));
    WhileLoop loop (&X1, &loop2);
    loop.getBody()->appendSimpleCommand (new CallAction (&X2, "A3", &X1));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* firstBlock = convertToSSA (&cmd1);
    std::vector<WhiskSequence*> seqs;
    firstBlock->convert (seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test4
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  while (X1) {
    //~ //    X1 = A2 (X1)
    //~ //  }
    //~ //}
    
    std::cout << "2 Nested While Loops Test" << std::endl;
    JSONIdentifier X1 ("X1"), X2("X2");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop2 (&X1, new CallAction (&X1, "A2", &X1));
    WhileLoop loop (&X1, &loop2);
    loop.getBody()->appendSimpleCommand (new CallAction (&X2, "A3", &X1));
    loop.getBody()->appendSimpleCommand (new WhileLoop (&X2, new CallAction (&X2, "A4", &X1)));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* firstBlock = convertToSSA (&cmd1);
    std::vector<WhiskSequence*> seqs;
    firstBlock->convert (seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  //While Inside If
  //If Inside while
  //If inside if
}
