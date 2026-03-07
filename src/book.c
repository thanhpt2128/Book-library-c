#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "book.h"
#include "database.h"
#include "error.h"

// Static storage for returning Book pointers
static Book cached_book;

int book_add(const char *title, const char *author) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO books (title, author, is_borrowed) VALUES ('%s', '%s', 0);",
             title, author);

    int rc = db_execute(sql);
    if (rc != ERR_OK) return -1;

    // Return the last inserted ID
    int id = (int)sqlite3_last_insert_rowid(db);
    return id - 1;  //  0-based index for compatibility
}

int book_edit(int id, const char *new_title, const char *new_author) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    Book *book = book_find_by_id(id);
    if (!book) return -1;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE books SET title='%s', author='%s' WHERE id=%d;",
             new_title, new_author, id);

    return db_execute(sql);
}

int book_delete(int id) {
    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM books WHERE id=%d;", id);

    return db_execute(sql);
}

Book* book_find_by_id(int id) {
    sqlite3 *db = db_get_connection();
    if (!db) return NULL;

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id, title, author, is_borrowed FROM books WHERE id=%d;", id);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return NULL;
    }

    Book *result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cached_book.id = sqlite3_column_int(stmt, 0);
        strncpy(cached_book.title, (const char*)sqlite3_column_text(stmt, 1), MAX_TITLE_LEN - 1);
        cached_book.title[MAX_TITLE_LEN - 1] = '\0';
        strncpy(cached_book.author, (const char*)sqlite3_column_text(stmt, 2), MAX_AUTHOR_LEN - 1);
        cached_book.author[MAX_AUTHOR_LEN - 1] = '\0';
        cached_book.is_borrowed = sqlite3_column_int(stmt, 3);
        result = &cached_book;
    }

    sqlite3_finalize(stmt);
    return result;
}

Book* book_find_by_title(const char *title) {
    sqlite3 *db = db_get_connection();
    if (!db) return NULL;

    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT id, title, author, is_borrowed FROM books WHERE title='%s' LIMIT 1;", title);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return NULL;
    }

    Book *result = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cached_book.id = sqlite3_column_int(stmt, 0);
        strncpy(cached_book.title, (const char*)sqlite3_column_text(stmt, 1), MAX_TITLE_LEN - 1);
        cached_book.title[MAX_TITLE_LEN - 1] = '\0';
        strncpy(cached_book.author, (const char*)sqlite3_column_text(stmt, 2), MAX_AUTHOR_LEN - 1);
        cached_book.author[MAX_AUTHOR_LEN - 1] = '\0';
        cached_book.is_borrowed = sqlite3_column_int(stmt, 3);
        result = &cached_book;
    }

    sqlite3_finalize(stmt);
    return result;
}

void book_list_available(void) {
    sqlite3 *db = db_get_connection();
    if (!db) {
        printf("Database not initialized.\n");
        return;
    }

    const char *sql = "SELECT id, title, author FROM books WHERE is_borrowed=0;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to fetch books: %s\n", sqlite3_errmsg(db));
        return;
    }

    printf("Available Books:\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *title = (const char*)sqlite3_column_text(stmt, 1);
        const char *author = (const char*)sqlite3_column_text(stmt, 2);
        printf("ID: %d | Title: %s | Author: %s\n", id, title, author);
    }

    sqlite3_finalize(stmt);
}
