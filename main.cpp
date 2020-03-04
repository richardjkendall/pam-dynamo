#include <iostream>
#include "User.h"

int main(int argc, char* argv[]) {
  std::string TABLE = "basicAuthUsers";
  std::string REALM = "test";
  std::string USER = "rjk";

  std::cout << "TABLE = " << TABLE << std::endl;
  std::cout << "REALM = " << REALM << std::endl;
  std::cout << "USER = " << USER << std::endl;
  User u(TABLE, REALM, USER);
  u.authenticate(argv[1]);
}