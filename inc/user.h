#ifndef USER_H
#define USER_H

#include "config.h"
#include "book.h"

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int borrowed_books[MAX_BORROWED_BOOKS];
    int borrowed_count;
} User;

int user_add(const char *name);
int user_edit(int id, const char *new_name);
int user_delete(int id);
User* user_find_by_id(int id);
void user_print_info(int id);

#endif // USER_H
