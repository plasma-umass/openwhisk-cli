#include "driver.h"

#define MAX_SEQ_NAME_SIZE 10

std::string gen_random_str(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string str = "";
    
    for (int i = 0; i < len; ++i) {
        str += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return str;
}

void Converter::convert (ASTNode* node)
{
  char sequence_name[MAX_SEQ_NAME_SIZE];
  gen_random (&sequence_name, MAX_SEQ_NAME_SIZE);
  WhiskSequence sequence ("SEQUENCE_" + sequence_name);
  
  
}
