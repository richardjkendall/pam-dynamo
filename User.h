#include <iostream>

class User {
  private:
    std::string region;
    std::string ddbtable;
    std::string realm;
    std::string username;
    std::string password;

  public:
    User(std::string region, std::string ddbtable, std::string realm, std::string username);
    bool authenticate(std::string password);
};