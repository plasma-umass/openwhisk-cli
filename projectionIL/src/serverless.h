#include <string>
#include <vector>
#include <string.h>
#include "utils.h"

#ifndef __SERVERLESS_H__
#define __SERVERLESS_H__

class ServerlessAction
{
private:
  std::string name;
  
public:
  ServerlessAction(std::string _name) : name(_name)
  {
  }
  
  virtual ~ServerlessAction () {}
  
  const char* getName () {return name.c_str ();}
  
  virtual void print () = 0;
  virtual void generateCommand(std::ostream& os) = 0;
  virtual std::string getNameForSeq () {return getName ();}
};

class ServerlessSequence : public ServerlessAction
{
protected:
  std::vector<ServerlessAction*> actions;
  
public:
  ServerlessSequence(std::string name) : ServerlessAction (name)
  {
  }
  
  ServerlessSequence(std::string name, std::vector<ServerlessAction*> _actions) : ServerlessAction(name), actions(_actions)
  {
  }
  
  std::vector<ServerlessAction*>& getActions () {return actions;}
  void appendAction (ServerlessAction* action) {actions.push_back (action);}
  void insertAction (ServerlessAction* action, int index) {actions.insert (actions.begin()+index, action);}
};

class ServerlessProjection : public ServerlessAction
{
protected:
  std::string projCode;
  
public:
  ServerlessProjection (std::string name, std::string _code) : ServerlessAction (name), projCode (_code)
  {
  }
  
  std::string getProjCode () {return projCode;}
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskProjection: '%s', %s)", getName (), projCode.c_str ());
  }
};

class ServerlessFork : public ServerlessAction
{
protected:
  std::string innerActionName;
  ServerlessAction* innerAction;
  std::string resultProjectionName;
  std::string returnName;
  std::string requiredFields;
  
public:
  ServerlessFork (std::string name, std::string _innerActionName, std::string _returnName, std::string _requiredFields) : 
    ServerlessAction(name), innerActionName(_innerActionName), returnName(_returnName), requiredFields(_requiredFields)
  {
  }
  
  ServerlessFork (std::string name, ServerlessAction* _innerAction, std::string _returnName, std::string _requiredFields) : 
    ServerlessAction(name), innerAction(_innerAction), returnName (_returnName), requiredFields(_requiredFields)
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
  
  ServerlessAction* getInnerAction() {return innerAction;}
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskFork '%s', '%s')", getName (), innerActionName.c_str ());
  }
};

class ServerlessApp : public ServerlessAction
{
private:
public:
  ServerlessApp () : ServerlessAction("App")
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

class ServerlessProgram : public ServerlessAction
{
protected:
  std::vector <ServerlessSequence*> basicBlocks;

public:
  ServerlessProgram (std::string _name) : ServerlessAction(_name)
  {}
  
  ServerlessProgram (std::string _name, std::vector <ServerlessSequence*> _basicBlocks) : ServerlessAction(_name), basicBlocks(_basicBlocks)
  {}
  
  void addBasicBlock (ServerlessSequence* block)
  {
    basicBlocks.push_back (block);
  }
};

#endif
