#include <GL4D/gl4dg.h>
#include <GL4D/gl4dp.h>

typedef struct { GLfloat x, y, rayon; } Cercle;

typedef struct { GLfloat x, y, w, h; } AABB;

typedef struct { GLfloat x, y; } Point;

typedef struct { GLfloat x, y; } Vecteur;

int CollisionAABBvsAABB(AABB box1, AABB box2) {
        if ((box2.x >= box1.x + box1.w) // trop à droite
            || (box2.x + box2.w <= box1.x) // trop à gauche
            || (box2.y >= box1.y + box1.h) // trop en bas
            || (box2.y + box2.h <= box1.y)) // trop en haut
                return 0;
        else
                return 1;
}

int CollisionPointAABB(GLfloat curseur_x, GLfloat curseur_y, AABB box) {
        if (curseur_x >= box.x && curseur_x < box.x + box.w && curseur_y >= box.y &&
            curseur_y < box.y + box.h)
                return 1;
        else
                return 0;
}

int CollisionPointCercle(GLfloat x, GLfloat y, Cercle C) {
        int d2 = (x - C.x) * (x - C.x) + (y - C.y) * (y - C.y);
        if (d2 > C.rayon * C.rayon)
                return 0;
        else
                return 1;
}

AABB GetBoxAutourCercle(Cercle c) {
        AABB box;
        box.x = c.x - c.rayon;
        box.y = c.y - c.rayon;
        box.w = c.rayon * 2;
        box.h = c.rayon * 2;
        return box;
}

int ProjectionSurSegment(GLfloat Cx, GLfloat Cy, GLfloat Ax, GLfloat Ay,
                         GLfloat Bx, GLfloat By) {
        GLfloat ACx = Cx - Ax;
        GLfloat ACy = Cy - Ay;
        GLfloat ABx = Bx - Ax;
        GLfloat ABy = By - Ay;
        GLfloat BCx = Cx - Bx;
        GLfloat BCy = Cy - By;
        GLfloat s1 = (ACx * ABx) + (ACy * ABy);
        GLfloat s2 = (BCx * ABx) + (BCy * ABy);
        if (s1 * s2 > 0)
                return 0;
        return 1;
}

int CollisionCercleAABB(Cercle C1, AABB box1) {
        AABB boxCercle = GetBoxAutourCercle(C1); // retourner la bounding box de
                                                 // l'image porteuse, ou calculer la
                                                 // bounding box.
        if (CollisionAABBvsAABB(box1, boxCercle) == 0)
                return 0;  // premier test
        if (CollisionPointCercle(box1.x, box1.y, C1) == 1 ||
            CollisionPointCercle(box1.x, box1.y + box1.h, C1) == 1 ||
            CollisionPointCercle(box1.x + box1.w, box1.y, C1) == 1 ||
            CollisionPointCercle(box1.x + box1.w, box1.y + box1.h, C1) == 1)
                return 1;  // deuxieme test
        if (CollisionPointAABB(C1.x, C1.y, box1) == 1)
                return 1;  // troisieme test
        int projvertical =
                ProjectionSurSegment(C1.x, C1.y, box1.x, box1.y, box1.x, box1.y + box1.h);
        int projhorizontal =
                ProjectionSurSegment(C1.x, C1.y, box1.x, box1.y, box1.x + box1.w, box1.y);
        if (projvertical == 1 || projhorizontal == 1)
                return 1;  // cas E
        return 0; // cas B
}

int CollisionDroiteSeg(Point A, Point B, Point O, Point P) {
        Vecteur AO, AP, AB;
        AB.x = B.x - A.x;
        AB.y = B.y - A.y;
        AP.x = P.x - A.x;
        AP.y = P.y - A.y;
        AO.x = O.x - A.x;
        AO.y = O.y - A.y;
        if ((AB.x * AP.y - AB.y * AP.x) * (AB.x * AO.y - AB.y * AO.x) < 0)
                return 1;
        else
                return 0;
}

int CollisionSegSeg(Point A, Point B, Point O, Point P) {
        if (CollisionDroiteSeg(A, B, O, P) == 0)
                return 0;  // inutile d'aller plus loin si le segment [OP] ne touche pas la
                          // droite (AB)
        Vecteur AB, OP;
        AB.x = B.x - A.x;
        AB.y = B.y - A.y;
        OP.x = P.x - O.x;
        OP.y = P.y - O.y;
        GLfloat k = -(A.x * OP.y - O.x * OP.y - OP.x * A.y + OP.x * O.y) /
                    (AB.x * OP.y - AB.y * OP.x);
        if (k < 0 || k > 1)
                return 0;
        else
                return 1;
}

int CollisionAABBSeg(AABB box, Point A, Point B) {
        Point a1;
        a1.x = box.x;
        a1.y = box.y;
        Point a2;
        a2.x = box.x + box.w;
        a2.y = box.y;
        Point a3;
        a3.x = box.x + box.w;
        a3.y = box.y + box.h;
        Point a4;
        a4.x = box.x;
        a4.y = box.y + box.h;
        if (CollisionSegSeg(A, B, a1, a2) == 1 ||
            CollisionSegSeg(A, B, a2, a3) == 1 ||
            CollisionSegSeg(A, B, a3, a4) == 1 ||
            CollisionSegSeg(A, B, a4, a1) == 1) {
                return 1;
        }
        return 0;
}
