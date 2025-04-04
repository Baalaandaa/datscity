#include <iostream>
#include "api.cpp"
#include "utils.cpp"

using namespace std;
using runes = vector<int>;
using idxs = vector<int>;

struct word{
    runes r;
    int idx;
    int turn;
    string s;
};

bool cmp(word l, word r) {
    return l.r.size() > r.r.size();
}

struct dw {
    int d, l, r;
};

bool operator<(const dw &a, const dw &b) {
    return a.d < b.d;
}

int main() {
    auto status = words_wrapper();
    while (true) {
        vector<vector<vector<idxs>>> wrld(status.mapSize[0], vector<vector<idxs>>(status.mapSize[1], vector<idxs> (status.mapSize[2])));
        vector<word> words;
        map<pair<int, int>, vector<int>> wp;
        map<dw, vector<int>> dw;
        int idx = 0;
        int turn = status.turn;
        for (const auto &w: status.words) {
            runes r = from_utf8(w);
            words.push_back({
                r, idx, status.turn, w
            });
            idx++;
        }

        sort(words.begin(), words.end(), cmp);

        idx = 0;
        for(const auto &w: words) {
            auto &r = w.r;
            for(int i = 0; i < r.size(); i++) {
                wp[{r.size() - i - 1, r[i]}].push_back(idx);
            }
            for(int d = 1; d < r.size(); d++) {
                for(int i = 0; i + d < r.size(); i++) {
                    dw[{d, r[i], r[i + d]}].push_back(idx);
                }
            }
            idx++;
        }
        bool found = false;
        int fbase, fl, fr, ft;
        int ffloor = 0, ffl, ffr;
        for(int base_idx = 0; base_idx < words.size(); base_idx++) {
            auto base = words[base_idx];
            for(int l = 0; l < base.r.size(); l++) {
                for(int r = base.r.size() - 2; r >= l + 2; r--) {
                    vector<int> lpos = wp[{0, base.r[l]}];
                    vector<int> rpos = wp[{0, base.r[r]}];
                    for(const auto &lidx: lpos) {
                        for(const auto &ridx: rpos) {
                            if (lidx == ridx || base_idx == lidx || base_idx == ridx) {
                                continue;
                            }
                            auto lw = words[lidx];
                            auto rw = words[ridx];
                            for(int floor = min(lw.r.size(), rw.r.size()) - 1; floor > 0; floor--) {
                                vector<int> pos = dw[{r - l + 1, lw.r[lw.r.size() - 1 - floor], rw.r[rw.r.size() - 1 - floor]}];
                                for(int fi = 0; fi < pos.size(); fi++) {
                                    if (pos[fi] == lidx || pos[fi] == ridx || pos[fi] == base_idx) continue;
                                    found = true;
                                    fbase = base_idx;
                                    ffloor = floor;
                                    fl = lidx;
                                    fr = ridx;
                                    ft = pos[fi];
                                    ffl = l;
                                    ffr = r;
                                    break;
                                }
                                if (found) break;
                            }
                            if (found) break;
                        }
                        if (found) break;
                    }
                    if (found) break;
                }
                if (found) break;
            }
            if (found) break;
        }

        vector<PositionedWord> pw;
        cout << words[fl].s << ' ' << words[fr].s << endl;
        cout << words[fbase].s << ' ' << words[ft].s << endl;
        cout << ffl << ' ' << ffr << endl;
        int flpos = -1;
        int lc = words[fl].r[ffloor];
        int rc = words[fr].r[ffloor];
        int d = ffr - ffl + 1;
        auto &top = words[ft];
        for(int i = 0; i + d < top.r.size(); i++) {
            if (top.r[i] == lc && top.r[i + d] == rc) {
                flpos = i;
                break;
            }
        }
        int xoffset = max(0, flpos - ffl);
        pw.push_back(PositionedWord{
            .id = words[fbase].idx,
            .dir = 2,
            .pos = {xoffset, 0, 0},
        });
        pw.push_back(PositionedWord{
            .id = words[fl].idx,
            .dir = 1,
            .pos = {xoffset + ffl, 0, static_cast<int>(words[fl].r.size())}
        });
        pw.push_back(PositionedWord{
            .id = words[fr].idx,
            .dir = 1,
            .pos = {xoffset + ffr, 0, static_cast<int>(words[fr].r.size())}
        });
        pw.push_back(PositionedWord{
            .id = top.idx,
            .dir = 2,
            .pos = {xoffset + ffl - flpos, 0, ffloor}
        });
        build(BuildRequest{
            .done = true,
            .words = pw
        });
        Status st2 = words_wrapper();
        sleep(st2.nextTurnSec);
        status = words_wrapper();
    }
}
