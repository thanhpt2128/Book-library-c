#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "book.h"
#include "user.h"
#include "database.h"

void search_books_by_title(const char *title) {
    sqlite3 *db = db_get_connection();
    if (!db) {
        printf("Database not initialized.\n");
        return;
    }

    printf("Search Results (Title: %s):\n", title);

    // Use LIKE for partial matching
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, title, author, is_borrowed FROM books WHERE title LIKE '%%%s%%';",
             title);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to search books: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *book_title = (const char*)sqlite3_column_text(stmt, 1);
        const char *author = (const char*)sqlite3_column_text(stmt, 2);
        int is_borrowed = sqlite3_column_int(stmt, 3);
        
        printf("ID: %d | Title: %s | Author: %s | %s\n",
               id, book_title, author, is_borrowed ? "Borrowed" : "Available");
    }

    sqlite3_finalize(stmt);
}

void search_books_by_author(const char *author) {
    sqlite3 *db = db_get_connection();
    if (!db) {
        printf("Database not initialized.\n");
        return;
    }

    printf("Search Results (Author: %s):\n", author);

    // Use LIKE for partial matching
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, title, author, is_borrowed FROM books WHERE author LIKE '%%%s%%';",
             author);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to search books: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *title = (const char*)sqlite3_column_text(stmt, 1);
        const char *book_author = (const char*)sqlite3_column_text(stmt, 2);
        int is_borrowed = sqlite3_column_int(stmt, 3);
        
        printf("ID: %d | Title: %s | Author: %s | %s\n",
               id, title, book_author, is_borrowed ? "Borrowed" : "Available");
    }

    sqlite3_finalize(stmt);
}

void list_all_books(void) {
    sqlite3 *db = db_get_connection();
    if (!db) {
        printf("Database not initialized.\n");
        return;
    }

    printf("All Books in Library:\n");

    const char *sql = "SELECT id, title, author, is_borrowed FROM books;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to list books: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *title = (const char*)sqlite3_column_text(stmt, 1);
        const char *author = (const char*)sqlite3_column_text(stmt, 2);
        int is_borrowed = sqlite3_column_int(stmt, 3);
        
        printf("ID: %d | Title: %s | Author: %s | %s\n",
               id, title, author, is_borrowed ? "Borrowed" : "Available");
    }

    sqlite3_finalize(stmt);
}

void list_all_users(void) {
    sqlite3 *db = db_get_connection();
    if (!db) {
        printf("Database not initialized.\n");
        return;
    }

    printf("Registered Users:\n");

    const char *sql = 
        "SELECT u.id, u.name, COUNT(b.book_id) as borrowed_count "
        "FROM users u "
        "LEFT JOIN borrowed_books b ON u.id = b.user_id "
        "GROUP BY u.id, u.name;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("Failed to list users: %s\n", sqlite3_errmsg(db));
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char*)sqlite3_column_text(stmt, 1);
        int borrowed_count = sqlite3_column_int(stmt, 2);
        
        printf("ID: %d | Name: %s | Borrowed: %d books\n", id, name, borrowed_count);
    }

    sqlite3_finalize(stmt);
}
