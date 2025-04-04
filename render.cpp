#define GL_SILENCE_DEPRECATION
#include <SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <cstdlib>

using namespace std;


struct Color {
    GLfloat r, g, b;
};

struct appWindow {
    const char* name;
    int w, h;
    Color backgroundColor;
};


constexpr appWindow AppWindow{"cubee envelope igger", 800, 600, Color{0.75f, 0.75f, 0.75f}};

float randColor() {
    return (rand() % 80 + 100) / 255.0f; // Random value between 0.39 and 1.0 (avoids too dark colors)
}

// Generate a random color for cube
Color generateRandomColor() {
    Color color{};
    color.r = randColor();
    color.g = randColor();
    color.b = randColor();
    return color;
}

// cube size
GLfloat* generateCubeVertices(GLfloat sideLength) {
    GLfloat halfSide = sideLength / 2.0f;

    static GLfloat vertices[] = {
        // Front face
        -halfSide, -halfSide,  halfSide,  halfSide,
        -halfSide,  halfSide,  halfSide,   halfSide,
        halfSide,  -halfSide,  halfSide,   halfSide,

        // Back face
        -halfSide, -halfSide, -halfSide,  halfSide,
        -halfSide,  -halfSide,  halfSide,  halfSide,
        -halfSide, -halfSide, halfSide,  -halfSide,
    };

    return vertices;
}

// Define indices for drawing the cube
const GLushort cubeIndices[] = {
    0, 1, 2, 2, 3, 0,  // Front
    4, 5, 6, 6, 7, 4,  // Back
    0, 3, 7, 7, 4, 0,  // Left
    1, 2, 6, 6, 5, 1,  // Right
    3, 2, 6, 6, 7, 3,  // Top
    0, 1, 5, 5, 4, 0   // Bottom
};

const GLushort cubeEdgeIndices[] = {
    0, 1, 1, 2, 2, 3, 3, 0,  // Front
    4, 5, 5, 6, 6, 7, 7, 4,  // Back
    0, 4, 1, 5, 2, 6, 3, 7   // Connecting front and back
};

void drawCubeEdges() {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, generateCubeVertices(1.0f));
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, cubeEdgeIndices);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawCube() {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, generateCubeVertices(1.0f));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, cubeIndices);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// main cube drawing function
void drawCubeWithOutline(float posX, float posY, float posZ, float colorR, float colorG, float colorB) {
    glPushMatrix();
    glTranslatef(posX, posY, posZ); // Apply translation once for the entire cube

    // Draw the outline (slightly larger cube)
    glPushMatrix();
    glScalef(1.01f, 1.01f, 1.01f); // Scale slightly larger for outline
    glColor3f(0.0f, 0.0f, 0.0f); // Outline color (black)
    drawCubeEdges(); // Draw the edges
    glPopMatrix();

    glColor3f(colorR, colorG, colorB);
    drawCube();

    glPopMatrix();
}

int setWindowUp() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(AppWindow.name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, AppWindow.w, AppWindow.h, SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_Quit();
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // OpenGL setup
    glEnable(GL_DEPTH_TEST);
    glClearColor(AppWindow.backgroundColor.r, AppWindow.backgroundColor.g, AppWindow.backgroundColor.b, 1.0f);

    // Setup projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 800.0 / 600.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);

    return 0;
}

void manageCamera(float defaultCameraX, float defaultCameraY, float defaultCameraZ) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // running = false; // TODO: manage window close
        }
        // Keyboard input for camera movement
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_w: defaultCameraZ -= 0.2f; break; // Move forward
                case SDLK_s: defaultCameraZ += 0.2f; break; // Move backward
                case SDLK_a: defaultCameraX -= 0.2f; break; // Move left
                case SDLK_d: defaultCameraX += 0.2f; break; // Move right
                case SDLK_UP:    defaultCameraY += 0.2f; break; // Move up
                case SDLK_DOWN:  defaultCameraY -= 0.2f; break; // Move down
            }
        }
    }

    glLoadIdentity();
    gluLookAt(defaultCameraX, defaultCameraY, defaultCameraZ,  0.0f,  0.0f,  0.0f, 0.0, 1.0, 0.0);
}


// cleanup on window close
void cleanupOnClose(SDL_GLContext context, SDL_Window* window) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// TODO: glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); ----> draw again ----> SDL_GL_SwapWindow(window);
