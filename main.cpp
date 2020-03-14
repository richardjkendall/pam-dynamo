#include <iostream>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <string.h>

#include "User.h"
#include "Log.h"

// external defs
extern "C" int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv);
extern "C" int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv);
extern "C" int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv);

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  // dummy, does not do anything
  return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  // dummy, does not do anything
  return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
  int ret_val;

  std::clog.rdbuf(new Log("libpam-dynamo", LOG_LOCAL0));

  // get the username
  const char* pam_username;
	ret_val = pam_get_user(pamh, &pam_username, "Username: ");
	if (ret_val != PAM_SUCCESS) {
		return ret_val;
	}

  // get the password
  const char* pam_authtok;
  ret_val = pam_get_authtok(pamh, PAM_AUTHTOK, &pam_authtok, "Password: ");
  if (ret_val != PAM_SUCCESS) {
		return ret_val;
	}

  // now check user/password against auth store
  std::string REGION(argv[0]);
  std::string TABLE(argv[1]);
  std::string REALM(argv[2]);
  std::string CACHE_LOC(argv[3]);
  std::string CACHE_DUR(argv[4]);

  int I_CACHE_DUR = 60;
  try {
    I_CACHE_DUR = std::atoi(CACHE_DUR.c_str());
  } catch(std::invalid_argument const &e) {
    std::clog << kLogErr << "Could not convert " << CACHE_DUR << " to integer: invalid.  Using default = 60" << std::endl;
  } catch(std::out_of_range const &e) {
    std::clog << kLogErr << "Could not convert " << CACHE_DUR << " to integer: overflow.  Using default = 60" << std::endl;
  }

  std::string s_pam_username(pam_username, strlen(pam_username));
  std::string s_pam_authtok(pam_authtok, strlen(pam_authtok));

  std::clog << kLogInfo << "REGION = " << REGION << ", TABLE = " << TABLE << ", REALM = " << REALM << ", USER = " << s_pam_username << std::endl;
  std::clog << kLogInfo << "CACHE FOLDER = " << CACHE_LOC << ", SESSION_DUR = " << CACHE_DUR << std::endl;

  User u(REGION, TABLE, REALM, CACHE_LOC, I_CACHE_DUR, s_pam_username);
  if(u.authenticate(s_pam_authtok)) {
    std::clog << kLogInfo << "Returning PAM_SUCCESS" << std::endl;
    return PAM_SUCCESS;
  } else {
    std::clog << kLogInfo << "Returning PAM_AUTH_ERR" << std::endl;
    return PAM_AUTH_ERR;
  }
}