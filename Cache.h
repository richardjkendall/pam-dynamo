#include <iostream>

class Cache {
  private:
    std::string realm;
    std::string directory;
    int session_duration;
  
  public:
    Cache(std::string realm, std::string directory, int session_duration);
    bool check_cache(std::string username, std::string password);
    bool save_in_cache(std::string username, std::string password);

    bool save_in_cache_w_salt(std::string username, std::string password, std::string salt);
    bool get_user_from_cache(std::string username, std::string& salt);
};