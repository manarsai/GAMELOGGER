#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class GamesController : public HttpController<GamesController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(GamesController::getGames, "/api/games", Get);
    ADD_METHOD_TO(GamesController::getGame, "/api/games/{1}", Get);
    METHOD_LIST_END

        void getGames(const HttpRequestPtr& req,
            std::function<void(const HttpResponsePtr&)>&& callback);

    void getGame(const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback,
        std::string id);
};