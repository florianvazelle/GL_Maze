/*!\file window.c
 *
 * \brief Walk in a labyrinth with floor marking and compass.
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date March 05 2018
 */
#include "collision_toolbox.h"
#include <GL4D/gl4dg.h>
#include <GL4D/gl4dp.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>
#include <math.h>
#include <time.h>

struct _Cercle {
        GLfloat x, y, rayon;
};

struct _AABB {
        GLfloat x, y, w, h;
};

struct _Point {
        GLfloat x, y;
};

typedef struct {
        Point a, b;
} Seg;


static void quit(void);
static void initGL(void);
static void initData(void);
static void resize(int w, int h);
static void idle(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void pmotion(int x, int y);
static void draw(void);

static void walls(void);
int hit2(Cercle, Point);

/* from makeLabyrinth.c */
extern unsigned int *labyrinth(int w, int h);

/*!\brief opened window width and height */
static int _wW = 800, _wH = 600;
/*!\brief mouse position (modified by pmotion function) */
static int _xm = 400, _ym = 300;
/*!\brief labyrinth to generate */
static GLuint *_labyrinth = NULL;
/*!\brief labyrinth side */
static GLuint _lab_side = 15;
/*!\brief Quad geometry Id  */
static GLuint _plane = 0;
/*!\brief Cube geometry Id  */
static GLuint _cube = 0;
/*!\brief GLSL program Id */
static GLuint _pId = 0;
/*!\brief plane texture Id */
static GLuint _planeTexId = 0;
/*!\brief compass texture Id */
static GLuint _compassTexId = 0;
/*!\brief plane scale factor */
static GLfloat _planeScale = 100.0f;
/*!\brief boolean to toggle anisotropic filtering */
static GLboolean _anisotropic = GL_FALSE;
/*!\brief boolean to toggle mipmapping */
static GLboolean _mipmap = GL_FALSE;

static GLuint _wallTexId = 0;
static GLuint _ballTexId = 0;
static GLuint _sphere = 0;


/*!\brief enum that index keyboard mapping for direction commands */
enum kyes_t { KLEFT = 0, KRIGHT, KUP, KDOWN };

/*!\brief virtual keyboard for direction commands */
static GLuint _keys[] = {0, 0, 0, 0};

typedef struct cam_t cam_t;
/*!\brief a data structure for storing camera position and
 * orientation */
struct cam_t {
        GLfloat x, z;
        GLfloat theta;
};

/*!\brief the used camera */
static cam_t _cam = {0, 0, 0};

float vec[4][2] = {
        {0.0f, 1.0f}, // up
        {1.0f, 0.0f}, // right
        {0.0f, -1.0f}, // down
        {-1.0f, 0.0f} // left
};

int nb_ball = 0;
GLfloat *balls;

int nb_ball_p = 0;
GLfloat *balls_p;

/*!\brief creates the window, initializes OpenGL parameters,
 * initializes data and maps callback functions */
int main(int argc, char **argv) {
        if (!gl4duwCreateWindow(argc, argv, "GL4Dummies", 10, 10, _wW, _wH,
                                GL4DW_RESIZABLE | GL4DW_SHOWN))
                return 1;
        initGL();
        initData();
        atexit(quit);
        gl4duwResizeFunc(resize);
        gl4duwKeyUpFunc(keyup);
        gl4duwKeyDownFunc(keydown);
        gl4duwPassiveMotionFunc(pmotion);
        gl4duwDisplayFunc(draw);
        gl4duwIdleFunc(idle);
        gl4duwMainLoop();
        return 0;
}

/*!\brief initializes OpenGL parameters :
 *
 * the clear color, enables face culling, enables blending and sets
 * its blending function, enables the depth test and 2D textures,
 * creates the program shader, the model-view matrix and the
 * projection matrix and finally calls resize that sets the projection
 * matrix and the viewport.
 */
static void initGL(void) {
        glClearColor(0.0f, 0.4f, 0.9f, 0.0f);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        _pId =
                gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
        gl4duGenMatrix(GL_FLOAT, "modelMatrix");
        gl4duGenMatrix(GL_FLOAT, "viewMatrix");
        gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
        resize(_wW, _wH);
}

void initBalls(){
        int i, j;
        GLfloat unit = (_planeScale * 2.0f) / _lab_side;
        for (j = 0; j < _lab_side; j++) {
                for (i = 0; i < _lab_side; i++) {
                        if (_labyrinth[j * _lab_side + i] != -1) {
                                srand((unsigned)time(0));
                                if(rand() % 2 > 0.5f) {
                                        nb_ball += 2;
                                        balls = realloc(balls, nb_ball * sizeof(GLfloat));
                                        balls[nb_ball - 2] = (i * unit) - _planeScale + unit/2;
                                        balls[nb_ball - 1] = -((j * unit) - _planeScale + unit/2);
                                }
                        }
                }
        }
        printf("%d\n", nb_ball);
}

/*!\brief initializes data :
 *
 * creates 3D objects (plane and sphere) and 2D textures.
 */
static void initData(void) {
        /* a red-white texture used to draw a compass */
        GLuint northsouth[] = {(255 << 24) + 255, -1};
        GLuint ball_color[2] = { RGB(255, 255, 0), RGB(255, 255, 0) };
        /* generates a quad using GL4Dummies */
        _plane = gl4dgGenQuadf();
        /* generates a cube using GL4Dummies */
        _cube = gl4dgGenCubef();

        _sphere = gl4dgGenSpheref(5, 5);

        /* creation and parametrization of the plane texture */
        glGenTextures(1, &_planeTexId);
        glBindTexture(GL_TEXTURE_2D, _planeTexId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        _labyrinth = labyrinth(_lab_side, _lab_side);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _lab_side, _lab_side, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, _labyrinth);

        /* creation and parametrization of the compass texture */
        glGenTextures(1, &_compassTexId);
        glBindTexture(GL_TEXTURE_2D, _compassTexId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     northsouth);

        SDL_Surface *t;
        glGenTextures(1, &_wallTexId);
        glBindTexture(GL_TEXTURE_2D, _wallTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        if ((t = IMG_Load("images/wall.jpeg")) != NULL) {
#ifdef __APPLE__
                int mode = t->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
#else
                int mode = t->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
#endif
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _planeScale, _planeScale, 0, mode,
                             GL_UNSIGNED_BYTE, t->pixels);
                SDL_FreeSurface(t);
        } else {
                fprintf(stderr, "can't open file images/wall.jpeg : %s\n", SDL_GetError());
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                             NULL);
        }

        glGenTextures(1, &_ballTexId);
        glBindTexture(GL_TEXTURE_2D, _ballTexId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     ball_color);

        glBindTexture(GL_TEXTURE_2D, 0);

        initBalls();
}

/*!\brief function called by GL4Dummies' loop at resize. Sets the
 *  projection matrix and the viewport according to the given width
 *  and height.
 * \param w window width
 * \param h window height
 */
static void resize(int w, int h) {
        _wW = w;
        _wH = h;
        glViewport(0, 0, _wW, _wH);
        gl4duBindMatrix("projectionMatrix");
        gl4duLoadIdentityf();
        gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0,
                      _planeScale + 1.0);
}

/*!\brief Help to carry out your work. Tracking the position in the
 * world with the position on the map.
 */
static void updatePosition(void) {
        GLfloat xf, zf;
        static int xi = -1, zi = -1;
        /* translate to lower-left */
        xf = _cam.x + _planeScale;
        zf = -_cam.z + _planeScale;
        /* scale to 1.0 x 1.0 */
        xf = xf / (2.0f * _planeScale);
        zf = zf / (2.0f * _planeScale);
        /* rescale to _lab_side x _lab_side */
        xf = xf * _lab_side;
        zf = zf * _lab_side;
        /* re-set previous position to black and the new one to red */
        if ((int)xf != xi || (int)zf != zi) {
                if (xi >= 0 && xi < _lab_side && zi >= 0 && zi < _lab_side &&
                    _labyrinth[zi * _lab_side + xi] != -1) {
                        _labyrinth[zi * _lab_side + xi] = 0;
                        glBindTexture(GL_TEXTURE_2D, _planeTexId);
                        /* try to use the glTexSubImage2D function instead of the glTexImage2D
                         * function */
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _lab_side, _lab_side, 0, GL_RGBA,
                                     GL_UNSIGNED_BYTE, _labyrinth);
                }
                xi = (int)xf;
                zi = (int)zf;
                if (xi >= 0 && xi < _lab_side && zi >= 0 && zi < _lab_side &&
                    _labyrinth[zi * _lab_side + xi] != -1) {
                        _labyrinth[zi * _lab_side + xi] = RGB(255, 0, 0);
                        glBindTexture(GL_TEXTURE_2D, _planeTexId);
                        /* try to use the glTexSubImage2D function instead of the glTexImage2D
                         * function */
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _lab_side, _lab_side, 0, GL_RGBA,
                                     GL_UNSIGNED_BYTE, _labyrinth);
                }
        }
}

/*!\brief function called by GL4Dummies' loop at idle.
 *
 * uses the virtual keyboard states to move the camera according to
 * direction, orientation and time (dt = delta-time)
 */
static void idle(void) {
        Point old;
        Cercle player;

        double dt, dtheta = M_PI, step = 30.0;
        static double t0 = 0, t;
        dt = ((t = gl4dGetElapsedTime()) - t0) / 1000.0;
        t0 = t;
        if (_keys[KLEFT])
                _cam.theta += dt * dtheta;
        if (_keys[KRIGHT])
                _cam.theta -= dt * dtheta;

        player.x = old.x = _cam.x;
        player.y = old.y = _cam.z;
        player.rayon = 1.5f;

        GLfloat s = sin(_cam.theta);
        GLfloat c = cos(_cam.theta);

        if (_keys[KUP]) {
                player.x += -dt * step * s;
                player.y += -dt * step * c;
        }
        if (_keys[KDOWN]) {
                player.x += dt * step * s;
                player.y += dt * step * c;
        }

        int res_col = hit2(player, old);
        if (res_col == 2 || res_col == 4) {
                int res_s = (s == 0) ? 0 : (s > 0) ? 1 : -1;
                if (_keys[KUP]) {
                        _cam.x += -dt * step * res_s;
                }
                if (_keys[KDOWN]) {
                        _cam.x += dt * step * res_s;
                }
        } else if (res_col == 3 || res_col == 6) {
                int res_c = (c == 0) ? 0 : (c > 0) ? 1 : -1;
                if (_keys[KUP]) {
                        _cam.z += -dt * step * res_c;
                }
                if (_keys[KDOWN]) {
                        _cam.z += dt * step * res_c;
                }
        } else if (res_col == 0) {
                _cam.x = player.x;
                _cam.z = player.y;
        }

        updatePosition();
}

/*!\brief function called by GL4Dummies' loop at key-down (key
 * pressed) event.
 *
 * stores the virtual keyboard states (1 = pressed) and toggles the
 * boolean parameters of the application.
 */
static void keydown(int keycode) {
        GLint v[2];
        switch (keycode) {
        case GL4DK_LEFT:
                _keys[KLEFT] = 1;
                break;
        case GL4DK_RIGHT:
                _keys[KRIGHT] = 1;
                break;
        case GL4DK_UP:
                _keys[KUP] = 1;
                break;
        case GL4DK_DOWN:
                _keys[KDOWN] = 1;
                break;
        case GL4DK_ESCAPE:
        case 'q':
                exit(0);
        /* when 'w' pressed, toggle between line and filled mode */
        case 'w':
                glGetIntegerv(GL_POLYGON_MODE, v);
                if (v[0] == GL_FILL) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        glLineWidth(3.0);
                } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        glLineWidth(1.0);
                }
                break;
        /* when 'm' pressed, toggle between mipmapping or nearest for the plane
         * texture */
        case 'm': {
                _mipmap = !_mipmap;
                glBindTexture(GL_TEXTURE_2D, _planeTexId);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
                glGenerateMipmap(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
                break;
        }
        /* when 'a' pressed, toggle on/off the anisotropic mode */
        case 'a': {
                _anisotropic = !_anisotropic;
/* l'Anisotropic sous GL ne fonctionne que si la version de la
   bibliothèque le supporte ; supprimer le bloc ci-après si
   problème à la compilation. */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
                GLfloat max;
                glBindTexture(GL_TEXTURE_2D, _planeTexId);
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                _anisotropic ? max : 1.0f);
                glBindTexture(GL_TEXTURE_2D, 0);
#endif
                break;
        }
        default:
                break;
        }
}

/*!\brief function called by GL4Dummies' loop at key-up (key
 * released) event.
 *
 * stores the virtual keyboard states (0 = released).
 */
static void keyup(int keycode) {
        switch (keycode) {
        case GL4DK_LEFT:
                _keys[KLEFT] = 0;
                break;
        case GL4DK_RIGHT:
                _keys[KRIGHT] = 0;
                break;
        case GL4DK_UP:
                _keys[KUP] = 0;
                break;
        case GL4DK_DOWN:
                _keys[KDOWN] = 0;
                break;
        default:
                break;
        }
}

/*!\brief function called by GL4Dummies' loop at the passive mouse motion
 * event.*/
static void pmotion(int x, int y) {
        _xm = x;
        _ym = y;
}

/*!\brief function called by GL4Dummies' loop at draw.*/
static void draw(void) {
        /* clears the OpenGL color buffer and depth buffer */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* sets the current program shader to _pId */
        glUseProgram(_pId);
        gl4duBindMatrix("viewMatrix");
        /* loads the identity matrix in the current GL4Dummies matrix ("viewMatrix")
         */
        gl4duLoadIdentityf();
        /* modifies the current matrix to simulate camera position and orientation in
         * the scene */
        /* see gl4duLookAtf documentation or gluLookAt documentation */
        gl4duLookAtf(_cam.x, 3.0, _cam.z, _cam.x - sin(_cam.theta),
                     3.0 - (_ym - (_wH >> 1)) / (GLfloat)_wH,
                     _cam.z - cos(_cam.theta), 0.0, 1.0, 0.0);
        gl4duBindMatrix("modelMatrix");
        /* loads the identity matrix in the current GL4Dummies matrix ("modelMatrix")
         */
        gl4duLoadIdentityf();
        /* sets the current texture stage to 0 */
        glActiveTexture(GL_TEXTURE0);
        /* tells the pId program that "tex" is set to stage 0 */
        glUniform1i(glGetUniformLocation(_pId, "tex"), 0);

        /* pushs (saves) the current matrix (modelMatrix), scales, rotates,
         * sends matrices to pId and then pops (restore) the matrix */
        gl4duPushMatrix();
        {
                gl4duRotatef(-90, 1, 0, 0);
                gl4duScalef(_planeScale, _planeScale, 1);
                gl4duSendMatrices();
        }
        gl4duPopMatrix();
        /* culls the back faces */
        glCullFace(GL_BACK);
        /* uses the checkboard texture */
        glBindTexture(GL_TEXTURE_2D, _planeTexId);
        /* sets in pId the uniform variable texRepeat to the plane scale */
        glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1.0);
        /* draws the plane */
        gl4dgDraw(_plane);

        walls();

        /* the compass should be drawn in an orthographic projection, thus
         * we should bind the projection matrix; save it; load identity;
         * bind the model-view matrix; modify it to place the compass at the
         * top left of the screen and rotate the compass according to the
         * camera orientation (theta); send matrices; restore the model-view
         * matrix; bind the projection matrix and restore it; and then
         * re-bind the model-view matrix for after.*/
        gl4duBindMatrix("projectionMatrix");
        gl4duPushMatrix();
        {
                gl4duLoadIdentityf();
                gl4duBindMatrix("modelMatrix");
                gl4duPushMatrix();
                {
                        gl4duLoadIdentityf();
                        gl4duTranslatef(-0.75, 0.7, 0.0);
                        gl4duRotatef(-_cam.theta * 180.0 / M_PI, 0, 0, 1);
                        gl4duScalef(0.03 / 5.0, 1.0 / 5.0, 1.0 / 5.0);
                        gl4duBindMatrix("viewMatrix");
                        gl4duPushMatrix();
                        {
                                gl4duLoadIdentityf();
                                gl4duSendMatrices();
                        }
                        gl4duPopMatrix();
                        gl4duBindMatrix("modelMatrix");
                }
                gl4duPopMatrix();
                gl4duBindMatrix("projectionMatrix");
        }
        gl4duPopMatrix();
        gl4duBindMatrix("modelMatrix");
        /* disables cull facing and depth testing */
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        /* uses the compass texture */
        glBindTexture(GL_TEXTURE_2D, _compassTexId);
        /* texture repeat only once */
        glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1);
        /* draws the compass */
        gl4dgDraw(_plane);

        gl4duBindMatrix("projectionMatrix");
        gl4duPushMatrix();
        {
                gl4duLoadIdentityf();
                gl4duOrthof(-1.0, 1.0, -_wH / (GLfloat)_wW, _wH / (GLfloat)_wW, 0.0, 2.0);
                gl4duBindMatrix("modelMatrix");
                gl4duPushMatrix();
                {
                        gl4duLoadIdentityf();
                        gl4duTranslatef(0.75, -0.4, 0.0);
                        gl4duRotatef(-_cam.theta * 180.0 / M_PI, 0, 0, 1);
                        gl4duScalef(1.0 / 5.0, 1.0 / 5.0, 1.0);
                        gl4duBindMatrix("viewMatrix");
                        gl4duPushMatrix();
                        {
                                gl4duLoadIdentityf();
                                gl4duSendMatrices();
                        }
                        gl4duPopMatrix();
                        gl4duBindMatrix("modelMatrix");
                }
                gl4duPopMatrix();
                gl4duBindMatrix("projectionMatrix");
        }
        gl4duPopMatrix();
        gl4duBindMatrix("modelMatrix");
        /* disables cull facing and depth testing */
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        /* uses the labyrinth texture */
        glBindTexture(GL_TEXTURE_2D, _planeTexId);
        /* draws borders */
        glUniform1i(glGetUniformLocation(_pId, "border"), 1);
        /* draws the map */
        gl4dgDraw(_plane);
        /* do not draw borders */
        glUniform1i(glGetUniformLocation(_pId, "border"), 0);

        /* enables cull facing and depth testing */
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
}

/*!\brief function called at exit. Frees used textures and clean-up
 * GL4Dummies.*/
static void quit(void) {
        if (_labyrinth)
                free(_labyrinth);
        if (_planeTexId)
                glDeleteTextures(1, &_planeTexId);
        if (_compassTexId)
                glDeleteTextures(1, &_compassTexId);
        gl4duClean(GL4DU_ALL);
}
void drawWalls() {
        int i, j;
        GLfloat unit = (_planeScale * 2.0f) / _lab_side;
        for (j = 0; j < _lab_side; j++) {
                for (i = 0; i < _lab_side; i++) {
                        if (_labyrinth[j * _lab_side + i] == -1) {
                                gl4duPushMatrix();
                                {
                                        gl4duTranslatef((i * unit) - _planeScale + unit/2, 0,
                                                        -((j * unit) - _planeScale + unit/2));
                                        gl4duScalef((_planeScale / _lab_side), 4, (_planeScale / _lab_side));
                                        gl4duSendMatrices();
                                }
                                gl4duPopMatrix();
                                gl4dgDraw(_cube);

                        }

                }
        }
}

void drawBalls(){
        int i, j, k;
        GLfloat unit = (_planeScale * 2.0f) / _lab_side;
        for(k = 0; k < nb_ball / 2; k += 2) {
                i = balls[k];
                j = balls[k + 1];
                gl4duPushMatrix();
                {
                        gl4duTranslatef(i, 2, j);
                        gl4duScalef((_planeScale / _lab_side) / 4, 1, (_planeScale / _lab_side) / 4);
                        gl4duSendMatrices();
                }
                gl4duPopMatrix();
                gl4dgDraw(_sphere);
        }
}

void walls() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _wallTexId);
        drawWalls();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _ballTexId);
        drawBalls();
}

int hit(Cercle player, Point old) {
        int i, j;
        GLfloat unit = (_planeScale * 2.0f) / _lab_side;
        for (j = 0; j < _lab_side; j++) {
                for (i = 0; i < _lab_side; i++) {
                        if (_labyrinth[j * _lab_side + i] == -1) {
                                AABB wall;
                                wall.x = ((i * unit) - _planeScale);
                                wall.y = -((j * unit) - _planeScale) - unit;
                                wall.w = unit;
                                wall.h = unit;
                                if (CollisionCercleAABB(player, wall) == 1) {
                                        Point p;
                                        p.x = player.x;
                                        p.y = old.y;
                                        if(CollisionAABBSeg(wall, old, p) == 0) {
                                                return 2;
                                        }
                                        p.x = old.x;
                                        p.y = player.y;
                                        if(CollisionAABBSeg(wall, old, p) == 0) {
                                                return 3;
                                        }
                                        else return 1;
                                }
                        }
                }
        }
        return 0;
}

GLfloat my_if(GLfloat x){
        return (x != 0) ? (x > 0) ? x + 1 : x - 1 : x;
}

void determineSeg(AABB wall, int xi, int zi, int i, int j, Point *A, Point* B){
        if(zi - j == -1) {
                if(xi - i == 0) {
                        printf("here1\n");
                        A->x = wall.x;
                        A->y = wall.y + wall.h;
                        B->x = wall.x + wall.w;
                        B->y = wall.y + wall.h;
                }
        } else if(zi - j == 0) {
                if(xi - i == -1) {
                        printf("here2\n");
                        A->x = wall.x;
                        A->y = wall.y;
                        B->x = wall.x;
                        B->y = wall.y + wall.h;
                } else if(xi - i == 1) {
                        printf("here3\n");
                        A->x = wall.x + wall.w;
                        A->y = wall.y;
                        B->x = wall.x + wall.w;
                        B->y = wall.y + wall.h;
                }
        } else if(zi - j == 1) {
                if(xi - i == 0) {
                        printf("here4\n");
                        A->x = wall.x;
                        A->y = wall.y;
                        B->x = wall.x + wall.w;
                        B->y = wall.y;
                }
        }
}


int EqualsSeg(Point A, Point B, Point C, Point D){
        if(floor(A.x) == floor(C.x) && floor(A.y) == floor(C.y) && floor(B.x) == floor(D.x) && floor(B.y == D.y)) {
                return 1;
        }
        if(floor(A.x) == floor(D.x) && floor(A.y) == floor(D.y) && floor(B.x )== floor(C.x) && floor(B.y == C.y)) {
                return 1;
        }
        return 0;
}

int determineSide(Point old, GLfloat unit, Point a, Point b){
        Point top;
        top.x = old.x;
        top.y = old.y;
        Point left;
        left.x = old.x;
        left.y = old.y + unit;
        Point bottom;
        bottom.x = old.x + unit;
        bottom.y = old.y + unit;
        Point right;
        right.x = old.x + unit;
        right.y = old.y;

        if(EqualsSeg(top, left, a, b) == 1) {
                return 1;
        }
        if(EqualsSeg(left, bottom, a, b) == 1) {
                return 1;
        }
        if(EqualsSeg(bottom, right, a, b) == 1) {
                return 1;
        }
        if(EqualsSeg(right, top, a, b) == 1) {
                return 1;
        }
        return 0;
}

void remove_ball(int index, GLfloat i, GLfloat j)
{
        nb_ball_p += 2;
        balls_p = realloc(balls_p, nb_ball_p * sizeof(GLfloat));
        balls_p[nb_ball_p - 2] = i;
        balls_p[nb_ball_p - 1] = j;
}

int hit_ball(Cercle player){
        int i, j, res = 1;
        GLfloat xi, zi;

        Cercle p;
        p.x = player.x;
        p.y = player.y;
        p.rayon = 1.0f;

        for(i = 0; i < nb_ball/2; i += 2) {
                xi = balls[i];
                zi = balls[i + 1];
                for(j = 0; j < nb_ball_p/2; j += 2) {
                        if(xi == balls_p[j] && zi == balls_p[j + 1]) {
                                res = 0;
                                break;
                        }
                }
                if(res == 1) {
                        if(CollisionPointCercle(xi, zi, p) == 1) {
                                printf("--------col--------\n");
                                remove_ball(i, xi, zi);
                                nb_ball -= 2;
                        }
                }
        }
}

void my_increase(GLfloat** data, int size)
{
        *data = realloc(*data, (size + 4) * sizeof(GLfloat));
}


int hit2(Cercle player, Point old) {
        hit_ball(player);
        GLfloat xf, zf;
        int xi, zi, i, j, res = 0;

        xf = player.x + _planeScale;
        zf = -player.y + _planeScale;

        xf = xf / (2.0f * _planeScale);
        zf = zf / (2.0f * _planeScale);

        xf = xf * _lab_side;
        zf = zf * _lab_side;

        xi = (int)xf;
        zi = (int)zf;

        GLfloat unit = (_planeScale * 2.0f) / _lab_side;

        GLfloat *data = NULL;
        int size = 0;
        for (j = zi - 1; j <= zi + 1; j++) {
                for (i = xi - 1; i <= xi + 1; i++) {
                        if (_labyrinth[j * _lab_side + i] == -1) {
                                AABB wall;
                                wall.x = ((i * unit) - _planeScale);
                                wall.y = -((j * unit) - _planeScale) - unit;
                                wall.w = unit;
                                wall.h = unit;
                                if(zi - j == -1) {
                                        if(xi - i == -1) {
                                                //RIGHT
                                                if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //BOTTOM
                                                if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y+ wall.h;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                        } else if(xi - i == 0) {
                                                //LEFT
                                                if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x + wall.w;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //RIGHT
                                                if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //BOTTOM
                                                if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y+ wall.h;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                        } else if(xi - i == 1) {
                                                //LEFT
                                                if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x + wall.w;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //BOTTOM
                                                if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y+ wall.h;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                        }
                                } else if(zi - j == 0) {
                                        if(xi - i == -1) {
                                                //TOP
                                                if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y;
                                                        size += 4;
                                                }
                                                //RIGHT
                                                if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //BOTTOM
                                                if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y+ wall.h;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                        } else if(xi - i == 1) {
                                                //TOP
                                                if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y;
                                                        size += 4;
                                                }
                                                //LEFT
                                                if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x + wall.w;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //BOTTOM
                                                if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y+ wall.h;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                        }
                                } else if(zi - j == 1) {
                                        if(xi - i == -1) {
                                                //RIGHT
                                                if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //TOP
                                                if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y;
                                                        size += 4;
                                                }
                                        } else if(xi - i == 0) {
                                                //LEFT
                                                if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x + wall.w;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //RIGHT
                                                if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //TOP
                                                if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x+ wall.w;
                                                        data[size + 3] = wall.y;
                                                        size += 4;
                                                }
                                        } else if(xi - i == 1) {
                                                //LEFT
                                                if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x + wall.w;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y + wall.h;
                                                        size += 4;
                                                }
                                                //TOP
                                                if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                                                        my_increase(&data, size);
                                                        data[size] = wall.x;
                                                        data[size + 1] = wall.y;
                                                        data[size + 2] = wall.x + wall.w;
                                                        data[size + 3] = wall.y;
                                                        size += 4;
                                                }
                                        }
                                }
                        }
                }
        }
        //printf("size: %d\n", size);

        Point a, b;
        for(i = 0; i < size; i += 4) {
                a.x = data[i];
                a.y = data[i + 1];
                b.x = data[i + 2];
                b.y = data[i + 3];
                if (CollisionSegment(a, b, player) == 1) {
/*                        Point p;

                        p.x = player.x + 1.0f;
                        p.y = old.y;

                        int col1 = CollisionSegSeg(old, p, a, b);

                        p.x = player.x - 1.0f;
                        p.y = old.y;

                        int col2 = CollisionSegSeg(old, p, a, b);

                        p.x = old.x;
                        p.y = player.y + 1.0f;

                        int col3 = CollisionSegSeg(old, p, a, b);

                        p.x = old.x;
                        p.y = player.y - 1.0f;

                        int col4 = CollisionSegSeg(old, p, a, b);

                        if(col1 == 0 && col2 == 0) {
                                return 1;
                        }
                        if(col1 == 0 || col2 == 0) {
                                return 2;
                        }
                        if(col3 == 0 || col4 == 0) {
                                return 3;
                        }
 */
//                       return 1;
                }
        }

        free(data);
/*
        for (j = zi - 1; j <= zi + 1; j++) {
                for (i = xi - 1; i <= xi + 1; i++) {
                        if (_labyrinth[j * _lab_side + i] == -1) {
                                Point a, b;
                                AABB wall;
                                wall.x = ((i * unit) - _planeScale);
                                wall.y = -((j * unit) - _planeScale) - unit;
                                wall.w = unit;
                                wall.h = unit;
                                //if(CollisionCercleAABB(player, wall) == 1) {

                                //return 1;
                                  if(!((zi - j == -1 && xi - i == -1) || (zi - j == -1 && xi - i == 1) ||
                                       (zi - j == 1 && xi - i == -1) || (zi - j == 1 && xi - i == 1))) {
                                          determineSeg(wall, xi, zi, i, j, &a, &b);
                                          if (CollisionSegment(a, b, player) == 1) {
                                                  Point p;

                                                  p.x = player.x;
                                                  p.y = player.y;
                                                  //if(determineSide(p, unit, a, b) == 1) {

                                                  p.x = player.x;
                                                  p.y = old.y;

                                                  if(CollisionDroiteSeg(old, p, a, b) == 0) {
                                                          res += 2;
                                                  }

                                                  p.x = old.x;
                                                  p.y = player.y;

                                                  if(CollisionDroiteSeg(old, p, a, b) == 0) {
                                                          res += 3;
                                                  }
                                          }
                                   }
        //  }
   }
   }
   } */
        //printf("%d\n", res);
        return res;
}

//barre d'en haut
int getTop(GLfloat* data, int size, AABB wall, int i, int j){
        if (_labyrinth[(j + 1) * _lab_side + i] != -1) {
                my_increase(&data, size);
                data[size] = wall.x;
                data[size + 1] = wall.y;
                data[size + 2] = wall.x+ wall.w;
                data[size + 3] = wall.y;
                return 4;
        }
        return 0;
}

//barre d'en gauche
int getRight(GLfloat* data, int size, AABB wall, int i, int j){
        if (_labyrinth[j * _lab_side + (i - 1)]!= -1) {
                my_increase(&data, size);
                data[size] = wall.x;
                data[size + 1] = wall.y;
                data[size + 2] = wall.x;
                data[size + 3] = wall.y + wall.h;
                return 4;
        }
        return 0;
}

//barre d'en droite
int getLeft(GLfloat* data, int size, AABB wall, int i, int j){
        if (_labyrinth[j * _lab_side + (i + 1)] != -1) {
                my_increase(&data, size);
                data[size] = wall.x + wall.w;
                data[size + 1] = wall.y;
                data[size + 2] = wall.x + wall.w;
                data[size + 3] = wall.y + wall.h;
                return 4;
        }
        return 0;
}

//barre d'en bas
int getBottom(GLfloat* data, int size, AABB wall, int i, int j){
        if (_labyrinth[(j - 1) * _lab_side + i] != -1) {
                my_increase(&data, size);
                data[size] = wall.x;
                data[size + 1] = wall.y+ wall.h;
                data[size + 2] = wall.x+ wall.w;
                data[size + 3] = wall.y + wall.h;
                return 4;
        }
        return 0;
}

int getWall(AABB wall, int xi, int zi, GLfloat* data, int size){
        int i, j;
        GLfloat unit = (_planeScale * 2.0f) / _lab_side;

        for (j = zi - 1; j <= zi + 1; j++) {
                for (i = xi - 1; i <= xi + 1; i++) {
                        if (_labyrinth[j * _lab_side + i] == -1) {
                                AABB wall;
                                wall.x = ((i * unit) - _planeScale);
                                wall.y = -((j * unit) - _planeScale) - unit;
                                wall.w = unit;
                                wall.h = unit;
                                if(zi - j == -1) {
                                        if(xi - i == -1) {
                                                size += getRight(data, size, wall, i, j);
                                                size += getBottom(data, size, wall, i, j);
                                        } else if(xi - i == 0) {
                                                size += getLeft(data, size, wall, i, j);
                                                size += getRight(data, size, wall, i, j);
                                                size += getBottom(data, size, wall, i, j);
                                        } else if(xi - i == 1) {
                                                size += getLeft(data, size, wall, i, j);
                                                size += getBottom(data, size, wall, i, j);
                                        }
                                } else if(zi - j == 0) {
                                        if(xi - i == -1) {
                                                size += getTop(data, size, wall, i, j);
                                                size += getRight(data, size, wall, i, j);
                                                size += getBottom(data, size, wall, i, j);
                                        } else if(xi - i == 1) {
                                                size += getTop(data, size, wall, i, j);
                                                size += getLeft(data, size, wall, i, j);
                                                size += getBottom(data, size, wall, i, j);
                                        }
                                } else if(zi - j == 1) {
                                        if(xi - i == -1) {
                                                size += getRight(data, size, wall, i, j);
                                                size += getTop(data, size, wall, i, j);
                                        } else if(xi - i == 0) {
                                                size += getLeft(data, size, wall, i, j);
                                                size += getRight(data, size, wall, i, j);
                                                size += getTop(data, size, wall, i, j);
                                        } else if(xi - i == 1) {
                                                size += getLeft(data, size, wall, i, j);
                                                size += getTop(data, size, wall, i, j);
                                        }
                                }
                        }
                }
        }
        return size;
}
