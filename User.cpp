#include <iostream>
#include <iomanip>

#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h> 
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <openssl/evp.h>

#include "User.h"
#include "Log.h"
#include "Cache.h"

User::User(std::string p_region, std::string p_ddbtable, std::string p_realm, std::string p_dir, int dur, std::string p_username) {
  region = p_region;
  ddbtable = p_ddbtable;
  realm = p_realm;
  username = p_username;
  cache_location = p_dir;
  session_dur = dur;
}

// based on this code https://stackoverflow.com/questions/2262386/generate-sha256-with-openssl-and-c
// and this code https://wiki.openssl.org/index.php/EVP_Message_Digests
bool hash_password(const std::string& unhashed, std::string& hashed) {
	EVP_MD_CTX *mdctx;
  bool success = false;

  mdctx = EVP_MD_CTX_new();
	if(mdctx != NULL) {
    if(EVP_DigestInit_ex(mdctx, EVP_sha3_256(), NULL)) {
      if(EVP_DigestUpdate(mdctx, unhashed.c_str(), unhashed.length())) {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int lengthOfHash = 0;

        if(EVP_DigestFinal_ex(mdctx, hash, &lengthOfHash)) {
          std::stringstream ss;
          for(unsigned int i = 0; i < lengthOfHash; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
          }

          hashed = ss.str();
          success = true;
        }
      } else {
        // could not add message to digest
        std::clog << kLogErr << "Unable to add password to digest" << std::endl;
      }
    } else {
      // could not init digest algorithm
      std::clog << kLogErr << "Unable to init digest algorithm" << std::endl;
    }
    EVP_MD_CTX_free(mdctx);
  } else {
    // could not get message digest context
    std::clog << kLogErr << "Could not get message digest context" << std::endl;
  }
  return success;
} 

bool User::authenticate(std::string password) {
  bool ret_val = false;

  // need to hash the password before we check the cache
  std::string hashed_pword;
  if(hash_password(password, hashed_pword)) {
    std::clog << kLogDebug << "Hashed input password = " << hashed_pword << std::endl;
  } else {
    // failed to hash password
    std::clog << kLogErr << "Hashing password failed" << std::endl;
    return(false);
  }

  // need to check for a valid cache record with salt
  std::clog << kLogInfo << "Checking cache for valid salt value" << std::endl;
  Cache c(realm, cache_location, session_dur);
  std::string cache_salt;
  if(c.get_user_from_cache(username, cache_salt)) {
    // there's a record in the cache 
    std::clog << kLogInfo << "Got salt from cache = " << cache_salt << std::endl;
    // so we need to rehash the password along with the salt
    hash_password(cache_salt + password, hashed_pword);
    std::clog << kLogInfo << "Rehashed password = " << hashed_pword << std::endl;
  }

  // check the cache 
  std::clog << kLogInfo << "Checking cache for username/password combination" << std::endl;
  if(c.check_cache(username, hashed_pword)) {
    // user is in the cache
    // so exit here with true
    std::clog << kLogInfo << "User found in cache, skipping AWS call" << std::endl;
    return(true);
  }

  std::clog << kLogInfo << "User not in cache, calling AWS..." << std::endl;
  // user not in cache, so keep running

  Aws::SDKOptions options;
  std::clog << kLogInfo << "Init for AWS API" << std::endl;
  Aws::InitAPI(options);
  {
    // get strings ready
    const Aws::String as_ddbtable(ddbtable.c_str());
    const Aws::String as_realm(realm.c_str());
    const Aws::String as_username(username.c_str());
    const Aws::String as_region(region.c_str());

    // get request ready
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = as_region;
    Aws::DynamoDB::DynamoDBClient dynamoClient(clientConfig);
    Aws::DynamoDB::Model::GetItemRequest req;
    
    // set table
    req.SetTableName(as_ddbtable);
    // add hash key (auth realm)
    Aws::DynamoDB::Model::AttributeValue hashKey;
    hashKey.SetS(as_realm);
    req.AddKey("realm", hashKey);
    // add sort key (username)
    Aws::DynamoDB::Model::AttributeValue sortKey;
    sortKey.SetS(as_username);
    req.AddKey("user", sortKey);

    // fire request to API
    std::clog << kLogInfo << "Calling DynamoDB API in region=" << region << std::endl;
    const Aws::DynamoDB::Model::GetItemOutcome& result = dynamoClient.GetItem(req);
    std::clog << kLogInfo << "Back from call to API" << std::endl;
    if(result.IsSuccess()) {
      std::clog << kLogInfo << "Result is a success from API" << std::endl;
      // got a response
      const Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>& item = result.GetResult().GetItem();
      if (item.size() > 1) {
        // got an item, check to see if password is present
        if(item.count("password") > 0) {
          // check if we have salt
          std::string std_salt;
          bool salted = false;
          if(item.count("salt") > 0) {
            std::clog << kLogInfo << "Got salt, so re-hashing password with salt" << std::endl;
            Aws::String salt = item.find("salt")->second.GetS();
            std_salt = std::string(salt.c_str(), salt.size());
            hash_password(std_salt + password, hashed_pword);
            salted = true;
          }
          // get password from record
          Aws::String saved_password = item.find("password")->second.GetS();
          std::string std_saved_password(saved_password.c_str(), saved_password.size());

          std::clog << kLogDebug << "Password from record = " << std_saved_password << std::endl;
          // compare the password hashes
          if(std_saved_password.compare(hashed_pword) == 0) {
            std::clog << kLogInfo << "Password matches what is stored" << std::endl;
            // need to update the cache
            // if salted we should store the salt
            if(salted) {
              if(c.save_in_cache_w_salt(username, hashed_pword, std_salt)) {
                std::clog << kLogInfo << "Saved in cache with salt" << std::endl;
              } else {
                std::clog << kLogErr << "Unable to save in cache, check previous messages" << std::endl;
              }
              ret_val = true;
            } else {
              if(c.save_in_cache(username, hashed_pword)) {
                std::clog << kLogInfo << "Saved in cache without salt" << std::endl;
              } else {
                std::clog << kLogErr << "Unable to save in cache, check previous messages" << std::endl;
              }
              ret_val = true;
            }
          } else {
            std::clog << kLogInfo << "Password does not match what is stored" << std::endl;
            ret_val = false;
          }
        } else {
          std::clog << kLogInfo << "No item with password field found" << std::endl;
        }
      } else {
        // got no items, user does not exist
        std::clog << kLogInfo << "No user found in realm=" << realm << ", with username=" << username << std::endl;
        ret_val = false;
      }
    } else {
      std::clog << kLogErr << "Failed to query table=" << ddbtable << ", error message=" << result.GetError().GetMessage();
      ret_val = false;
    }

  }
  Aws::ShutdownAPI(options);
  return ret_val;
}