#include <GL4D/gl4dg.h>
#include <GL4D/gl4dp.h>

typedef struct _Cercle Cercle;

typedef struct _AABB AABB;

typedef struct _Point Point;

typedef struct _Vecteur Vecteur;

int CollisionCercleAABB(Cercle C1, AABB box1);
int CollisionPointCercle(GLfloat x, GLfloat y, Cercle C);
