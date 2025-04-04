#include "http.hpp"
#include "model.cpp"
using namespace std;

httplib::Client cli("http://games-test.datsteam.dev");

const string token = "2f97738c-ddab-4f24-8186-552c4620389c";

Status words() {
    try
    {
        auto headers = httplib::Headers{
                {"X-Auth-Token", token}
        };
        auto res = cli.Get("/api/words", headers);
        if (res == nullptr) {
            throw std::exception();
        }
        Status resp(json::parse(res->body));
        return resp;
    }
    catch (const std::exception& e)
    {
        std::cout << "Request failed, error: " << e.what() << endl;
        throw e;
    }
}

Status words_wrapper(){
    std::mutex m;
    std::condition_variable cv;
    Status retValue;

    std::thread t([&cv, &retValue]()
                  {
                      try {
                          retValue = words();
                      } catch (exception e) {
                          return;
                      }
                      cv.notify_one();
                  });

    t.detach();

    {
        std::unique_lock<std::mutex> l(m);
        if(cv.wait_for(l, 5s) == std::cv_status::timeout) {
            throw std::runtime_error("Timeout words");
        }
    }

    return retValue;
}

ShuffleResponse shuffle() {
    try
    {
        auto headers = httplib::Headers{
                {"X-Auth-Token", token}
        };
        auto res = cli.Post("/api/shuffle", headers);
        if (res == nullptr) {
            throw std::exception();
        }
        ShuffleResponse resp(json::parse(res->body));
        return resp;
    }
    catch (const std::exception& e)
    {
        std::cout << "Request failed, error: " << e.what() << endl;
        throw e;
    }
}


ShuffleResponse shuffle_wrapper(){
    std::mutex m;
    std::condition_variable cv;
    ShuffleResponse retValue;

    std::thread t([&cv, &retValue]()
                  {
                      try {
                          retValue = shuffle();
                      } catch (exception e) {
                          return;
                      }
                      cv.notify_one();
                  });

    t.detach();

    {
        std::unique_lock<std::mutex> l(m);
        if(cv.wait_for(l, 1s) == std::cv_status::timeout) {
            throw std::runtime_error("Timeout");
        }
    }

    return retValue;
}


ShuffleResponse build(const BuildRequest &t) {
    try
    {
        json jdata(t);
        string body = jdata.dump();
        auto headers = httplib::Headers{
                {"X-Auth-Token", token}
        };
        auto res = cli.Post("/api/build", headers, body.c_str(), body.size(), "application/json");
        if (res == nullptr) {
            throw std::exception();
        }
        cout << res->body << endl;
        ShuffleResponse resp(json::parse(res->body));
        return resp;
    }
    catch (const std::exception& e)
    {
        std::cout << "Request failed, error: " << e.what() << endl;
        throw e;
    }
}

ShuffleResponse build_wrapper(BuildRequest req)
{
    std::mutex m;
    std::condition_variable cv;
    ShuffleResponse retValue;

    std::thread t([&cv, &retValue, &req]()
                  {
                      try {
                          retValue = build(req);
                      } catch (exception e) {
                          return;
                      }
                      cv.notify_one();
                  });

    t.detach();

    {
        std::unique_lock<std::mutex> l(m);
        if(cv.wait_for(l, 1s) == std::cv_status::timeout) {
            throw std::runtime_error("Timeout");
        }
    }

    return retValue;
}