#include <iostream>
#include <iomanip>

#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h> 
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <openssl/evp.h>

#include "User.h"

User::User(std::string p_ddbtable, std::string p_realm, std::string p_username) {
  ddbtable = p_ddbtable;
  realm = p_realm;
  username = p_username;
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
      }
    } else {
      // could not init digest algorithm
    }
    EVP_MD_CTX_free(mdctx);
  } else {
    // could not get message digest context
  }
  return success;
} 

bool User::authenticate(std::string password) {
  bool ret_val = false;
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  std::cout << "init API" << std::endl;
  {
    // get strings ready
    const Aws::String as_ddbtable(ddbtable.c_str());
    const Aws::String as_realm(realm.c_str());
    const Aws::String as_username(username.c_str());
    
    // get request ready
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = "ap-southeast-2";
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
    // add projection expression (scope and password)
    req.SetProjectionExpression("password,scopes");

    // fire request
    std::cout << "sending request" << std::endl;
    const Aws::DynamoDB::Model::GetItemOutcome& result = dynamoClient.GetItem(req);
    std::cout << "back from call" << std::endl;
    if(result.IsSuccess()) {
      std::cout << "result is a success" << std::endl;
      // got a response
      const Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>& item = result.GetResult().GetItem();
      if (item.size() > 1) {
        // got an item, check to see if password is present
        if(item.count("password") > 0) {
          // get password from record
          Aws::String saved_password = item.find("password")->second.GetS();
          std::string std_saved_password(saved_password.c_str(), saved_password.size());

          std::cout << "password from record = " << std_saved_password << std::endl;
          // calc hash for password provided by user
          std::string hashed_pword;
          if(hash_password(password, hashed_pword)) {
            std::cout << "hashed input password = " << hashed_pword << std::endl;
            // compare the hashes
            if(std_saved_password.compare(hashed_pword) == 0) {
              std::cout << "password matches" << std::endl;
              ret_val = true;
            } else {
              std::cout << "password does not match" << std::endl;
              ret_val = false;
            }
          } else {
            // failed to hash password
          }
        } else {
          std::cout << "no item with password found" << std::endl;
        }
      } else {
        // got no items, user does not exist
        std::cout << "No user found in realm=" << realm << ", with username=" << username << std::endl;
        ret_val = false;
      }
    } else {
      std::cout << "Failed to query table=" << ddbtable << ", error message=" << result.GetError().GetMessage();
      ret_val = false;
    }

  }
  Aws::ShutdownAPI(options);
  return ret_val;
}