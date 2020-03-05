#include <iostream>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
//#include <security/pam_ext.h>

#include <string.h>

#include "User.h"

// external defs
extern "C" /*PAM_EXTERN*/ int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv);
extern "C" /*PAM_EXTERN*/ int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv);
extern "C" /*PAM_EXTERN*/ int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv);

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

  // get the username
  const char* pam_username;
	ret_val = pam_get_user(pamh, &pam_username, NULL);
	if (ret_val != PAM_SUCCESS) {
		return ret_val;
	}

  // get the password
  const char* pam_authtok;
  ret_val = pam_get_authtok(pamh, PAM_AUTHTOK, &pam_authtok, NULL);
  if (ret_val != PAM_SUCCESS) {
		return ret_val;
	}

  // now check user/password against auth store
  // TODO need to get table and realm from pam config
  std::string TABLE = "basicAuthUsers";
  std::string REALM = "test";
  std::string s_pam_username(pam_username, strlen(pam_username));
  std::string s_pam_authtok(pam_authtok, strlen(pam_authtok));
  User u(TABLE, REALM, s_pam_username);
  if(u.authenticate(s_pam_authtok)) {
    return PAM_SUCCESS;
  } else {
    return PAM_AUTH_ERR;
  }
}


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