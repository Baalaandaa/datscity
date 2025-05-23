#pragma once
#include <vector>
#include <map>
#include "json.hpp"

using namespace std;

using json = nlohmann::json;

struct Status {
    vector<int> mapSize;
    int nextTurnSec;
    string roundEndsAt;
    int shuffleLeft;
    int turn;
    vector<int> usedWords;
    vector<string> words;
};

struct PositionedWord {
    int id;
    int dir;
    vector<int> pos; // x,y,z
};

struct BuildRequest {
    bool done;
    vector<PositionedWord> words;
};

struct DoneTower {
    int id;
    double score;
};

struct Tower {
    double score;
    vector<string> words;
};

struct BuildResponse {
    double score;
    vector<DoneTower> doneTowers;
    Tower tower;
};

struct ShuffleResponse {
    int shuffleLeft;
    vector<string> words;
};

void from_json(const json &j, DoneTower &t) {
    j.at("id").get_to(t.id);
    j.at("score").get_to(t.score);
}

void from_json(const json &j, Tower &t) {
    j.at("score").get_to(t.score);
    j.at("words").get_to(t.words);
}

void from_json(const json &j, BuildResponse &b) {
    j.at("score").get_to(b.score);
    j.at("doneTowers").get_to(b.doneTowers);
    j.at("tower").get_to(b.tower);
}

void from_json(const json &j, Status &s) {
    j.at("mapSize").get_to(s.mapSize);
    j.at("nextTurnSec").get_to(s.nextTurnSec);
    j.at("roundEndsAt").get_to(s.roundEndsAt);
    j.at("shuffleLeft").get_to(s.shuffleLeft);
    j.at("turn").get_to(s.turn);
    j.at("usedIndexes").get_to(s.usedWords);
    j.at("words").get_to(s.words);
}

void to_json(json &j, const PositionedWord &w) {
    j = json{
            {"id", w.id},
            {"dir", w.dir},
            {"pos", w.pos}
    };
}

void to_json(json &j, const BuildRequest &r) {
    j = json{
            {"done", r.done},
            {"words", r.words}
    };
}

void from_json(const json &j, ShuffleResponse &sr) {
    j.at("shuffleLeft").get_to(sr.shuffleLeft);
    j.at("words").get_to(sr.words);
}