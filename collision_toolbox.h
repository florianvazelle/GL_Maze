struct _Cercle;
typedef struct _Cercle Cercle;

struct _AABB;
typedef struct _AABB AABB;

int CollisionPointAABB(GLfloat curseur_x, GLfloat curseur_y, AABB box);

int CollisionPointCercle(GLfloat x, GLfloat y, Cercle C);

int CollisionCercleAABB(Cercle C1, AABB box1);

int ProjectionSurSegment(GLfloat Cx, GLfloat Cy, GLfloat Ax, GLfloat Ay,
                         GLfloat Bx, GLfloat By);
