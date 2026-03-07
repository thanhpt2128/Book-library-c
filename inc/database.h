#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

// Initialize database connection and create tables
int db_init(const char *db_path);

// Close database connection
void db_close(void);

// Get the database connection (for use by other modules)
sqlite3* db_get_connection(void);

// Execute a non-query SQL statement (INSERT, UPDATE, DELETE)
int db_execute(const char *sql);

// Check if database is initialized
int db_is_initialized(void);

#endif // DATABASE_H
