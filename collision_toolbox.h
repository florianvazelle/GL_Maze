typedef struct _Cercle Cercle;

typedef struct _AABB AABB;

typedef struct _Point Point;

typedef struct _Vecteur Vecteur;

int CollisionCercleAABB(Cercle C1, AABB box1);
int CollisionDroiteSeg(Point A, Point B, Point O, Point P);
int CollisionAABBSeg(AABB box, Point A, Point B);
int CollisionSegment(Point A, Point B, Cercle C);
