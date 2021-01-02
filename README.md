# pam-dynamo

![build-image](https://github.com/richardjkendall/pam-dynamo/workflows/build-image/badge.svg)

This is a PAM module which authenticates users against a DyanmoDB table.

It has the following features:

* configurable AWS region, DynamoDB table and 'realm' all from the pam.d file
* passwords are hashed using SHA3-256
* passwords are optionally salted with a user specific salt
* configurable caching to a local sqlite3 database to increase response time and reduce AWS calls
* logging to Syslog
* packaged as a Docker image for further use (based on Ubuntu 18.04 LTS 'bionic')
* will pick up AWS credentials as normal e.g. in your .aws folder, from environment variables or via instance/task roles if running on AWS infrastructure

---
**NOTES**
* This module only implements pam_sm_authenticate, all other calls will return PAM_SUCCESS
* v2 adds support for salted password hashes.

---

## How to use/build
The easiest way is to extend the pre-built Docker image which is available here: https://hub.docker.com/r/richardjkendall/ubuntu-pam-dynamo

There is an example of how to do this here: https://github.com/richardjkendall/basicauth-rproxy

If you want to build it yourself then you need:

* AWS CPP SDK (DynamoDB only), build instructions are here: https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/setup.html
* Cmake
* GCC and make
* OpenSSL
* pam development library

### Build instructions

1. Clone the repo
2. Ensure you have the pre-reqs installed
3. Create a new build folder
4. From the build folder, run ``cmake -DBUILD_SHARED_LIBS=ON -DPAM_DIR=/<path to source> /<path to source>``
5. Run ``make``
6. If you want to package as a .deb then run ``cpack -G DEB``

## PAM Module Config
Here is an example pam.d file which will work with this module
```
account required        libpam-dynamo.so        ${REGION}  ${TABLE}  ${REALM} ${CACHE_FOLDER} ${CACHE_DURATION}
auth    required        libpam-dynamo.so        ${REGION}  ${TABLE}  ${REALM} ${CACHE_FOLDER} ${CACHE_DURATION}
```

Be sure to replace the variables as follows:

* REGION = AWS region e.g. ap-southeast-2
* TABLE = DynamoDB table name
* REALM = name of a realm configured in your table
* CACHE_FOLDER = directory where cache databases will be created
* CACHE_DURATION = number of seconds that cache entries will be valid for

## Expected table structure
The module expects the table to look as follows:

| Field | Type | Role |
|---|---|---|
|realm|String|Hash key|
|username|String|Sort key|
|password|String|contains SHA3-256 hashed password, optionally salted, see below|
|salt|String|An optional string which is used to salt the password before hashing

Where the `salt` field is present, it will be used when comparing the password provided in the pam_sm_authenticate call.

The salt is prefixed to the password before hashing.  So if the password is 12345 and the salt is abcde then the hashed value would be abcde12345.
