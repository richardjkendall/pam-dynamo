#include <iostream>
#include <time.h>
#include <string.h>

#include <sqlite3.h>

#include "Cache.h"
#include "Log.h"

const char *CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS users (" \
  "username TEXT NOT NULL PRIMARY KEY," \
  "password TEXT NOT NULL," \
  "timestamp INT NOT NULL);";

const char *CREATE_INDEX_SQL = "CREATE INDEX IF NOT EXISTS username_idx ON users(username);";

const char *CHECK_FOR_USER_SQL = "SELECT timestamp FROM users WHERE username=?1 AND password=?2 AND timestamp>?3;";

const char *ADD_USER_TO_CACHE_SQL = "INSERT INTO users(username, password, timestamp) " \
  "VALUES(?1, ?2, ?3) " \
  "ON CONFLICT(username) DO UPDATE SET " \
  "password = ?2, " \
  "timestamp = ?3;";

Cache::Cache(std::string p_realm, std::string p_directory, int p_session_duration) {
  realm = p_realm;
  directory = p_directory;
  session_duration = p_session_duration;
}

bool check_and_init(std::string p_dir, std::string p_realm, sqlite3 **db) {
  // creates sqlite3 db if missing
  // checks if user table exists and if not creates it
  int rc = 0;
  char *zErrMsg = 0;

  std::string db_filepath = p_dir + "/" + p_realm + "_cache.db";
  rc = sqlite3_open(db_filepath.c_str(), db);
  if(rc) {
    // there was an issue
    std::clog << kLogErr << "Can't open database: " << sqlite3_errmsg(*db) << std::endl;
    return(false);
  } else {
    std::clog << kLogInfo << "Connected to database: " << db_filepath << std::endl;
    // we are connected, now check for table and create
    rc = sqlite3_exec(*db, CREATE_TABLE_SQL, NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
      std::clog << kLogErr << "Error creating table: " << zErrMsg << std::endl;
      sqlite3_free(zErrMsg);
      return(false);
    }
    // we created the table, now create the index
    rc = sqlite3_exec(*db, CREATE_INDEX_SQL, NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
      std::clog << kLogErr << "Error creating index: " << zErrMsg << std::endl;
      sqlite3_free(zErrMsg);
      return(false);
    }
    // all done, return true
    return(true);
  }
}

bool prep_stmt(sqlite3 **db, sqlite3_stmt **stmt, const char *sql) {
  int rc = 0;
  rc = sqlite3_prepare_v2(*db, sql, strlen(sql), stmt, NULL);
  if(rc != SQLITE_OK) {
    std::clog << kLogErr << "Failed to prepare statement, error code=" << rc << ", error message=" << sqlite3_errmsg(*db) << std::endl;
    return false;
  }
  return true;
}

bool add_text_bind(sqlite3_stmt *stmt, int index, std::string text, const char *field_name) {
  int rc = 0;
  rc = sqlite3_bind_text(stmt, index, text.c_str(), -1, SQLITE_TRANSIENT);
  if(rc != SQLITE_OK) {
    std::clog << kLogErr << "Unable to bind @index=" << index << " field_name=" << field_name << std::endl;
    return(false);
  }
  return(true);
}

bool add_int_bind(sqlite3_stmt *stmt, int index, int value, const char *field_name) {
  int rc = 0;
  rc = sqlite3_bind_int(stmt, index, value);
  if(rc != SQLITE_OK) {
    std::clog << kLogErr << "Unable to bind @index=" << index << " field_name=" << field_name << std::endl;
    return(false);
  }
  return(true);
}

bool run_stmt(sqlite3_stmt *stmt, int expected_rc) {
  int rc = 0;
  rc = sqlite3_step(stmt);
  std::clog << kLogInfo << "Response from sqlite=" << rc << " expected=" << expected_rc << std::endl;
  if(rc == expected_rc) {
    return true;
  } else {
    return false;
  }
}

bool save_user_in_cache(sqlite3 **db, std::string username, std::string password) {
  bool success = true;

  // prepare SQL statement
  sqlite3_stmt *stmt;
  if(prep_stmt(db, &stmt, ADD_USER_TO_CACHE_SQL)) {
    std::clog << kLogInfo << "Statement prepared" << std::endl;
    // add binds
    if(!add_text_bind(stmt, 1, username, "username")) {
      success = false;
    }
    if(!add_text_bind(stmt, 2, password, "password")) {
      success = false;
    }
    int current_ts = (int)time(NULL);
    if(!add_int_bind(stmt, 3, current_ts, "timestamp")) {
      success = false;
    }
    std::clog << kLogInfo << "Binds added" << std::endl;
    // run statement
    if(success) {
      if(run_stmt(stmt, SQLITE_DONE)) {
        success = true;
      } else {
        success = false;
      }
    } 
  } else {
    std::clog << kLogErr << "Unable to prepare statement" << std::endl;
  }
  // free the statement as we finished with it
  sqlite3_finalize(stmt);
  return success;
}

bool check_for_user_in_cache(sqlite3 **db, std::string username, std::string password, int sess_dur) {
  bool success = true;
  // prepare SQL statement
  sqlite3_stmt *stmt;
  if(prep_stmt(db, &stmt, CHECK_FOR_USER_SQL)) {
    std::clog << kLogInfo << "Statement prepared" << std::endl;
    // add binds
    if(!add_text_bind(stmt, 1, username, "username")) {
      success = false;
    }
    if(!add_text_bind(stmt, 2, password, "password")) {
      success = false;
    }
    int current_ts = (int)time(NULL);
    //current_ts -= sess_dur;
    if(!add_int_bind(stmt, 3, current_ts - sess_dur, "timestamp")) {
      success = false;
    }
    std::clog << kLogInfo << "Binds added" << std::endl;
    // run statement
    if(success) {
      if(run_stmt(stmt, SQLITE_ROW)) {
        success = true;
      } else {
        success = false;
      }
    } 
  }
  // free the statement as we finished with it
  sqlite3_finalize(stmt);
  return success;
}

bool Cache::save_in_cache(std::string username, std::string password) {
  bool success = false;
  sqlite3 *db;
  if(check_and_init(directory, realm, &db)) {
    std::clog << kLogInfo << "Cache open" << std::endl;
    success = save_user_in_cache(&db, username, password);
    sqlite3_close(db);
  } else {
    std::clog << kLogWarning << "Could not open cache, look at previous log messages for hints" << std::endl;
    success = false;
  }
  return success;
}

bool Cache::check_cache(std::string username, std::string password) {
  bool success = false;
  sqlite3 *db;
  if(check_and_init(directory, realm, &db)) {
    std::clog << kLogInfo << "Cache open" << std::endl;
    // check if user is in cache
    success = check_for_user_in_cache(&db, username, password, session_duration);
    sqlite3_close(db);
  } else {
    std::clog << kLogWarning << "Could not open cache, look at previous log messages for hints" << std::endl;
    success = false;
  }
  return success;
}