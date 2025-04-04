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

vector<PositionedWord> findLadder(vector<string> w, vector<int> used, vector<int> mapSize, int base_char = 0, int base_pos = 0, int xMin = 0, int xMax = 0) {
    vector<word> words;
    map<pair<int, int>, vector<int>> wp;
    map<pair<int, pair<int, int>>, vector<int>> dw;
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
                dw[{d, {r[i], r[i + d]}}].push_back(idx);
            }
        }
        idx++;
    }
    for (int base_idx = 0; base_idx < words.size(); base_idx++) {
        int base_x = 0;
        if (base_char != -1) {
            base_x = 0;
            for (int i = 0; i < words[base_idx].r.size(); i++) {
                if (words[base_idx].r[i] == base_char) {
                    base_x = i;
                }
            }
            if (base_x == -1) {
                continue;
            }
            if (base_x + words[base_idx].r.size() >= mapSize[0]) {
                continue;
            }
        }

    }

}

int main() {
    simInit();
    std::thread solution_thread([&]() {
        auto status = words_wrapper();
        while (true) {
            vector<word> words;
            map<pair<int, int>, vector<int>> wp;
            map<pair<int, pair<int, int>>, vector<int>> dw;
            if (status.usedWords.size() >= 800) {
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
            for (const auto &w: words) {
                auto &r = w.r;
                for (int i = 0; i < r.size(); i++) {
                    wp[{r.size() - i - 1, r[i]}].push_back(idx);
                }
                for (int d = 1; d < r.size(); d++) {
                    for (int i = 0; i + d < r.size(); i++) {
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
            for (int base_idx = 0; base_idx < words.size(); base_idx++) {
                auto base = words[base_idx];
                bool found = false;
                for (int l = 0; l < base.r.size(); l++) {
                    for (int r = base.r.size() - 2; r >= l + 2; r--) {
                        vector<int> lpos = wp[{0, base.r[l]}];
                        vector<int> rpos = wp[{0, base.r[r]}];
                        for (const auto &lidx: lpos) {
                            for (const auto &ridx: rpos) {
                                if (lidx == ridx || base_idx == lidx || base_idx == ridx) {
                                    continue;
                                }
                                auto lw = words[lidx];
                                auto rw = words[ridx];
                                vector<pair<int, int> > floors;
                                int h = min(lw.r.size(), rw.r.size());
                                double score = 1.25 + 1.25 * base.r.size();
                                for (int floor = min(lw.r.size(), rw.r.size()) - 1; floor > 1; floor--) {
                                    vector<int> pos = dw[{r - l, {lw.r[lw.r.size() - 1 - floor],
                                                                  rw.r[rw.r.size() - 1 - floor]}}];
                                    for (int fi = 0; fi < pos.size(); fi++) {
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
            for (int i = 0; i < fts.size(); i++) {
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
