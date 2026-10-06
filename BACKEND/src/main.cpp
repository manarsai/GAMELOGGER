#include <drogon/drogon.h>
#include <sqlite3.h>
#include <openssl/sha.h>

#include <iostream>
#include <mutex>
#include <string>
#include <iomanip>
#include <sstream>

std::mutex dbMutex;
sqlite3* db = nullptr;

std::string hashPassword(const std::string& password)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256(
        reinterpret_cast<const unsigned char*>(password.c_str()),
        password.length(),
        hash
    );

    std::stringstream ss;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(hash[i]);
    }

    return ss.str();
}


// CORS 

void addCorsHeaders(
    const drogon::HttpResponsePtr& response)
{
    response->addHeader(
        "Access-Control-Allow-Origin",
        "http://localhost:5173"
    );

    response->addHeader(
        "Access-Control-Allow-Methods",
        "GET, POST, PUT, DELETE, OPTIONS"
    );

    response->addHeader(
        "Access-Control-Allow-Headers",
        "Content-Type, Authorization"
    );

    response->addHeader(
        "Access-Control-Max-Age",
        "86400"
    );
}

void registerCors()
{
    // Handle options  before routing
    drogon::app().registerPreRoutingAdvice(
        [](const drogon::HttpRequestPtr& req,
            drogon::AdviceCallback&& callback,
            drogon::AdviceChainCallback&& chain)
        {
            if (req->method() == drogon::Options)
            {
                auto resp = drogon::HttpResponse::newHttpResponse();

                resp->setStatusCode(drogon::k204NoContent);
                resp->addHeader("Access-Control-Allow-Origin", "http://localhost:5173");
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                resp->addHeader("Access-Control-Max-Age", "86400");

                callback(resp);   // Return the response immediately
                return;
            }

            chain();
        });

    // Add CORS headers to every normal response
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&,
            const drogon::HttpResponsePtr& resp)
        {
            resp->addHeader("Access-Control-Allow-Origin", "http://localhost:5173");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            resp->addHeader("Access-Control-Max-Age", "86400");
        });
}

// sqlite

bool initializeDatabase()
{
    int result = sqlite3_open(
        "gamelog.db",
        &db
    );

    if (result != SQLITE_OK)
    {
        std::cerr
            << "Failed to open database: "
            << sqlite3_errmsg(db)
            << std::endl;

        return false;
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            email TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS games (
            id INTEGER,
            user_id INTEGER NOT NULL,
            title TEXT NOT NULL,
            platform TEXT,
            status TEXT,
            rating INTEGER DEFAULT 0,
            hours REAL DEFAULT 0,
            thumbnail TEXT,
            genre TEXT,
            publisher TEXT,
            release_date TEXT,
            PRIMARY KEY (id, user_id)
        );
    )";

    char* errorMessage = nullptr;

    result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK)
    {
        std::cerr
            << "Failed to create database tables: "
            << errorMessage
            << std::endl;

        sqlite3_free(errorMessage);

        return false;
    }

    std::cout
        << "SQLite database ready."
        << std::endl;

    return true;
}

// account

void registerUpdateUserEndpoint()
{
    drogon::app().registerHandler(
        "/api/users/{1}",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;
                error["error"] = "Invalid JSON";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::lock_guard<std::mutex> lock(dbMutex);

            std::string username = (*json)["username"].asString();
            std::string email = (*json)["email"].asString();

            const char* sql = R"(
                UPDATE users
                SET username=?, email=?
                WHERE id=?;
            )";

            sqlite3_stmt* statement = nullptr;

            sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

            sqlite3_bind_text(statement, 1, username.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(statement, 2, email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(statement, 3, std::stoi(id));

            int result = sqlite3_step(statement);
            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                Json::Value error;
                error["error"] = sqlite3_errmsg(db);

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                addCorsHeaders(response);
                callback(response);
                return;
            }

            Json::Value success;
            success["status"] = "updated";

            auto response = drogon::HttpResponse::newHttpJsonResponse(success);
            addCorsHeaders(response);
            callback(response);
        },
        { drogon::Put }
    );
}

// change passowrd 

void registerChangePasswordEndpoint()
{
    drogon::app().registerHandler(
        "/api/users/{1}/password",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;
                error["error"] = "Invalid JSON";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::string password = (*json)["password"].asString();

            if (password.length() < 6)
            {
                Json::Value error;
                error["error"] = "Password must be at least 6 characters";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::string hash = hashPassword(password);

            std::lock_guard<std::mutex> lock(dbMutex);

            const char* sql = "UPDATE users SET password_hash=? WHERE id=?;";

            sqlite3_stmt* statement = nullptr;

            sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

            sqlite3_bind_text(statement, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(statement, 2, std::stoi(id));

            sqlite3_step(statement);
            sqlite3_finalize(statement);

            Json::Value success;
            success["status"] = "password_updated";

            auto response = drogon::HttpResponse::newHttpJsonResponse(success);
            addCorsHeaders(response);
            callback(response);
        },
        { drogon::Post }
    );
}

// delete account 

void registerDeleteUserEndpoint()
{
    drogon::app().registerHandler(
        "/api/users/{1}",
        [](const drogon::HttpRequestPtr&,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            std::lock_guard<std::mutex> lock(dbMutex);

            sqlite3_stmt* statement = nullptr;

            sqlite3_prepare_v2(db,
                "DELETE FROM games WHERE user_id=?;",
                -1,
                &statement,
                nullptr);

            sqlite3_bind_int(statement, 1, std::stoi(id));
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            sqlite3_prepare_v2(db,
                "DELETE FROM users WHERE id=?;",
                -1,
                &statement,
                nullptr);

            sqlite3_bind_int(statement, 1, std::stoi(id));
            sqlite3_step(statement);
            sqlite3_finalize(statement);

            Json::Value success;
            success["status"] = "deleted";

            auto response = drogon::HttpResponse::newHttpJsonResponse(success);
            addCorsHeaders(response);
            callback(response);
        },
        { drogon::Delete }
    );
}

// root

void registerRootEndpoint()
{
    drogon::app().registerHandler(
        "/",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto response =
                drogon::HttpResponse::newHttpResponse();

            response->setBody(
                "GameLogger Backend is running!"
            );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get }
    );
}


// health

void registerHealthEndpoint()
{
    drogon::app().registerHandler(
        "/api/health",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value json;

            json["status"] = "ok";
            json["service"] = "GameLoggerBackend";
            json["database"] = "SQLite";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    json
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get }
    );
}

//dashboard

void registerDashboardEndpoint()
{
    drogon::app().registerHandler(
        "/api/dashboard",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            std::string userId = request->getParameter("user_id");

            if (userId.empty())
            {
                Json::Value error;
                error["error"] = "user_id is required";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }

            std::lock_guard<std::mutex> lock(dbMutex);

            Json::Value stats;

            sqlite3_stmt* statement = nullptr;

            const char* sql = R"(
                SELECT
                    COUNT(*),
                    SUM(CASE WHEN status='Playing' THEN 1 ELSE 0 END),
                    SUM(CASE WHEN status='Completed' THEN 1 ELSE 0 END),
                    SUM(CASE WHEN status='Want to Play' THEN 1 ELSE 0 END),
                    COALESCE(SUM(hours),0)
                FROM games
                WHERE user_id=?;
            )";

            sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);
            sqlite3_bind_int(statement, 1, std::stoi(userId));

            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                stats["total"] =
                    sqlite3_column_int(statement, 0);

                stats["playing"] =
                    sqlite3_column_int(statement, 1);

                stats["completed"] =
                    sqlite3_column_int(statement, 2);

                stats["want_to_play"] =
                    sqlite3_column_int(statement, 3);

                stats["hours"] =
                    sqlite3_column_double(statement, 4);
            }

            sqlite3_finalize(statement);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(stats);

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get });
}


// =get api/games api 

void registerFreeToGameEndpoint()
{
    drogon::app().registerHandler(
        "/api/games",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto client =
                drogon::HttpClient::newHttpClient(
                    "https://www.freetogame.com"
                );

            auto request =
                drogon::HttpRequest::newHttpRequest();

            request->setMethod(
                drogon::Get
            );

            request->setPath(
                "/api/games"
            );

            client->sendRequest(
                request,
                [callback](
                    drogon::ReqResult result,
                    const drogon::HttpResponsePtr& response)
                {
                    if (result != drogon::ReqResult::Ok ||
                        !response)
                    {
                        Json::Value json;

                        json["error"] =
                            "Failed to fetch games from FreeToGame";

                        auto errorResponse =
                            drogon::HttpResponse::newHttpJsonResponse(
                                json
                            );

                        errorResponse->setStatusCode(
                            drogon::k502BadGateway
                        );

                        addCorsHeaders(
                            errorResponse
                        );

                        callback(
                            errorResponse
                        );

                        return;
                    }

                    auto proxyResponse =
                        drogon::HttpResponse::newHttpResponse();

                    proxyResponse->setStatusCode(
                        response->getStatusCode()
                    );

                    proxyResponse->setBody(
                        std::string(
                            response->getBody()
                        )
                    );

                    proxyResponse->setContentTypeCode(
                        drogon::CT_APPLICATION_JSON
                    );

                    addCorsHeaders(
                        proxyResponse
                    );

                    callback(
                        proxyResponse
                    );
                }
            );
        },
        { drogon::Get }
    );
}


// api details
// GET /api/games/{id}

void registerFreeToGameDetailsEndpoint()
{
    drogon::app().registerHandler(
        "/api/games/{1}",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            auto client =
                drogon::HttpClient::newHttpClient(
                    "https://www.freetogame.com"
                );

            auto request =
                drogon::HttpRequest::newHttpRequest();

            request->setMethod(
                drogon::Get
            );

            request->setPath(
                "/api/game?id=" + id
            );

            client->sendRequest(
                request,
                [callback](
                    drogon::ReqResult result,
                    const drogon::HttpResponsePtr& response)
                {
                    if (result != drogon::ReqResult::Ok ||
                        !response)
                    {
                        Json::Value json;

                        json["error"] =
                            "Failed to fetch game from FreeToGame";

                        auto errorResponse =
                            drogon::HttpResponse::newHttpJsonResponse(
                                json
                            );

                        errorResponse->setStatusCode(
                            drogon::k502BadGateway
                        );

                        addCorsHeaders(
                            errorResponse
                        );

                        callback(
                            errorResponse
                        );

                        return;
                    }

                    auto proxyResponse =
                        drogon::HttpResponse::newHttpResponse();

                    proxyResponse->setStatusCode(
                        response->getStatusCode()
                    );

                    proxyResponse->setBody(
                        std::string(
                            response->getBody()
                        )
                    );

                    proxyResponse->setContentTypeCode(
                        drogon::CT_APPLICATION_JSON
                    );

                    addCorsHeaders(
                        proxyResponse
                    );

                    callback(
                        proxyResponse
                    );
                }
            );
        },
        { drogon::Get }
    );
}


// sqlite library
// GET /api/library


void registerGetLibraryEndpoint()
{
    drogon::app().registerHandler(
        "/api/library",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            // Get user_id from URL
            std::string userId =
                request->getParameter("user_id");

            if (userId.empty())
            {
                Json::Value error;

                error["error"] =
                    "user_id is required";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            std::lock_guard<std::mutex> lock(
                dbMutex
            );

            Json::Value games(
                Json::arrayValue
            );

            const char* sql = R"(
                SELECT
                    id,
                    title,
                    platform,
                    status,
                    rating,
                    hours,
                    thumbnail,
                    genre,
                    publisher,
                    release_date
                FROM games
                WHERE user_id = ?
                ORDER BY id DESC;
            )";

            sqlite3_stmt* statement = nullptr;

            int result =
                sqlite3_prepare_v2(
                    db,
                    sql,
                    -1,
                    &statement,
                    nullptr
                );

            if (result != SQLITE_OK)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            sqlite3_bind_int(
                statement,
                1,
                std::stoi(userId)
            );

            while (
                sqlite3_step(statement)
                == SQLITE_ROW
                )
            {
                Json::Value game;

                game["id"] =
                    sqlite3_column_int(
                        statement,
                        0
                    );

                game["title"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            1
                        )
                        );

                game["platform"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            2
                        )
                        );

                game["status"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            3
                        )
                        );

                game["rating"] =
                    sqlite3_column_int(
                        statement,
                        4
                    );

                game["hours"] =
                    sqlite3_column_double(
                        statement,
                        5
                    );

                game["thumbnail"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            6
                        )
                        );

                game["genre"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            7
                        )
                        );

                game["publisher"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            8
                        )
                        );

                game["release_date"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(
                            statement,
                            9
                        )
                        );

                games.append(game);
            }

            sqlite3_finalize(statement);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    games
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get }
    );
}

// add to user library
// POST /api/library

void registerAddLibraryEndpoint()
{
    drogon::app().registerHandler(
        "/api/library",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json =
                request->getJsonObject();

            if (!json)
            {
                Json::Value errorJson;

                errorJson["error"] =
                    "Invalid JSON";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        errorJson
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(
                    response
                );

                callback(
                    response
                );

                return;
            }

            std::lock_guard<std::mutex> lock(
                dbMutex
            );

            const char* sql = R"(
    INSERT OR REPLACE INTO games (
        id,
        user_id,
        title,
        platform,
        status,
        rating,
        hours,
        thumbnail,
        genre,
        publisher,
        release_date
    )
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
)";

            sqlite3_stmt* statement =
                nullptr;

            int prepareResult =
                sqlite3_prepare_v2(
                    db,
                    sql,
                    -1,
                    &statement,
                    nullptr
                );

            if (prepareResult != SQLITE_OK)
            {
                Json::Value jsonError;

                jsonError["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        jsonError
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(
                    response
                );

                callback(
                    response
                );

                return;
            }

            sqlite3_bind_int(
                statement,
                1,
                (*json)["id"].asInt()
            );

            sqlite3_bind_int(
                statement,
                2,
                (*json)["user_id"].asInt()
            );

            std::string title =
                (*json)["title"].asString();

            std::string platform =
                (*json)["platform"].asString();

            std::string status =
                (*json)["status"].asString();

            std::string thumbnail =
                (*json)["thumbnail"].asString();

            std::string genre =
                (*json)["genre"].asString();

            std::string publisher =
                (*json)["publisher"].asString();

            std::string releaseDate =
                (*json)["release_date"].asString();

            sqlite3_bind_text(
                statement,
                3,
                title.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                4,
                platform.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                5,
                status.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_int(
                statement,
                6,
                (*json)["rating"].asInt()
            );

            sqlite3_bind_double(
                statement,
                7,
                (*json)["hours"].asDouble()
            );

            sqlite3_bind_text(
                statement,
                8,
                thumbnail.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                9,
                genre.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                10,
                publisher.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                11,
                releaseDate.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            int result =
                sqlite3_step(
                    statement
                );

            sqlite3_finalize(
                statement
            );

            if (result != SQLITE_DONE)
            {
                Json::Value jsonError;

                jsonError["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        jsonError
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(
                    response
                );

                callback(
                    response
                );

                return;
            }

            Json::Value jsonResponse;

            jsonResponse["status"] =
                "added";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    jsonResponse
                );

            response->setStatusCode(
                drogon::k201Created
            );

            addCorsHeaders(
                response
            );

            callback(
                response
            );
        },
        { drogon::Post }
    );
}

// update library
// PUT /api/library/{id}

void registerUpdateLibraryEndpoint()
{
    drogon::app().registerHandler(
        "/api/library/{1}",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;
                error["error"] = "Invalid JSON";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::lock_guard<std::mutex> lock(dbMutex);

            const char* sql = R"(
                UPDATE games
                SET
                    title = ?,
                    platform = ?,
                    status = ?,
                    rating = ?,
                    hours = ?,
                    thumbnail = ?,
                    genre = ?,
                    publisher = ?,
                    release_date = ?
                WHERE id = ?;
            )";

            sqlite3_stmt* statement = nullptr;

            int result = sqlite3_prepare_v2(
                db,
                sql,
                -1,
                &statement,
                nullptr
            );

            if (result != SQLITE_OK)
            {
                Json::Value error;
                error["error"] = sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            sqlite3_bind_text(
                statement,
                1,
                (*json)["title"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                2,
                (*json)["platform"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                3,
                (*json)["status"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_int(
                statement,
                4,
                (*json)["rating"].asInt()
            );

            sqlite3_bind_double(
                statement,
                5,
                (*json)["hours"].asDouble()
            );

            sqlite3_bind_text(
                statement,
                6,
                (*json)["thumbnail"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                7,
                (*json)["genre"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                8,
                (*json)["publisher"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                9,
                (*json)["release_date"].asString().c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_int(
                statement,
                10,
                std::stoi(id)
            );

            result = sqlite3_step(statement);

            sqlite3_finalize(statement);

            if (result != SQLITE_DONE)
            {
                Json::Value error;
                error["error"] = sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            Json::Value responseJson;
            responseJson["status"] = "updated";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Put }
    );
}



// delete from library
// DELETE /api/library/{id}


void registerDeleteLibraryEndpoint()
{
    drogon::app().registerHandler(
        "/api/library/{1}",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            std::lock_guard<std::mutex> lock(
                dbMutex
            );

            const char* sql =
                "DELETE FROM games WHERE id = ?;";

            sqlite3_stmt* statement =
                nullptr;

            int prepareResult =
                sqlite3_prepare_v2(
                    db,
                    sql,
                    -1,
                    &statement,
                    nullptr
                );

            if (prepareResult != SQLITE_OK)
            {
                Json::Value json;

                json["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        json
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(
                    response
                );

                callback(
                    response
                );

                return;
            }

            sqlite3_bind_int(
                statement,
                1,
                std::stoi(id)
            );

            int result =
                sqlite3_step(
                    statement
                );

            sqlite3_finalize(
                statement
            );

            Json::Value json;

            if (result == SQLITE_DONE)
            {
                json["status"] =
                    "deleted";
            }
            else
            {
                json["error"] =
                    sqlite3_errmsg(db);
            }

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    json
                );

            addCorsHeaders(
                response
            );

            callback(
                response
            );
        },
        { drogon::Delete }
    );
}



// register
// POST /api/auth/register

void registerRegisterEndpoint()
{
    drogon::app().registerHandler(
        "/api/auth/register",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;

                error["error"] = "Invalid JSON";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            std::string username =
                (*json)["username"].asString();

            std::string email =
                (*json)["email"].asString();

            std::string password =
                (*json)["password"].asString();

            // validation

            if (username.empty() ||
                email.empty() ||
                password.empty())
            {
                Json::Value error;

                error["error"] =
                    "Username, email and password are required";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            if (password.length() < 6)
            {
                Json::Value error;

                error["error"] =
                    "Password must be at least 6 characters";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            // hash password

            std::string passwordHash =
                hashPassword(password);

            // database
            std::lock_guard<std::mutex> lock(
                dbMutex
            );

            const char* sql = R"(
                INSERT INTO users (
                    username,
                    email,
                    password_hash
                )
                VALUES (?, ?, ?);
            )";

            sqlite3_stmt* statement = nullptr;

            int result =
                sqlite3_prepare_v2(
                    db,
                    sql,
                    -1,
                    &statement,
                    nullptr
                );

            if (result != SQLITE_OK)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            sqlite3_bind_text(
                statement,
                1,
                username.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                2,
                email.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            sqlite3_bind_text(
                statement,
                3,
                passwordHash.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            result =
                sqlite3_step(statement);

            sqlite3_finalize(statement);

            // duplicate user
            if (result == SQLITE_CONSTRAINT)
            {
                Json::Value error;

                error["error"] =
                    "Username or email already exists";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k409Conflict
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            // other db error

            if (result != SQLITE_DONE)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            // success

            Json::Value responseJson;

            responseJson["status"] =
                "registered";

            responseJson["username"] =
                username;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson
                );

            response->setStatusCode(
                drogon::k201Created
            );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Post }
    );
}


// LOGIN
// POST /api/auth/login


void registerLoginEndpoint()
{
    drogon::app().registerHandler(
        "/api/auth/login",
        [](const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;
                error["error"] = "Invalid JSON";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::string email =
                (*json)["email"].asString();

            std::string password =
                (*json)["password"].asString();

            if (email.empty() || password.empty())
            {
                Json::Value error;

                error["error"] =
                    "Email and password are required";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k400BadRequest
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            std::string passwordHash =
                hashPassword(password);

            std::lock_guard<std::mutex> lock(
                dbMutex
            );

            const char* sql = R"(
                SELECT
                    id,
                    username,
                    email,
                    password_hash
                FROM users
                WHERE email = ?;
            )";

            sqlite3_stmt* statement = nullptr;

            int result =
                sqlite3_prepare_v2(
                    db,
                    sql,
                    -1,
                    &statement,
                    nullptr
                );

            if (result != SQLITE_OK)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            sqlite3_bind_text(
                statement,
                1,
                email.c_str(),
                -1,
                SQLITE_TRANSIENT
            );

            result =
                sqlite3_step(statement);

            if (result != SQLITE_ROW)
            {
                sqlite3_finalize(statement);

                Json::Value error;

                error["error"] =
                    "Invalid email or password";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k401Unauthorized
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            int userId =
                sqlite3_column_int(
                    statement,
                    0
                );

            std::string username =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(
                        statement,
                        1
                    )
                    );

            std::string storedEmail =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(
                        statement,
                        2
                    )
                    );

            std::string storedPasswordHash =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(
                        statement,
                        3
                    )
                    );

            sqlite3_finalize(statement);

            if (passwordHash != storedPasswordHash)
            {
                Json::Value error;

                error["error"] =
                    "Invalid email or password";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k401Unauthorized
                );

                addCorsHeaders(response);
                callback(response);
                return;
            }

            Json::Value user;

            user["id"] = userId;
            user["username"] = username;
            user["email"] = storedEmail;

            Json::Value responseJson;

            responseJson["status"] =
                "logged_in";

            responseJson["user"] =
                user;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Post }
    );
}


// GET ALL USERS
// GET /api/users


void registerGetUsersEndpoint()
{
    drogon::app().registerHandler(
        "/api/users",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            std::lock_guard<std::mutex> lock(dbMutex);

            Json::Value users(Json::arrayValue);

            const char* sql = R"(
                SELECT id, username, email, created_at
                FROM users
                ORDER BY id DESC;
            )";

            sqlite3_stmt* statement = nullptr;

            int result = sqlite3_prepare_v2(
                db,
                sql,
                -1,
                &statement,
                nullptr
            );

            if (result != SQLITE_OK)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                Json::Value user;

                user["id"] =
                    sqlite3_column_int(statement, 0);

                user["username"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(statement, 1)
                        );

                user["email"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(statement, 2)
                        );

                user["created_at"] =
                    reinterpret_cast<const char*>(
                        sqlite3_column_text(statement, 3)
                        );

                users.append(user);
            }

            sqlite3_finalize(statement);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    users
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get }
    );
}


// GET USER BY ID
// GET /api/users/{id}


void registerGetUserEndpoint()
{
    drogon::app().registerHandler(
        "/api/users/{1}",
        [](const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback,
            const std::string& id)
        {
            std::lock_guard<std::mutex> lock(dbMutex);

            const char* sql = R"(
                SELECT id, username, email, created_at
                FROM users
                WHERE id = ?;
            )";

            sqlite3_stmt* statement = nullptr;

            int result = sqlite3_prepare_v2(
                db,
                sql,
                -1,
                &statement,
                nullptr
            );

            if (result != SQLITE_OK)
            {
                Json::Value error;

                error["error"] =
                    sqlite3_errmsg(db);

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k500InternalServerError
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            sqlite3_bind_int(
                statement,
                1,
                std::stoi(id)
            );

            result = sqlite3_step(statement);

            if (result != SQLITE_ROW)
            {
                sqlite3_finalize(statement);

                Json::Value error;

                error["error"] = "User not found";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        error
                    );

                response->setStatusCode(
                    drogon::k404NotFound
                );

                addCorsHeaders(response);

                callback(response);
                return;
            }

            Json::Value user;

            user["id"] =
                sqlite3_column_int(statement, 0);

            user["username"] =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 1)
                    );

            user["email"] =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 2)
                    );

            user["created_at"] =
                reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 3)
                    );

            sqlite3_finalize(statement);

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    user
                );

            addCorsHeaders(response);

            callback(response);
        },
        { drogon::Get }
    );
}

//void registerGlobalOptionsEndpoint()
//{
//    drogon::app().registerHandler(
//        "/{1}",
//        [](const drogon::HttpRequestPtr& request,
//            std::function<void(
//                const drogon::HttpResponsePtr&)>&& callback,
//            const std::string&)
//        {
//            auto response =
//                drogon::HttpResponse::newHttpResponse();
//
//            response->setStatusCode(
//                drogon::k204NoContent
//            );
//
//            addCorsHeaders(response);
//
//            callback(response);
//        },
//        { drogon::Options }
//    );
//}


// MAIN


int main()
{
    if (!initializeDatabase())
        return 1;

    registerCors();

    registerUpdateUserEndpoint();
    registerChangePasswordEndpoint();
    registerDeleteUserEndpoint();

    registerRootEndpoint();
    registerHealthEndpoint();

    registerRegisterEndpoint();
    registerLoginEndpoint();
    registerGetUsersEndpoint();
    registerGetUserEndpoint();

    registerFreeToGameEndpoint();
    registerFreeToGameDetailsEndpoint();

    registerDashboardEndpoint();

    registerGetLibraryEndpoint();
    registerAddLibraryEndpoint();
    registerUpdateLibraryEndpoint();
    registerDeleteLibraryEndpoint();

    drogon::app()
        .addListener("0.0.0.0", 8080)
        .run();

    sqlite3_close(db);
}