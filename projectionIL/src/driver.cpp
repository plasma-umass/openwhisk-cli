#include "driver.h"
#include "ssa.h"

#include <vector>
#include <string>
#include <iostream>
#include <typeinfo>
#include <set>
#include <utility>

#define MAX_SEQ_NAME_SIZE 10

typedef std::unordered_map <std::string, int> VersionMap;
typedef std::unordered_map <BasicBlock*, std::unordered_map <std::string, int> > BasicBlockVersionMap;
typedef std::unordered_map <std::string, std::vector<std::pair <BasicBlock*, std::string>>> PHINodePair;

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

std::string identifierForVersion (std::string id, VersionMap& versionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  }
  
  return id + "_"+std::to_string (versionMap[id]);
}

std::string updateVersionNumber (std::string id, VersionMap& versionMap, 
                                 VersionMap& bbVersionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  } else {
    versionMap [id] += 1;
  }
  
  bbVersionMap[id] = versionMap[id];
  
  return identifierForVersion (id, versionMap);
}

void allPredsDefiningID (BasicBlock* currBasicBlock, std::string id,
                         BasicBlockVersionMap& bbVersionMap, 
                         PHINodePair& phiPair)
{
  auto preds = currBasicBlock->getPredecessors();
  
  if (preds.size () < 2) {
    return;
  }
  
  for (auto pred : preds) {
    if (bbVersionMap[pred].find (id) != bbVersionMap[pred].end ()) {
      phiPair[id].push_back (std::make_pair (pred, id));
    } else {
      allPredsDefiningID (pred, id, bbVersionMap, phiPair);      
    }
  }
}

IRNode* convertToSSAIR (ASTNode* astNode, BasicBlock* currBasicBlock,
                        VersionMap& idVersions, 
                        BasicBlockVersionMap& bbVersionMap,
                        PHINodePair& phiPair)
{
  if (dynamic_cast <JSONIdentifier*> (astNode) != nullptr) {
    JSONIdentifier* jsonId;
    
    jsonId = dynamic_cast <JSONIdentifier*> (astNode);
    return new Identifier (identifierForVersion (jsonId->getIdentifier (), idVersions));
  } else if (dynamic_cast <ReturnJSON*> (astNode) != nullptr) {
    ReturnJSON* retJson;
    
    retJson = dynamic_cast <ReturnJSON*> (astNode);
    abort (); //return new Return ((Expression*)convertToSSAIR (retJson->getReturnExpr ()));
  } else if (dynamic_cast <CallAction*> (astNode) != nullptr) {
    CallAction* callAction;
    std::string newOutputID;
    Identifier* newInput;
    std::string newInputID;
    std::string inputID;

    callAction = dynamic_cast <CallAction*> (astNode);
    inputID = ((JSONIdentifier*)callAction->getArgument ())->getIdentifier ();
    newInputID = identifierForVersion (inputID, idVersions);
    newInput = new Identifier (newInputID);
    newOutputID = updateVersionNumber (callAction->getReturnValue()->getIdentifier (),
                                       idVersions, 
                                       bbVersionMap[currBasicBlock]);
    allPredsDefiningID (currBasicBlock, inputID, bbVersionMap, phiPair);
    return new Call (new Identifier (newOutputID), callAction->getActionName (),
                     newInput);
                     //TODO: (Expression*)convertToSSAIR (callAction->getArgument ()));
  } else if (dynamic_cast <JSONTransformation*> (astNode) != nullptr) {
    JSONTransformation* trans;
    
    trans = dynamic_cast <JSONTransformation*> (astNode);
    return new Transformation ((Identifier*) convertToSSAIR (trans->getOutput (), currBasicBlock, idVersions, bbVersionMap, phiPair),
                                (Identifier*) convertToSSAIR (trans->getInput (), currBasicBlock, idVersions, bbVersionMap, phiPair),
                                (Expression*) convertToSSAIR (trans->getTransformation (), currBasicBlock, idVersions, bbVersionMap, phiPair));
  } else if (dynamic_cast <LetCommand*> (astNode) != nullptr) {
    abort ();
  } else if (dynamic_cast <NumberExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <StringExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <BooleanExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <JSONArrayExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <KeyValuePair*> (astNode) != nullptr) {
  } else if (dynamic_cast <JSONObjectExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <JSONInput*> (astNode) != nullptr) {
    return new Input ();
  } else if (dynamic_cast <JSONPatternApplication*> (astNode) != nullptr) {
  } else if (dynamic_cast <FieldGetJSONPattern*> (astNode) != nullptr) {
  } else if (dynamic_cast <ArrayIndexJSONPattern*> (astNode) != nullptr) {
  } else if (dynamic_cast <KeyGetJSONPattern*> (astNode) != nullptr) {
  } else {
    fprintf (stderr, "Invalid AstNode type\n");
    abort ();
  }
  
  abort ();
}

BasicBlock* convertToBasicBlock (ComplexCommand* complexCmd, 
                                 std::vector<BasicBlock*>& basicBlocks, 
                                 VersionMap& idVersions,
                                 BasicBlockVersionMap& bbVersionMap)
{
  //This function takes one ComplexCommand containing all other
  //simple commands.
  
  //First convert ComplexCommand to a set of Basic Blocks connected
  //by branches.
  
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
                                            bbVersionMap);
      elseBasicBlock = convertToBasicBlock (ifThenElsecmd->getElseBranch (), 
                                            basicBlocks, idVersions, 
                                            bbVersionMap);
      cond = (Expression*) convertToSSAIR (ifThenElsecmd->getCondition (), 
                                           currBasicBlock, idVersions, 
                                           bbVersionMap, phiPair);
      condBr = new ConditionalBranch (cond, thenBasicBlock, 
                                      elseBasicBlock, currBasicBlock);
      currBasicBlock->appendInstruction (condBr);
      target = new BasicBlock ();
      thenBasicBlock->appendInstruction (new DirectBranch (target, thenBasicBlock));
      elseBasicBlock->appendInstruction (new DirectBranch (target, elseBasicBlock));
      currBasicBlock = target;
      basicBlocks.push_back (target);
    } else {
      PHINodePair phiPair;
      Instruction* ssaInstr;
      
      ssaInstr = (Instruction*)convertToSSAIR (cmd, currBasicBlock, idVersions, bbVersionMap, phiPair);
      currBasicBlock->appendInstruction (ssaInstr);
      
      //~ if (phiPair.size () > 0) {
        //~ for (auto iter : phiPair) {
          //~ std::cout << iter.first << std::endl;
          //~ for (auto vecIter : iter.second) {
            //~ std::cout << vecIter.first << " " << vecIter.second << std::endl;
          //~ }
        //~ }
      //~ }
    }
  }
  
  return firstBasicBlock;
}

BasicBlock* convertToSSA (ComplexCommand* cmd)
{
  BasicBlockVersionMap bbVersionMap;
  VersionMap idVersions;
  BasicBlock* firstBasicBlock;
  std::vector<BasicBlock*> basicBlocks;
  
  firstBasicBlock = convertToBasicBlock (cmd, basicBlocks, idVersions, 
                                         bbVersionMap);
  
  for (auto bb : basicBlocks) {
    
  }
  
  for (auto bb : basicBlocks) {
    bb->print (std::cout);
  }
  
  return firstBasicBlock;
}

int main ()
{
  //test1
  {
    // X1 = A1 (input)
    // X2 = A2 (X1)
    std::cout << "Test 1" << std::endl;
    JSONIdentifier X1("X1"), X2("X2");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&CallA2);
    ComplexCommand allCmds(v);
    
    BasicBlock* firstBlock = convertToSSA (&allCmds);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    //~ std::vector<WhiskSequence*> seqs = Converter::convert (&allCmds);
    //~ for (auto seq : seqs) {
        //~ seq->generateCommand (std::cout);
        //~ std::cout << std::endl << std::endl;
    //~ }
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  //test2
  
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
    
    BasicBlock* firstBlock = convertToSSA (&cmd1);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::cout << std::endl;
    
    //std::vector<WhiskSequence*> seqs = Converter::convert (&cmd1);
    
    //for (auto seq : seqs) {
//        seq->generateCommand (std::cout);
  //      std::cout << std::endl << std::endl;
    //}
    //seq->print ();
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
    
    //~ std::cout << "While Loop Test" << std::endl;
    //~ JSONIdentifier X1 ("X1"), X2 ("X2"), X3("X3"), X4("X4");
    //~ Pointer X2_ptr ("X2_ptr");
    //~ Input input;
    
    //~ CallAction A1 (&X1, "A1", &input);
    //~ CallAction A2 (&
  }
}
