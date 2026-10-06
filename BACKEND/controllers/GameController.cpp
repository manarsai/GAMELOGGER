#include "GamesController.h"
#include <drogon/drogon.h>

using namespace drogon;

void GamesController::getGames(
    const HttpRequestPtr&,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto client = HttpClient::newHttpClient("https://www.freetogame.com");

    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Get);
    req->setPath("/api/games");

    client->sendRequest(req, [callback](ReqResult result,
        const HttpResponsePtr& resp) {
            if (result != ReqResult::Ok)
            {
                callback(HttpResponse::newHttpJsonResponse(
                    Json::Value("Failed to fetch games")));
                return;
            }

            callback(resp);
        });
}

void GamesController::getGame(
    const HttpRequestPtr&,
    std::function<void(const HttpResponsePtr&)>&& callback,
    std::string id)
{
    auto client = HttpClient::newHttpClient("https://www.freetogame.com");

    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Get);
    req->setPath("/api/game?id=" + id);

    client->sendRequest(req, [callback](ReqResult result,
        const HttpResponsePtr& resp) {
            if (result != ReqResult::Ok)
            {
                callback(HttpResponse::newHttpJsonResponse(
                    Json::Value("Failed to fetch game")));
                return;
            }

            callback(resp);
        });
}