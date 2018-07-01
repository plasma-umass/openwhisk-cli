#include <string>
#include <vector>
#include <string.h>
#include "utils.h"

#ifndef __WHISK_ACTION_H__
#define __WHISK_ACTION_H__

#define WHISK_CLI_PATH "./wsk"
#define WHISK_CLI_ARGS "-i"
#define ECHO(x) (std::string("echo \"")+x+"\"")
enum
{
  WHISK_FORK_NAME_LENGTH = 10,
  WHISK_SEQ_NAME_LENGTH = 10,
  WHISK_PROJ_NAME_LENGTH = 10
};
  
class WhiskAction
{
private:
  std::string name;
  
public:
  WhiskAction(std::string _name) : name(_name)
  {
  }
  
  virtual ~WhiskAction () {}
  
  const char* getName () {return name.c_str ();}
  
  virtual void print () = 0;
  virtual void generateCommand(std::ostream& os) = 0;
  virtual std::string getNameForSeq () {return getName ();}
};

class WhiskSequence : public WhiskAction
{
private:
  std::vector<WhiskAction*> actions;
public:
  WhiskSequence(std::string name) : WhiskAction (name)
  {
  }
  
  WhiskSequence(std::string name, std::vector<WhiskAction*> _actions) : WhiskAction(name), actions(_actions)
  {
  }
  
  std::vector<WhiskAction*>& getActions () {return actions;}
  void appendAction (WhiskAction* action) {actions.push_back (action);}
  void insertAction (WhiskAction* action, int index) {actions.insert (actions.begin()+index, action);}
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskSequence %s, %ld, (", getName (), actions.size ());
    
    for (auto action : actions) {
      action->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    for (auto action : actions) {
      action->generateCommand (os);
    }
    
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --sequence ";
    if (actions.size () > 0) {
      for (int i = 0; i < actions.size () - 1; i++) {
        os << actions[i]->getNameForSeq () << ",";
      }
      
      os << actions[actions.size () - 1]->getNameForSeq () << std::endl;
    }
  }
};

class WhiskProjection : public WhiskAction
{
private:
  std::string projCode;
  
public:
  WhiskProjection (std::string name, std::string _code) : WhiskAction (name), projCode (_code)
  {
  }
  
  std::string getProjCode () {return projCode;}
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskProjection: '%s', %s)", getName (), projCode.c_str ());
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    char temp[256];
    assert (getProjectionTempFile (temp, 256) != -1);
    os << ECHO(getProjCode ()) << " > " << temp << "\n";
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --projection " << temp << "\n";
  }
};

class WhiskFork : public WhiskAction
{
private:
  std::string innerActionName;
  WhiskAction* innerAction;
  std::string resultProjectionName;
  std::string returnName;
  
public:
  WhiskFork (std::string name, std::string _innerActionName, std::string _returnName) : 
    WhiskAction(name), innerActionName(_innerActionName), returnName(_returnName)
  {
  }
  
  WhiskFork (std::string name, WhiskAction* _innerAction, std::string _returnName) : 
    WhiskAction(name), innerAction(_innerAction), returnName (_returnName)
  {
    innerActionName = innerAction->getName ();
  }
  
  std::string getInnerActionName()
  {
    return innerActionName;
  }
  
  std::string getResultProjectionName ()
  {
    return resultProjectionName;
  }
  
  WhiskAction* getInnerAction() {return innerAction;}
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskFork '%s', '%s')", getName (), innerActionName.c_str ());
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " <<
      getName() << " --fork " << getInnerActionName() << std::endl;
    
    char temp[256];
    assert (getProjectionTempFile (temp, 256) != -1);
    char code[2048];
    resultProjectionName = "Proj_"+gen_random_str (WHISK_PROJ_NAME_LENGTH);
    sprintf (code, R"(. * {\"saved\": {\"%s\": .input}})", returnName.c_str());
    os << ECHO(code) << " > " << temp << std::endl;
    os << WHISK_CLI_PATH << " " WHISK_CLI_ARGS << " action update " << 
       resultProjectionName << " --projection " << temp << std::endl;
  }
};

class WhiskProjForkPair : public WhiskAction
{
private:
  WhiskFork* fork;
  WhiskProjection* proj;

public:
  WhiskProjForkPair (WhiskProjection* _proj, WhiskFork* _fork):
    WhiskAction ("ProjForkPair_" + gen_random_str (WHISK_FORK_NAME_LENGTH)),
    fork(_fork), proj(_proj)
  {
  }
  
  virtual void print ()
  {
    proj->print ();
    fprintf (stdout, " -> ");
    fork->print ();
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    proj->generateCommand(os);
    fork->generateCommand(os);
  }
  
  virtual std::string getNameForSeq () {return proj->getName () + std::string(",") + fork->getName () + "," + fork->getResultProjectionName();}
};

class WhiskApp : public WhiskAction
{
private:
public:
  WhiskApp () : WhiskAction("App")
  {}
  
  virtual void print ()
  {
    fprintf (stdout, "App");
  }
  
  virtual void generateCommand (std::ostream& os)
  {
    //os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action invoke " << getName () << std::endl;
  }
};

class WhiskDirectBranch : public WhiskApp
{
private:
  std::string target;
  WhiskProjection* proj;
  
public:
  WhiskDirectBranch (std::string _target) : target(_target)
  {
    proj = new WhiskProjection ("Proj_DirectBranch_" +gen_random_str (WHISK_PROJ_NAME_LENGTH), 
                       R"(. * {\"action\":\")" + target+R"(\"})"); //TODO: Wrap correctly in app.
    
  }
  
  virtual void print ()
  {
    fprintf (stdout, "App (%s)", target.c_str ());
  }
  
  virtual void generateCommand (std::ostream& os)
  {
    proj->generateCommand (os);
    //os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action invoke " << getName () << std::endl;
  }
  
  virtual std::string getNameForSeq () {return std::string(proj->getName ());}
};

class WhiskProgram : public WhiskAction
{
private:
  std::vector <WhiskSequence*> basicBlocks;

public:
  WhiskProgram (std::string _name) :WhiskAction(_name)
  {}
  
  WhiskProgram (std::string _name, std::vector <WhiskSequence*> _basicBlocks) :WhiskAction(_name), basicBlocks(_basicBlocks)
  {}
  
  void addBasicBlock (WhiskSequence* block)
  {
    basicBlocks.push_back (block);
  }
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskProgram %s, %ld, (", getName (), basicBlocks.size ());
    
    for (auto block : basicBlocks) {
      block->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    for (auto block : basicBlocks) {
      block->generateCommand (os);
      std::cout << std::endl;
    }
    
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --program ";
    if (basicBlocks.size () > 0) {
      for (int i = 0; i < basicBlocks.size () - 1; i++) {
        os << basicBlocks[i]->getNameForSeq () << ",";
      }
      
      os << basicBlocks[basicBlocks.size () - 1]->getNameForSeq () << std::endl;
    }
  }
};
#endif
