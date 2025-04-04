#include <iostream>
#include "api.cpp"
#include "render.cpp"

using namespace std;
using runes = vector<int>;

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
    int d;
    int l;
    int r;
};

bool operator<(const dw &a, const dw &b) {
    if (a.d == b.d) {
        if (a.l == b.l) {
            return a.r < b.r;
        }
        return a.l < b.l;
    }
    return a.d < b.d;
}
bool operator==(const dw &a, const dw &b) {
    return a.d == b.d && a.l == b.l && a.r == b.r;
}

vector<word> words;
map<pair<int, int>, vector<int>> wp;
map<pair<int, pair<int, int>>, vector<int>> dw;

int main() {
    simInit();
    auto status = words_wrapper();
    while (true) {
        words.clear();
        wp.clear();
        dw.clear();
        if (status.usedWords.size() >= 800 ) {
            break;
        }
//        for(int i = 0; i < status.words.size(); i++) {
//            cout << status.words[i] << endl;
//        }
//        return 0;
        int idx = 0;
        for (const auto &w: status.words) {
            bool skip = false;
            for (const auto used: status.usedWords) {
                if (used == idx) {
                    idx++;
                    skip = true;
                    break;
                }
            }
            if (skip) {
                continue;
            }
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
//                    cout << r[i] << ' ' << r[i + d] << endl;
                    dw[{d, {r[i], r[i + d]}}].push_back(idx);
                }
            }
            idx++;
        }
        double best = -1;
        int fbase, fl, fr;
        vector<pair<int, int>> fts;
        int ffloor = 0, ffl, ffr;
        int st_clock = clock();
        for(int base_idx = 0; base_idx < words.size(); base_idx++) {
            auto base = words[base_idx];
            bool found = false;
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
                            vector<pair<int, int> > floors;
                            int h = min(lw.r.size(), rw.r.size());
                            double score = 1.25 + 1.25 * base.r.size();
                            for(int floor = min(lw.r.size(), rw.r.size()) - 1; floor > 1; floor--) {
                                vector<int> pos = dw[{r - l, {lw.r[lw.r.size() - 1 - floor], rw.r[rw.r.size() - 1 - floor]}}];
                                for(int fi = 0; fi < pos.size(); fi++) {
                                    if (pos[fi] == lidx || pos[fi] == ridx || pos[fi] == base_idx) continue;
                                    floors.push_back({floor, pos[fi]});
                                    score += (floor + 1) * (r - l + 1);
                                    floor--;
                                    break;
                                }
                            }
                            if (floors.size() > 0) {
                                found = true;
                            }
                            if (score > best && floors.size() > 0) {
                                fbase = base_idx;
                                best = score;
                                fl = lidx;
                                fr = ridx;
                                ffl = l;
                                ffr = r;
                                fts = floors;
                                break;
                            }
                        }
                        if (found) {
                            break;
                        }
                    }
                }
            }
        }
        cout << ' ' << (clock() - st_clock) * 1. / CLOCKS_PER_SEC << endl;
        if (best == -1) {
            cout << "NOT FOUND?!" << endl;
        }
        vector<PositionedWord> pw;
        cout << words[fl].s << ' ' << words[fr].s << endl;
        cout << words[fbase].s << endl;
        cout << ffl << ' ' << ffr << endl;
        auto lw = words[fl];
        auto rw = words[fr];
        int d = ffr - ffl;
        int xoffset = 0;
        vector<int> flpos;
        for (const auto &[floor, ft]: fts) {
            auto &top = words[ft];
            int lc = lw.r[lw.r.size() - 1 - floor];
            int rc = rw.r[rw.r.size() - 1 - floor];
            for (int i = 0; i + d < top.r.size(); i++) {
                if (top.r[i] == lc && top.r[i + d] == rc) {
                    if (ffr + i >= 30) {
                        flpos.push_back(-1);
                        continue;
                    }
                    xoffset = max(xoffset, i);
                    flpos.push_back(i);
                    break;
                }
            }
        }
        cout << fts.size() << ' ' << flpos.size() << endl;
        pw.push_back(PositionedWord{
            .id = words[fbase].idx,
            .dir = 2,
            .pos = {xoffset, 0, 0},
        });
        pw.push_back(PositionedWord{
            .id = words[fl].idx,
            .dir = 1,
            .pos = {xoffset + ffl, 0, static_cast<int>(words[fl].r.size()) - 1}
        });
        pw.push_back(PositionedWord{
            .id = words[fr].idx,
            .dir = 1,
            .pos = {xoffset + ffr, 0, static_cast<int>(words[fr].r.size()) - 1}
        });
        for(int i = 0; i < fts.size(); i++) {
            if (flpos[i] == -1) {
                continue;
            }
            auto &top = words[fts[i].second];
            if (xoffset + ffl - flpos[i] + top.r.size() + 1 >= 30 || xoffset + ffl - flpos[i] >= 30) {
                continue;
            }
            pw.push_back(PositionedWord{
                    .id = top.idx,
                    .dir = 2,
                    .pos = {xoffset + ffl - flpos[i], 0, fts[i].first}
            });
        }
        build_wrapper(BuildRequest{
            .done = true,
            .words = pw
        });
        Status st2 = words_wrapper();
//        sleep(st2.nextTurnSec + 3);
        status = words_wrapper();
    }
}
