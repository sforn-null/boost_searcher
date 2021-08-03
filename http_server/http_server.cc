#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "cpphttplib/httplib.h"
#include "../searcher/searcher.h"

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;

int main() {
    using namespace httplib;
    // 1. 创建 Searcher 对象
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if (!ret) {
        std::cout << "Searcher 初始化失败" << std::endl;
        return 1;
    }

    // 2. 创建 server 对象
    Server server;
    server.Get("/searcher", [&searcher](const Request& req, Response& resp) {
        if (!req.has_param("query")) {
            resp.set_content("您发的请求参数错误", "text/plain");
            return;
        }
        string query = req.get_param_value("query");
        string results;
        searcher.Search(query, &results);
        resp.set_content(results, "application/json");
    });
    // 告诉服务器, 静态资源存放在 wwwroot 目录下. (html, css, js, 图片....)
    server.set_mount_point("/", "./ShySearcher");
    // 3. 启动服务器
    server.listen("0.0.0.0", 10001);
    return 0;
}
