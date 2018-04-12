%module ilswig
%{
#include "ast.h"
#include "driver.h"
#include "utils.h"
#include "whisk_action.h"
#include "ast.cpp"
#include "driver.cpp"
%}

%include <std_vector.i>
%include <std_string.i>
%include "ast.h"
%include "driver.h"
%include "utils.h"
%include "whisk_action.h"

namespace std {
%template(VectorSimpleCommand) vector<SimpleCommand*>;
}
