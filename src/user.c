#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "user.h"
#include "database.h"
#include "error.h"

// Static storage for returning User pointers
static User cached_user;

int user_add(const char *name) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    char sql[512];
    snprintf(sql, sizeof(sql), "INSERT INTO users (name) VALUES ('%s');", name);

    int rc = db_execute(sql);
    if (rc != ERR_OK) return -1;

    // Return the last inserted ID
    int id = (int)sqlite3_last_insert_rowid(db);
    return id - 1;  
}

int user_edit(int id, const char *new_name) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    // Check if user exists first
    User *user = user_find_by_id(id);
    if (!user) return -1;

    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET name='%s' WHERE id=%d;", new_name, id);

    return db_execute(sql);
}

int user_delete(int id) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    char sql[512];
    
    // First, update all borrowed books to set is_borrowed=0
    snprintf(sql, sizeof(sql), 
             "UPDATE books SET is_borrowed=0 WHERE id IN (SELECT book_id FROM borrowed_books WHERE user_id=%d);", 
             id);
    db_execute(sql);
    
    // Then delete all borrowed books records
    snprintf(sql, sizeof(sql), "DELETE FROM borrowed_books WHERE user_id=%d;", id);
    db_execute(sql);

    // Finally delete the user
    snprintf(sql, sizeof(sql), "DELETE FROM users WHERE id=%d;", id);
    return db_execute(sql);
}

User* user_find_by_id(int id) {
    sqlite3 *db = db_get_connection();
    if (!db) return NULL;

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id, name FROM users WHERE id=%d;", id);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return NULL;
    }

    User *result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cached_user.id = sqlite3_column_int(stmt, 0);
        strncpy(cached_user.name, (const char*)sqlite3_column_text(stmt, 1), MAX_NAME_LEN - 1);
        cached_user.name[MAX_NAME_LEN - 1] = '\0';

        // Get borrowed books count and list
        sqlite3_finalize(stmt);

        char sql_borrowed[256];
        snprintf(sql_borrowed, sizeof(sql_borrowed),
                 "SELECT book_id FROM borrowed_books WHERE user_id=%d;", id);

        rc = sqlite3_prepare_v2(db, sql_borrowed, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            cached_user.borrowed_count = 0;
            while (sqlite3_step(stmt) == SQLITE_ROW && cached_user.borrowed_count < MAX_BORROWED_BOOKS) {
                cached_user.borrowed_books[cached_user.borrowed_count++] = sqlite3_column_int(stmt, 0);
            }
        }
        result = &cached_user;
    }

    sqlite3_finalize(stmt);
    return result;
}

void user_print_info(int id) {
    User *user = user_find_by_id(id);
    if (!user) {
        printf("User ID %d not found.\n", id);
        return;
    }

    printf("User ID: %d | Name: %s\n", user->id, user->name);
    printf("Borrowed Books (%d):\n", user->borrowed_count);
    for (int i = 0; i < user->borrowed_count; ++i) {
        Book *b = book_find_by_id(user->borrowed_books[i]);
        if (b) {
            printf(" - [%d] %s by %s\n", b->id, b->title, b->author);
        }
    }
}
