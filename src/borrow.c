#include <stdio.h>
#include <sqlite3.h>
#include "borrow.h"
#include "user.h"
#include "book.h"
#include "error.h"
#include "database.h"

int borrow_book(int user_id, int book_id) {
    User *user = user_find_by_id(user_id);
    Book *book = book_find_by_id(book_id);

    if (!user) {
        printf("Borrow failed: user ID %d not found.\n", user_id);
        return ERR_USER_NOT_FOUND;
    }

    if (!book) {
        printf("Borrow failed: book ID %d not found.\n", book_id);
        return ERR_BOOK_NOT_FOUND;
    }

    if (book->is_borrowed) {
        printf("Borrow failed: book ID %d is already borrowed.\n", book_id);
        return ERR_BOOK_ALREADY_BORROWED;
    }

    if (user->borrowed_count >= MAX_BORROWED_BOOKS) {
        printf("Borrow failed: user has reached borrowing limit.\n");
        return ERR_USER_BORROW_LIMIT;
    }

    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    // Insert into borrowed_books table
    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT INTO borrowed_books (user_id, book_id) VALUES (%d, %d);",
             user_id, book_id);
    
    int rc = db_execute(sql);
    if (rc != ERR_OK) {
        printf("Borrow failed: database error.\n");
        return rc;
    }

    // Update book status to borrowed
    snprintf(sql, sizeof(sql), "UPDATE books SET is_borrowed=1 WHERE id=%d;", book_id);
    rc = db_execute(sql);
    if (rc != ERR_OK) {
        // Rollback the borrowed_books insert
        snprintf(sql, sizeof(sql),
                 "DELETE FROM borrowed_books WHERE user_id=%d AND book_id=%d;",
                 user_id, book_id);
        db_execute(sql);
        printf("Borrow failed: database error.\n");
        return rc;
    }

    printf("Borrow successful: User %d borrowed Book %d.\n", user_id, book_id);
    return ERR_OK;
}

int return_book(int user_id, int book_id) {
    User *user = user_find_by_id(user_id);
    Book *book = book_find_by_id(book_id);

    if (!user) {
        printf("Return failed: user ID %d not found.\n", user_id);
        return ERR_USER_NOT_FOUND;
    }

    if (!book) {
        printf("Return failed: book ID %d not found.\n", book_id);
        return ERR_BOOK_NOT_FOUND;
    }

    // Check if user has borrowed this book
    int found = 0;
    for (int i = 0; i < user->borrowed_count; ++i) {
        if (user->borrowed_books[i] == book_id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Return failed: book ID %d not borrowed by user ID %d.\n", book_id, user_id);
        return ERR_BOOK_NOT_BORROWED_BY_USER;
    }

    sqlite3 *db = db_get_connection();
    if (!db) return ERR_DB_CONNECTION;

    // Delete from borrowed_books table
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM borrowed_books WHERE user_id=%d AND book_id=%d;",
             user_id, book_id);
    
    int rc = db_execute(sql);
    if (rc != ERR_OK) {
        printf("Return failed: database error.\n");
        return rc;
    }

    // Update book status to available
    snprintf(sql, sizeof(sql), "UPDATE books SET is_borrowed=0 WHERE id=%d;", book_id);
    rc = db_execute(sql);
    if (rc != ERR_OK) {
        printf("Return failed: database error.\n");
        return rc;
    }

    printf("Return successful: User %d returned Book %d.\n", user_id, book_id);
    return ERR_OK;
}
