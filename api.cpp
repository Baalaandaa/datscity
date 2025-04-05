#include "http.hpp"
#include "model.cpp"
#include "utils.cpp"
using namespace std;

httplib::Client cli("http://games-test.datsteam.dev");

const string token = "2f97738c-ddab-4f24-8186-552c4620389c";

vector<string> sim_words;
vector<int> sim_used(0);
int sim_turn = 0;
double total_score = 0;

Status simWords() {
    return Status{
        .mapSize = {30, 30, 100},
        .nextTurnSec = 300,
        .roundEndsAt = "unused",
        .shuffleLeft = 3,
        .turn = sim_turn,
        .usedWords = sim_used,
        .words = sim_words,
    };
}

void simInit() {
    ifstream fin("words.txt");
    string s;
    vector<string> dict;
    while(getline(fin, s)) {
        if (!s.empty()) {
            dict.push_back(s);
        }
    }
    for(int i = 0; i < 1000; i++) {
        sim_words.push_back(dict[2 * i + rand() % 2]);
    }
}


mutex mu;

struct Cube {
    float x, y, z;
    float r, g, b;
};

vector<Cube> cubes;


float randomColor() {
    return (rand() % 80 + 100) / 255.0f; // Random value between 0.39 and 1.0 (avoids too dark colors)
}

using idxs = vector<int>;
double calc(vector<int> mapSize, vector<PositionedWord> words, vector<int> rlen) {
    vector<vector<vector<idxs>>> wrld(mapSize[2], vector<vector<idxs>>(mapSize[0], vector<idxs> (mapSize[1])));
    map<int, int> dens;
    double score = 0;
    mu.lock();
    cubes.clear();
    float r, g, b;
    for (int i = 0; i < words.size(); i++) {
        if (words[i].dir == 2 || words[i].dir == 3) {
            dens[words[i].pos[2]] += 1;
        }
        r = randomColor(), g = randomColor(), b = randomColor();
        int x = words[i].pos[0], y = words[i].pos[1], z = words[i].pos[2], d = words[i].dir;
//        cout << x << ' ' << y << ' ' << z << ' ' << d << ' ' << rlen[i] << endl;
        for(int p = 0; p < rlen[i]; p++) {
            if (x < 0 || y < 0 || x >= mapSize[0] || y >= mapSize[1]) {
                cout << "SIZE MISMATCH " << x << ' ' << y << ' ' << z << ' ' << words[i].id << ' ' << words[i].pos[0] << endl;
            }
            wrld[z][x][y].push_back(words[i].id);
            cubes.push_back({
                (float)x, (float)z, (float)y,
                    r, g, b
            });
            if (d == 1) {
                z--;
            } else if (d == 2) {
                x++;
            } else {
                y++;
            }
        }
    }
    mu.unlock();
    for(int z = 0; z < wrld.size(); z++) {
        int min_x = 1e9, min_y = 1e9, max_x=0, max_y=0, cnt = 0;
        for(int x = 0; x < wrld[z].size(); x++) {
            for(int y = 0; y < wrld[z].size(); y++) {
                if (!wrld[z][x][y].empty()) {
                    min_x = min(x, min_x);
                    max_x = max(x, max_x);
                    min_y = min(y, min_y);
                    max_y = max(y, max_y);
                    cnt++;
                }
            }
        }
        if (cnt == 0) continue;
        int dx = max_x - min_x + 1;
        int dy = max_y - min_y + 1;
        cout << "z: " << z << " | cnt: " << cnt << " | / factor: " << (min(dx, dy) / (double)max(dx, dy)) << " | density: " << (1 + (double)dens[z] / 4) << " | TOTAL= " << cnt * (z + 1) * (min(dx, dy) / (double)max(dx, dy)) * (1 + (double)dens[z] / 4) << endl;
        score += cnt * (z + 1) * (min(dx, dy) / (double)max(dx, dy)) * (1 + (double)dens[z] / 4);
    }
    return score;
}

vector<DoneTower> sim_done_towers;

BuildResponse simBuild(BuildRequest &req) {
    vector<int> rlen;
    for(const auto &w: req.words) {
        auto r = from_utf8(sim_words[w.id]);
        rlen.push_back(r.size());
    }
    for(const auto &w: req.words) {
        sim_used.push_back(w.id);
    }
    double score = calc({30, 30, 100}, req.words, rlen);
    cout << sim_turn << ' ' << score << endl;
    total_score += score;
    sim_done_towers.push_back({static_cast<int>(sim_done_towers.size()), score});
    sim_turn++;
    if (sim_turn == 27) {
        cout << "TOTAL: " << total_score << endl;
        exit(0);
    }
    return BuildResponse{
        total_score, sim_done_towers, Tower{}
    };
}

Status api_words() {
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
                          retValue = simWords();
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


BuildResponse build(const BuildRequest &t) {
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
        BuildResponse resp(json::parse(res->body));
        return resp;
    }
    catch (const std::exception& e)
    {
        std::cout << "Request failed, error: " << e.what() << endl;
        throw e;
    }
}

BuildResponse build_wrapper(BuildRequest req)
{
    std::mutex m;
    std::condition_variable cv;
    BuildResponse retValue;

    std::thread t([&cv, &retValue, &req]()
                  {
                      try {
                          retValue = simBuild(req);
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