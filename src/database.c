#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "error.h"

static sqlite3 *db = NULL;

int db_init(const char *db_path) {
    int rc;
    
    if (db != NULL) {
        printf("Database already initialized.\n");
        return ERR_OK;
    }

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = NULL;
        return ERR_DB_CONNECTION;
    }

    // Create books table
    const char *sql_books = 
        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "author TEXT NOT NULL,"
        "is_borrowed INTEGER DEFAULT 0"
        ");";

    rc = sqlite3_exec(db, sql_books, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to create books table: %s\n", sqlite3_errmsg(db));
        db_close();
        return ERR_DB_QUERY;
    }

    // Create users table
    const char *sql_users = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL"
        ");";

    rc = sqlite3_exec(db, sql_users, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to create users table: %s\n", sqlite3_errmsg(db));
        db_close();
        return ERR_DB_QUERY;
    }

    // Create borrowed_books table (junction table for user-book borrowing)
    const char *sql_borrowed = 
        "CREATE TABLE IF NOT EXISTS borrowed_books ("
        "user_id INTEGER,"
        "book_id INTEGER,"
        "FOREIGN KEY(user_id) REFERENCES users(id),"
        "FOREIGN KEY(book_id) REFERENCES books(id),"
        "PRIMARY KEY(user_id, book_id)"
        ");";

    rc = sqlite3_exec(db, sql_borrowed, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to create borrowed_books table: %s\n", sqlite3_errmsg(db));
        db_close();
        return ERR_DB_QUERY;
    }

    printf("Database initialized successfully.\n");
    return ERR_OK;
}

void db_close(void) {
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
        printf("Database closed.\n");
    }
}

sqlite3* db_get_connection(void) {
    return db;
}

int db_execute(const char *sql) {
    if (db == NULL) {
        printf("Database not initialized.\n");
        return ERR_DB_CONNECTION;
    }

    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return ERR_DB_QUERY;
    }

    return ERR_OK;
}

int db_is_initialized(void) {
    return (db != NULL) ? 1 : 0;
}
