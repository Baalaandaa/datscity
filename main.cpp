#include <iostream>
#include "api.cpp"
#include "render.cpp"

#include <SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

using namespace std;
using runes = vector<int>;

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

vector<PositionedWord> findLadder(vector<string> w, vector<int> used, vector<int> mapSize, int base_char = -1, int base_pos = 0, int xMin = 0, int xMax = 0) {
    vector<word> words;
    map<pair<int, int>, vector<int>> wp;
    map<pair<int, pair<int, int>>, vector<pair<int, int>>> dw;
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
        idx++;
    }
    double best_score = -1;
    int f_base_idx = 0, f_base_x;
    int flx, frx;
    int flidx, fridx;
    vector<pair<int, pair<int, int>>> ftops;
    vector<thread> threads;
    mutex sol;
    for (int base_idx = 0; base_idx < min(100, (int)words.size()); base_idx++) {
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
            int xMinB = xMin+base_pos-base_x;
            int xmx = max(xMax, xMinB + (int)words[base_idx].r.size());
            int xmn = min(xMin, xMinB);
            if (xmx-xmn >= mapSize[0]) {
                return;
            }
            const auto &base = words[base_idx];
            for (int d = 2; d < base.r.size(); d++) {
                bool found = false;
                for(int lx = 0; lx + d < base.r.size(); lx++) {
                    int rx = lx + d;
                    vector<int> lpos = wp[{0, base.r[lx]}];
                    vector<int> rpos = wp[{0, base.r[rx]}];
                    for(const int lidx: lpos) {
                        for(const int ridx: rpos) {
                            vector<pair<int, pair<int, int>>> tops;
                            double score = 1.25;
                            const auto &l = words[lidx];
                            const auto &r = words[ridx];
                            int fstart = (int)min(l.r.size(), r.r.size());
                            int fend = (int)max(l.r.size(), r.r.size());
//                            score += (fstart + 1 + fend) * (fend - fstart) / 2;
                            for(int floor = fstart - 1; floor > 1; floor--) {
                                vector<pair<int, int> > possible_tops = dw[{d, {l.r[l.r.size() - 1 - floor], r.r[r.r.size() - 1 - floor]}}];
                                int min_delta = mapSize[0], mti = -1;
                                for(int ti = 0; ti < possible_tops.size(); ti++) {
                                    const auto &top = words[possible_tops[ti].first];
                                    int idx = possible_tops[ti].second;
                                    int len = (int)top.r.size();
                                    int xl = min(xmn, base_pos - base_x + lx - idx);
                                    int xr = max(xmx, base_pos - base_x + lx - idx + len);
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
    for (auto& thread : threads) {
        thread.join();
    }

    if (best_score == -1) {
        return {};
    }
    vector<PositionedWord> res;
    res.push_back(PositionedWord{
            .id = words[f_base_idx].idx,
            .dir = 2,
            .pos = {base_pos-f_base_x, 0, 0},
    });
    res.push_back(PositionedWord{
            .id = words[flidx].idx,
            .dir = 1,
            .pos = {base_pos - f_base_idx + flx, 0, static_cast<int>(words[flidx].r.size()) - 1}
    });
    res.push_back(PositionedWord{
            .id = words[fridx].idx,
            .dir = 1,
            .pos = {base_pos - f_base_idx + frx, 0, static_cast<int>(words[fridx].r.size()) - 1}
    });
    for (auto & ftop : ftops) {
        auto &top = words[ftop.second.first];
        int floor = ftop.first;
        int offs = ftop.second.second;
        res.push_back(PositionedWord{
                .id = top.idx,
                .dir = 2,
                .pos = {base_pos - f_base_idx + flx - offs, 0, floor}
        });
    }
    return res;
}

int main() {
    simInit();
    std::thread solution_thread([&]() {
        auto status = words_wrapper();
        while (true) {
            vector<word> words;
            map<pair<int, int>, vector<int>> wp;
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
                words.push_back({
                                        r, idx, status.turn, w
                                });
                idx++;
            }

            sort(words.begin(), words.end(), cmp);
            for(int i = 0; i < words.size(); i++) {
                word_idx[words[i].idx] = i;
            }
            idx = 0;
            for (const auto &w: words) {
                auto &r = w.r;
                for (int i = 0; i < r.size(); i++) {
                    wp[{0, r[i]}].push_back(idx);
                }

                idx++;
            }
            auto pw = findLadder(status.words, status.usedWords, status.mapSize);
            vector<int> curr_used = status.usedWords;
            int xMin = 1e9, xMax = -1;
            for(const auto &w: pw) {
                curr_used.push_back(w.id);
                if (w.dir == 2) {
                    xMin = min(xMin, w.pos[0]);
                    xMax = max(xMax, w.pos[0] + (int) words[word_idx[w.id]].r.size());
                }
            }
            auto base_word = words[word_idx[pw[0].id]];
            auto connector = words[wp[{0, base_word.r[0]}][0]];
            pw.push_back(PositionedWord{
                .id = connector.idx,
                .dir = 3,
                .pos = {0, 0, 0}
            });
            curr_used.push_back(connector.idx);
            for (int y = connector.r.size(); y >= 0; y--) {
                auto nw = findLadder(status.words, curr_used, status.mapSize, connector.r[y], 0, xMin, xMax);
                if (!nw.empty()) {
                    for (auto &w : nw) {
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
                cout << y << ' ' << nw.size() << ' ' << xMin << ' ' << xMax << endl;
            }
            int x_offs = 0;
            int y_offs = 0;
            for(const auto & i : pw) {
                x_offs = min(x_offs, i.pos[0]);
                y_offs = min(y_offs, i.pos[1]);
            }
            for(auto & i : pw) {
                i.pos[0] -= x_offs;
                i.pos[1] -= y_offs;
            }
            build_wrapper(BuildRequest{
                    .done = true,
                    .words = pw
            });
            Status st2 = words_wrapper();
            cout << "used " << st2.usedWords.size() << endl;
//        sleep(st2.nextTurnSec + 3);
            status = words_wrapper();
            while(true) {
                sleep(1);
            }
        }
    });


    // render
    SDL_GLContext glContext;
    SDL_Window * window;
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
                }else if (event.type == SDL_MOUSEMOTION) {
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
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        if (keystate[SDL_SCANCODE_W])      defaultCameraZ -= AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_S])      defaultCameraZ += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_A])      defaultCameraX -= AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_D])      defaultCameraX += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_UP])     defaultCameraY += AppWindow.cameraSpeed * deltaTime;
        if (keystate[SDL_SCANCODE_DOWN])   defaultCameraY -= AppWindow.cameraSpeed * deltaTime;

        float dirX = cosf(yaw * M_PI / 180.0f) * cosf(pitch * M_PI / 180.0f);
        float dirY = sinf(pitch * M_PI / 180.0f);
        float dirZ = sinf(yaw * M_PI / 180.0f) * cosf(pitch * M_PI / 180.0f);

        float lookAtX = defaultCameraX + dirX;
        float lookAtY = defaultCameraY + dirY;
        float lookAtZ = defaultCameraZ + dirZ;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mu.lock();
        for(const auto& cube: cubes) {
            drawCubeWithOutline(cube.x, cube.y, cube.z, cube.r, cube.g, cube.b);
        }
        mu.unlock();

        glLoadIdentity();
        gluLookAt(defaultCameraX, defaultCameraY, defaultCameraZ,  lookAtX,  lookAtY,  lookAtZ, 0.0, 1.0, 0.0);

        SDL_GL_SwapWindow(window);
    }

    cleanupOnClose(glContext, window);

    solution_thread.join();
}
