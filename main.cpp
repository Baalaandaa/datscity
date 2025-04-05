#include <iostream>
#include "api.cpp"
#include "render.cpp"

#include <SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

using namespace std;
using runes = vector<int>;

const int zEnrichLimit = 9;
const int yEnrichLimit = 9;
const int enrichCnt = 3;

struct word {
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

vector<PositionedWord> findLadder(vector<string> w, vector<int> used, vector<int> mapSize, int base_char = -1,
                                  int base_pos = 0, int xMin = 0, int xMax = 0, int prev_base = -1, int lx_old = -1,
                                  int rx_old = -1) {
    vector<word> words;
    map<pair<int, int>, vector<int> > wp;
    map<pair<int, pair<int, int> >, vector<pair<int, int> > > dw;
    map<int, int> base_id;
    int idx = 0;
    for (const auto &w: w) {
        bool skip = false;
        for (const auto u: used) {
            if (u == idx) {
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
            r, idx, 0, w
        });
        idx++;
    }
    sort(words.begin(), words.end(), cmp);
    idx = 0;
    for (const auto &w: words) {
        auto &r = w.r;
        for (int i = 0; i < r.size(); i++) {
            wp[{r.size() - i - 1, r[i]}].push_back(idx);
        }
        for (int d = 1; d < r.size(); d++) {
            for (int i = 0; i + d < r.size(); i++) {
                dw[{d, {r[i], r[i + d]}}].push_back({idx, i});
            }
        }
        base_id[words[idx].idx] = idx;

        idx++;
    }
    double best_score = -1;
    int f_base_idx = 0, f_base_x;
    int flx, frx;
    int flidx, fridx;
    int fbidx;
    vector<pair<int, pair<int, int> > > ftops;
    vector<thread> threads;
    mutex sol;

    for (int base_idx = (prev_base == -1 ? 0 : base_id[prev_base]); base_idx < (prev_base == -1
                                                                        ? min(25, (int) words.size())
                                                                        : base_id[prev_base] + 1); base_idx++) {
        threads.emplace_back([&, base_idx]() {
            int base_x = 0;
            if (base_char != -1) {
                base_x = -1;
                for (int i = 0; i < words[base_idx].r.size(); i++) {
                    if (words[base_idx].r[i] == base_char) {
                        base_x = i;
                    }
                }
                if (base_x == -1) {
                    return;
                }
            }
            int xMinB = base_pos - base_x;
            int xmx = max(xMax, xMinB + (int) words[base_idx].r.size());
            int xmn = min(xMin, xMinB);
            if (xmx - xmn + 1 >= mapSize[0]) {
                return;
            }
            const auto &base = words[base_idx];
            for (int d = 2; d < base.r.size(); d++) {
                bool found = false;
                for (int lx = 0; lx + d < base.r.size(); lx++) {
                    int rx = lx + d;
                    if (lx == lx_old || lx == rx_old || rx == lx_old || rx == rx_old) {
                        continue;
                    }

                    vector<int> lpos = wp[{0, base.r[lx]}];
                    vector<int> rpos = wp[{0, base.r[rx]}];
                    for (const int lidx: lpos) {
                        for (const int ridx: rpos) {
                            if (lidx == base_idx || ridx == base_idx || lidx == ridx) continue;
                            vector<pair<int, pair<int, int> > > tops;
                            double score = 1.25;
                            const auto &l = words[lidx];
                            const auto &r = words[ridx];
                            int fstart = (int) min(l.r.size(), r.r.size());
                            int fend = (int) max(l.r.size(), r.r.size());
                            //                            score += (fstart + 1 + fend) * (fend - fstart) / 2;
                            for (int floor = fstart - 1; floor > 1; floor--) {
                                vector<pair<int, int> > possible_tops = dw[{
                                    d, {l.r[l.r.size() - 1 - floor], r.r[r.r.size() - 1 - floor]}
                                }];
                                int min_delta = mapSize[0], mti = -1;
                                for (int ti = 0; ti < possible_tops.size(); ti++) {
                                    int ix = possible_tops[ti].first;
                                    if (ix == lidx || ix == ridx || ix == base_idx) {
                                        continue;
                                    }
                                    bool coll = false;
                                    for (int i = 0; i < tops.size(); i++) {
                                        if (tops[i].second.first == ix) {
                                            coll = true;
                                        }
                                    }
                                    if (coll) {
                                        continue;
                                    }
                                    const auto &top = words[possible_tops[ti].first];
                                    int idx = possible_tops[ti].second;
                                    int len = (int) top.r.size();
                                    int xl = min(xmn, base_pos - base_x + lx - idx);
                                    int xr = max(xmx, base_pos - base_x + lx - idx + len);
                                    if (xr - xl + 1 >= mapSize[0]) {
                                        continue;
                                    }
                                    if (xr - xl < min_delta) {
                                        min_delta = xr - xl;
                                        mti = ti;
                                    }
                                }
                                if (mti == -1) {
                                    score += 2. / (rx - lx) * floor;
                                    continue;
                                }
                                score += 1.25 * floor;
                                tops.emplace_back(floor, possible_tops[mti]);
                                floor--;
                            }
                            if (tops.empty()) continue;
                            sol.lock();
                            if (score > best_score) {
                                best_score = score;
                                found = true;
                                ftops = tops;
                                fbidx = base_idx;
                                f_base_idx = base_pos;
                                f_base_x = base_x;
                                flx = lx;
                                frx = rx;
                                flidx = lidx;
                                fridx = ridx;
                            }
                            sol.unlock();
                            if (found) break;
                        }
                        if (found) break;
                    }
                    if (found) break;
                }
                if (found) break;
            }
            //            cout << base_idx << " success" << endl;
        });
    }
    for (auto &thread: threads) {
        thread.join();
    }

    if (best_score == -1) {
        return {};
    }
    vector<PositionedWord> res;
    res.push_back(PositionedWord{
        .id = words[fbidx].idx,
        .dir = 2,
        .pos = {f_base_idx - f_base_x, 0, 0},
    });
    res.push_back(PositionedWord{
        .id = words[flidx].idx,
        .dir = 1,
        .pos = {f_base_idx - f_base_x + flx, 0, static_cast<int>(words[flidx].r.size()) - 1}
    });
    res.push_back(PositionedWord{
        .id = words[fridx].idx,
        .dir = 1,
        .pos = {f_base_idx - f_base_x + frx, 0, static_cast<int>(words[fridx].r.size()) - 1}
    });
    int fi = 0;

    for (auto &ftop: ftops) {
        auto &top = words[ftop.second.first];

        if (fi == 0) {
            used.push_back(words[fbidx].idx);
            used.push_back(words[flidx].idx);
            used.push_back(words[fridx].idx);

            findLadder(w, used, mapSize, base_char, base_pos, 0, 0, top.idx, flx, frx);
        }

        int floor = ftop.first;
        int offs = ftop.second.second;
        res.push_back(PositionedWord{
            .id = top.idx,
            .dir = 2,
            .pos = {f_base_idx - f_base_x + flx - offs, 0, floor}
        });

        fi++;
    }
    return res;
}


vector<PositionedWord> enrichZ(vector<runes> &w, vector<int> &used, vector<int> mapSize,
                               vector<PositionedWord> &already, int y) {
    vector<word> words;
    int idx = -1;
    for (const runes &r: w) {
        idx++;
        if (r.size() > zEnrichLimit) {
            continue;
        }
        bool skip = false;
        for (const int u: used) {
            if (idx == u) {
                skip = true;
                break;
            }
        }
        if (skip) continue;
        words.push_back({
            r, idx, 0, ""
        });
    }
    vector<vector<int> > xz(mapSize[0], vector<int>(mapSize[2], -1));
    vector<vector<bool> > horizontal(mapSize[0], vector<bool>(mapSize[2], false));
    vector<vector<bool> > vertical(mapSize[0], vector<bool>(mapSize[2], false));
    for (int i = 0; i < already.size(); i++) {
        const auto &wrd = already[i];
        int len = w[wrd.id].size();
        if (wrd.dir == 1) {
            int x = wrd.pos[0];
            for (int z = wrd.pos[2]; z > wrd.pos[2] - len; z--) {
                if (wrd.pos[1] == y) {
                    xz[x][z] = w[wrd.id][wrd.pos[2] - z];
                }
                if (abs(wrd.pos[1] - y) < 2) {
                    vertical[x][z] = true;
                }
            }
        } else if (wrd.dir == 2) {
            int z = wrd.pos[2];
            for (int x = wrd.pos[0]; x < wrd.pos[0] + len; x++) {
                if (wrd.pos[1] == y) {
                    xz[x][z] = w[wrd.id][x - wrd.pos[0]];
                    horizontal[x][z] = true;
                }
            }
        } else {
            int x = wrd.pos[0], z = wrd.pos[2];
            if (wrd.pos[1] <= y && wrd.pos[1] + len > y) {
                xz[x][z] = w[wrd.id][y - wrd.pos[1]];
                horizontal[x][z] = true;
            }
        }
    }
    vector<PositionedWord> res;
    for (int pz = mapSize[2]; pz > 0; pz--) {
        for (int lx = 0; lx < mapSize[0]; lx++) {
            int x = mapSize[0] / 2 + (lx % 2 == 0 ? -(lx / 2) : (lx / 2));
            //            cout << "PROCESSING" << x << ' ' << pz << endl;
            for (int wid = 0; wid < words.size(); wid++) {
                const auto &word = words[wid];
                if (word.turn == -1) {
                    continue;
                }
                int z = pz;
                bool ok = true;
                int cnt = 0;
                if (z + 1 < mapSize[2] && (vertical[x][z + 1] || horizontal[x][z + 1])) {
                    ok = false;
                    continue;
                }
                for (int i = 0; i < word.r.size(); i++, z--) {
                    if (z < 0) {
                        ok = false;
                        break;
                    }
                    cnt += horizontal[x][z];
                    if (xz[x][z] != -1 && word.r[i] != xz[x][z]) {
                        ok = false;
                        break;
                    }
                    if (vertical[x][z] || (x + 1 < mapSize[0] && (
                                               vertical[x + 1][z] || (horizontal[x + 1][z] && !horizontal[x][z]))) || (
                            x >= 1 && (vertical[x - 1][z] || (horizontal[x - 1][z] && !horizontal[x][z])))) {
                        ok = false;
                        break;
                    }
                }
                if (z >= 0 && (vertical[x][z] || horizontal[x][z])) {
                    ok = false;
                    continue;
                }
                if (ok && cnt >= 2) {
                    words[wid].turn = -1;
                    res.push_back(PositionedWord{
                        .id = word.idx,
                        .dir = 1,
                        .pos = {x, y, pz}
                    });
                    z = pz;
                    for (int i = 0; i < word.r.size(); i++, z--) {
                        xz[x][z] = word.r[i];
                        vertical[x][z] = true;
                    }
                    break;
                }
            }
        }
    }
    return res;
}

vector<PositionedWord> enrichY(vector<runes> &w, vector<int> &used, vector<int> mapSize,
                               vector<PositionedWord> &already, int z) {
    vector<word> words;
    int idx = -1;
    for (const runes &r: w) {
        idx++;
        if (r.size() > yEnrichLimit) {
            continue;
        }
        bool skip = false;
        for (const int u: used) {
            if (idx == u) {
                skip = true;
                break;
            }
        }
        if (skip) continue;
        words.push_back({
            r, idx, 0, ""
        });
    }
    vector<vector<int> > xy(mapSize[0], vector<int>(mapSize[1], -1));
    vector<vector<bool> > horizontal(mapSize[0], vector<bool>(mapSize[1], false));
    vector<vector<bool> > horizontalDiff(mapSize[0], vector<bool>(mapSize[1], false));
    vector<vector<bool> > horizontalY(mapSize[0], vector<bool>(mapSize[1], false));
    vector<vector<bool> > horizontalDiffY(mapSize[0], vector<bool>(mapSize[1], false));
    vector<vector<bool> > vertical(mapSize[0], vector<bool>(mapSize[1], false));
    for (int i = 0; i < already.size(); i++) {
        const auto &wrd = already[i];
        int len = w[wrd.id].size();
        if (wrd.dir == 1) {
            int x = wrd.pos[0], y = wrd.pos[1];
            if (wrd.pos[2] >= z && z > wrd.pos[2] - len) {
                xy[x][y] = w[wrd.id][wrd.pos[2] - z];
                vertical[x][y] = true;
            }
        } else if (wrd.dir == 2) {
            int y = wrd.pos[1];
            for (int x = wrd.pos[0]; x < wrd.pos[0] + len; x++) {
                if (wrd.pos[2] == z) {
                    xy[x][y] = w[wrd.id][x - wrd.pos[0]];
                    horizontal[x][y] = true;
                }
                if (abs(wrd.pos[2] - z) < 2) {
                    horizontalDiff[x][y] = true;
                }
            }
        } else {
            int x = wrd.pos[0];
            for (int y = wrd.pos[1]; y < wrd.pos[1] + len; y++) {
                if (wrd.pos[2] == z) {
                    xy[x][y] = w[wrd.id][y - wrd.pos[1]];
                    horizontal[x][y] = true;
                    horizontalY[x][y] = true;
                }
                if (abs(wrd.pos[2] - z) < 2) {
                    horizontalDiff[x][y] = true;
                    horizontalDiffY[x][y] = true;
                }
            }
        }
    }
    vector<PositionedWord> res;
    for (int py = 0; py < mapSize[1]; py++) {
        for (int x = 0; x < mapSize[0]; x++) {
            for (int wid = 0; wid < words.size(); wid++) {
                const auto &word = words[wid];
                if (word.turn == -1) continue;
                int y = py;
                bool ok = true;
                int cnt = 0;
                if (y - 1 >= 0 && (horizontal[x][y - 1] || vertical[x][y - 1])) {
                    ok = false;
                    continue;
                }
                for (int i = 0; i < word.r.size(); i++, y++) {
                    if (y >= mapSize[1]) {
                        ok = false;
                        break;
                    }
                    if (horizontalY[x][y] || horizontalDiffY[x][y] || horizontalDiff[x][y] || (
                            x > 0 && ((horizontal[x - 1][y] && !horizontal[x][y]) || vertical[x - 1][y]))
                        || (x + 1 < mapSize[0] && (
                                (horizontal[x + 1][y] && !horizontal[x][y]) || vertical[x + 1][y]))) {
                        ok = false;
                        break;
                    }
                    if (xy[x][y] != -1 && word.r[i] != xy[x][y]) {
                        ok = false;
                        break;
                    }
                    cnt += vertical[x][y];
                }
                if (y < mapSize[1] && (horizontal[x][y] || vertical[x][y])) {
                    ok = false;
                    continue;
                }
                if (ok && cnt >= 2) {
                    words[wid].turn = -1;
                    res.push_back(PositionedWord{
                        .id = word.idx,
                        .dir = 3,
                        .pos = {x, py, z}
                    });
                    y = py;
                    for (int i = 0; i < word.r.size(); i++, y++) {
                        xy[x][y] = word.r[i];
                        horizontal[x][y] = true;
                        horizontalY[x][y] = true;
                    }
                    break;
                }
            }
        }
    }
    return res;
}

uint64_t timeSinceEpochMillisec() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int main() {
    simInit();
    std::thread solution_thread([&]() {
        auto status = words_wrapper();
        while (true) {
            auto cl = timeSinceEpochMillisec();
            vector<word> words;
            vector<runes> rs;
            map<pair<int, int>, vector<int> > wp;
            map<int, int> word_idx;
            if (status.usedWords.size() >= 800) {
                break;
            }
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
                rs.push_back(r);
                words.push_back({
                    r, idx, status.turn, w
                });
                idx++;
            }

            sort(words.begin(), words.end(), cmp);
            for (int i = 0; i < words.size(); i++) {
                word_idx[words[i].idx] = i;
            }
            auto pw = findLadder(status.words, status.usedWords, status.mapSize);
            vector<int> curr_used = status.usedWords;
            int xMin = 1e9, xMax = -1;
            cout << "baseLadder: ";
            for (const auto &w: pw) {
                cout << w.id << ' ' << words[word_idx[w.id]].s << endl;
                curr_used.push_back(w.id);
            }
            idx = 0;
            for (const auto &w: words) {
                auto &r = w.r;
                bool found = false;
                for (const int u: curr_used) {
                    if (u == w.idx) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    idx++;
                    continue;
                }
                for (int i = 0; i < r.size(); i++) {
                    wp[{i, r[i]}].push_back(idx);
                }
                idx++;
            }
            for (const auto &w: pw) {
                if (w.dir == 2) {
                    xMin = min(xMin, w.pos[0]);
                    xMax = max(xMax, w.pos[0] + (int) words[word_idx[w.id]].r.size());
                }
            }
            auto base_word = words[word_idx[pw[0].id]];
            auto connector = words[wp[{0, base_word.r[0]}][0]];
            cout << endl << "base + connector:" << base_word.s << ' ' << connector.s << endl;
            cout << base_word.r[0] << ' ' << connector.r[0] << endl;
            pw.push_back(PositionedWord{
                .id = connector.idx,
                .dir = 3,
                .pos = {0, 0, 0}
            });
            cout << endl << "connector " << connector.idx << endl;
            curr_used.push_back(connector.idx);
            for (int y = connector.r.size() - 1; y > 1; y--) {
                auto nw = findLadder(status.words, curr_used, status.mapSize, connector.r[y], 0, xMin, xMax);
                if (!nw.empty()) {
                    cout << "yLadder: ";
                    for (auto &w: nw) {
                        cout << w.id << ' ' << words[word_idx[w.id]].s << ' ' << w.pos[0] << ' ' << w.pos[1] << ' ' << w
                                .pos[2] << ' ' << w.dir << endl;
                        w.pos[1] = y;
                        pw.push_back(w);
                        curr_used.push_back(w.id);
                        if (w.dir == 2) {
                            xMin = min(xMin, w.pos[0]);
                            xMax = max(xMax, w.pos[0] + (int) words[word_idx[w.id]].r.size());
                        }
                    }
                    y--;
                }
                //предусмотрительность
                cout << y << ' ' << nw.size() << ' ' << xMin << ' ' << xMax << endl;
            }
            int x_offs = 0;
            int y_offs = 0;
            for (const auto &i: pw) {
                x_offs = min(x_offs, i.pos[0]);
                y_offs = min(y_offs, i.pos[1]);
            }
            for (auto &i: pw) {
                i.pos[0] -= x_offs;
                i.pos[1] -= y_offs;
            }
            cout << x_offs << ' ' << y_offs << endl;
            for (int i = 0; i < enrichCnt; i++) {
                for (int y = 0; y < status.mapSize[1]; y++) {
                    auto ew = enrichZ(rs, curr_used, status.mapSize, pw, y);
                    cout << "enrichY " << i << ' ' << y << ":";
                    for (const auto &w: ew) {
                        cout << w.id << ' ' << words[word_idx[w.id]].s << ' ' << w.pos[0] << ' ' << w.pos[1] << ' ' << w
                                .pos[2] << ' ' << w.dir << endl;
                        pw.push_back(w);
                        curr_used.push_back(w.id);
                    }
                    cout << endl;
                    if (ew.size() > 0) {
                        cout << "enriched Oz(" << ew.size() << ") with y = " << y << endl;
                    }
                }

                for (int z = status.mapSize[2] - 1; z >= 0; z--) {
                    auto ew = enrichY(rs, curr_used, status.mapSize, pw, z);
                    cout << "enrichZ " << i << ' ' << z << ":";
                    for (const auto &w: ew) {
                        cout << w.id << ' ' << words[word_idx[w.id]].s << ' ' << w.pos[0] << ' ' << w.pos[1] << ' ' << w
                                .pos[2] << ' ' << w.dir << endl;
                        pw.push_back(w);
                        curr_used.push_back(w.id);
                    }
                    cout << endl;
                    if (ew.size() > 0) {
                        cout << "enriched Oy(" << ew.size() << ") with z = " << z << endl;
                    }
                }
            }

            cout << timeSinceEpochMillisec() - cl << "ms elapsed";
            build_wrapper(BuildRequest{
                .done = true,
                .words = pw
            });
            while (true) {
                sleep(1);
            }
            Status st2 = words_wrapper();
            sleep(st2.nextTurnSec + 1);
            status = words_wrapper();
        }
    });


    // render
    SDL_GLContext glContext;
    SDL_Window *window;
    tie(glContext, window) = setWindowUp();

    if (!window || !glContext) {
        return 1;
    }

    float defaultCameraX = AppWindow.cameraX, defaultCameraY = AppWindow.cameraY, defaultCameraZ = AppWindow.cameraZ;
    float yaw = AppWindow.mouseControl.yaw, pitch = AppWindow.mouseControl.pitch;

    Uint32 lastFrameTime = SDL_GetTicks();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEMOTION) {
                float xOffset = event.motion.xrel * AppWindow.mouseControl.sensitivity;
                float yOffset = -event.motion.yrel * AppWindow.mouseControl.sensitivity; // Inverted Y

                yaw += xOffset;
                pitch += yOffset;

                // Clamp pitch
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;
            }
        }

        // Keyboard input for camera movement
        Uint32 currentFrameTime = SDL_GetTicks();
        float deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentFrameTime;

        // Continuous key input handling
        const Uint8 *keystate = SDL_GetKeyboardState(nullptr);
        if (keystate[SDL_SCANCODE_W]) defaultCameraZ -= AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_S]) defaultCameraZ += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_A]) defaultCameraX -= AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_D]) defaultCameraX += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_UP]) defaultCameraY += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_DOWN]) defaultCameraY -= AppWindow.cameraSpeed * deltaTime;

        float dirX = cosf(yaw * M_PI / 180.0f) * cosf(pitch * M_PI / 180.0f);
        float dirY = sinf(pitch * M_PI / 180.0f);
        float dirZ = sinf(yaw * M_PI / 180.0f) * cosf(pitch * M_PI / 180.0f);

        float lookAtX = defaultCameraX + dirX;
        float lookAtY = defaultCameraY + dirY;
        float lookAtZ = defaultCameraZ + dirZ;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mu.lock();
        for (const auto &cube: cubes) {
            drawCubeWithOutline(cube.x, cube.y, cube.z, cube.r, cube.g, cube.b);
        }
        mu.unlock();

        glLoadIdentity();
        gluLookAt(defaultCameraX, defaultCameraY, defaultCameraZ, lookAtX, lookAtY, lookAtZ, 0.0, 1.0, 0.0);

        SDL_GL_SwapWindow(window);
    }

    cleanupOnClose(glContext, window);

    solution_thread.join();
}
