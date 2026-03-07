#include "crow.h"
#include <string>

extern "C" {
    #include "book.h"
    #include "user.h"
    #include "borrow.h"
    #include "search.h"
    #include "error.h"
    #include "database.h"
}

int main() {
    crow::SimpleApp app;

    // Initialize database
    int db_status = db_init("library.db");
    if (db_status != ERR_OK) {
        CROW_LOG_ERROR << "Failed to initialize database: " << get_error_msg(db_status);
        return 1;
    }

    // Root endpoint - serve HTML interface
    CROW_ROUTE(app, "/")
    ([](){
        auto response = crow::response();
        response.set_static_file_info("index.html");
        return response;
    });

    // Get all available books
    CROW_ROUTE(app, "/api/books/available")
    .methods("GET"_method)
    ([](const crow::request& req){
        sqlite3 *db = db_get_connection();
        if (!db) {
            crow::json::wvalue response;
            response["error"] = "Database not initialized";
            return crow::response(500, response);
        }

        const char *sql = "SELECT id, title, author FROM books WHERE is_borrowed=0;";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        
        crow::json::wvalue response;
        std::vector<crow::json::wvalue> books;
        
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue book;
                book["id"] = sqlite3_column_int(stmt, 0);
                book["title"] = (const char*)sqlite3_column_text(stmt, 1);
                book["author"] = (const char*)sqlite3_column_text(stmt, 2);
                book["is_borrowed"] = 0;
                books.push_back(std::move(book));
            }
        }
        sqlite3_finalize(stmt);
        
        response["books"] = std::move(books);
        response["count"] = (int)books.size();
        return crow::response(200, response);
    });

    // List all books
    CROW_ROUTE(app, "/api/books")
    .methods("GET"_method)
    ([](){
        sqlite3 *db = db_get_connection();
        if (!db) {
            crow::json::wvalue response;
            response["error"] = "Database not initialized";
            return crow::response(500, response);
        }

        const char *sql = "SELECT id, title, author, is_borrowed FROM books;";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        
        crow::json::wvalue response;
        std::vector<crow::json::wvalue> books;
        
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue book;
                book["id"] = sqlite3_column_int(stmt, 0);
                book["title"] = (const char*)sqlite3_column_text(stmt, 1);
                book["author"] = (const char*)sqlite3_column_text(stmt, 2);
                book["is_borrowed"] = sqlite3_column_int(stmt, 3);
                books.push_back(std::move(book));
            }
        }
        sqlite3_finalize(stmt);
        
        response["books"] = std::move(books);
        response["count"] = (int)books.size();
        return crow::response(200, response);
    });

    // Add new book
    CROW_ROUTE(app, "/api/books")
    .methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string title = body["title"].s();
        std::string author = body["author"].s();

        int id = book_add(title.c_str(), author.c_str());
        
        crow::json::wvalue response;
        if (id >= 0) {
            response["success"] = true;
            response["book_id"] = id;
            response["message"] = "Book added successfully";
            return crow::response(201, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_MAX_BOOKS_REACHED);
            return crow::response(400, response);
        }
    });

    // List all users
    CROW_ROUTE(app, "/api/users")
    .methods("GET"_method)
    ([](){
        sqlite3 *db = db_get_connection();
        if (!db) {
            crow::json::wvalue response;
            response["error"] = "Database not initialized";
            return crow::response(500, response);
        }

        const char *sql = "SELECT id, name FROM users;";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        
        crow::json::wvalue response;
        std::vector<crow::json::wvalue> users;
        
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue user;
                user["id"] = sqlite3_column_int(stmt, 0);
                user["name"] = (const char*)sqlite3_column_text(stmt, 1);
                users.push_back(std::move(user));
            }
        }
        sqlite3_finalize(stmt);
        
        response["users"] = std::move(users);
        response["count"] = (int)users.size();
        return crow::response(200, response);
    });

    // Add new user
    CROW_ROUTE(app, "/api/users")
    .methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string name = body["name"].s();
        int id = user_add(name.c_str());
        
        crow::json::wvalue response;
        if (id >= 0) {
            response["success"] = true;
            response["user_id"] = id;
            response["message"] = "User added successfully";
            return crow::response(201, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_MAX_USERS_REACHED);
            return crow::response(400, response);
        }
    });

    // Borrow book
    CROW_ROUTE(app, "/api/borrow")
    .methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        int user_id = body["user_id"].i();
        int book_id = body["book_id"].i();

        int result = borrow_book(user_id, book_id);
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "Book borrowed successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(result);
            return crow::response(400, response);
        }
    });

    // Return book
    CROW_ROUTE(app, "/api/return")
    .methods("POST"_method)
    ([](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        int user_id = body["user_id"].i();
        int book_id = body["book_id"].i();

        int result = return_book(user_id, book_id);
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "Book returned successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(result);
            return crow::response(400, response);
        }
    });

    // Get user info
    CROW_ROUTE(app, "/api/users/<int>")
    .methods("GET"_method)
    ([](int user_id){
        User* user = user_find_by_id(user_id);
        
        crow::json::wvalue response;
        if (user) {
            response["id"] = user->id;
            response["name"] = user->name;
            response["borrowed_count"] = user->borrowed_count;
            
            // Add borrowed books array
            std::vector<crow::json::wvalue> borrowed_books;
            for (int i = 0; i < user->borrowed_count; i++) {
                crow::json::wvalue book;
                book["book_id"] = user->borrowed_books[i];
                borrowed_books.push_back(std::move(book));
            }
            response["borrowed_books"] = std::move(borrowed_books);
            
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_USER_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Edit user
    CROW_ROUTE(app, "/api/users/<int>")
    .methods("PUT"_method)
    ([](const crow::request& req, int user_id){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string name = body["name"].s();
        int result = user_edit(user_id, name.c_str());
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "User updated successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_USER_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Delete user
    CROW_ROUTE(app, "/api/users/<int>")
    .methods("DELETE"_method)
    ([](int user_id){
        int result = user_delete(user_id);
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "User deleted successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_USER_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Get book info
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("GET"_method)
    ([](int book_id){
        Book* book = book_find_by_id(book_id);
        
        crow::json::wvalue response;
        if (book) {
            response["id"] = book->id;
            response["title"] = book->title;
            response["author"] = book->author;
            response["is_borrowed"] = book->is_borrowed;
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_BOOK_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Edit book
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("PUT"_method)
    ([](const crow::request& req, int book_id){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string title = body["title"].s();
        std::string author = body["author"].s();

        int result = book_edit(book_id, title.c_str(), author.c_str());
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "Book updated successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_BOOK_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Delete book
    CROW_ROUTE(app, "/api/books/<int>")
    .methods("DELETE"_method)
    ([](int book_id){
        int result = book_delete(book_id);
        
        crow::json::wvalue response;
        if (result == ERR_OK) {
            response["success"] = true;
            response["message"] = "Book deleted successfully";
            return crow::response(200, response);
        } else {
            response["success"] = false;
            response["message"] = get_error_msg(ERR_BOOK_NOT_FOUND);
            return crow::response(404, response);
        }
    });

    // Search books by title or author
    CROW_ROUTE(app, "/api/search/books")
    .methods("GET"_method)
    ([](const crow::request& req){
        auto title = req.url_params.get("title");
        auto author = req.url_params.get("author");
        
        crow::json::wvalue response;
        
        if (!title && !author) {
            response["error"] = "Missing 'title' or 'author' parameter";
            return crow::response(400, response);
        }

        sqlite3 *db = db_get_connection();
        if (!db) {
            response["error"] = "Database not initialized";
            return crow::response(500, response);
        }

        char sql[512];
        if (title) {
            snprintf(sql, sizeof(sql),
                     "SELECT id, title, author, is_borrowed FROM books WHERE title LIKE '%%%s%%';",
                     title);
            response["search_by"] = "title";
            response["keyword"] = title;
        } else {
            snprintf(sql, sizeof(sql),
                     "SELECT id, title, author, is_borrowed FROM books WHERE author LIKE '%%%s%%';",
                     author);
            response["search_by"] = "author";
            response["keyword"] = author;
        }

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        
        std::vector<crow::json::wvalue> books;
        
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                crow::json::wvalue book;
                book["id"] = sqlite3_column_int(stmt, 0);
                book["title"] = (const char*)sqlite3_column_text(stmt, 1);
                book["author"] = (const char*)sqlite3_column_text(stmt, 2);
                book["is_borrowed"] = sqlite3_column_int(stmt, 3);
                books.push_back(std::move(book));
            }
        }
        sqlite3_finalize(stmt);
        
        response["results"] = std::move(books);
        response["count"] = (int)books.size();
        return crow::response(200, response);
    });

    CROW_LOG_INFO << "Starting Library Management Web API on port 8080...";
    app.port(8080).multithreaded().run();

    // Cleanup
    db_close();
    return 0;
}
