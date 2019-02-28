#include <GL4D/gl4dg.h>
#include <GL4D/gl4dp.h>

typedef struct _Cercle
{
        GLfloat x,y;
        GLfloat rayon;
} Cercle;

typedef struct _AABB
{
        GLfloat x;
        GLfloat y;
        GLfloat w;
        GLfloat h;
} AABB;

int CollisionAABBvsAABB(AABB box1,AABB box2)
{
        if((box2.x >= box1.x + box1.w) // trop à droite
           || (box2.x + box2.w <= box1.x) // trop à gauche
           || (box2.y >= box1.y + box1.h) // trop en bas
           || (box2.y + box2.h <= box1.y)) // trop en haut
                return 0;
        else
                return 1;
}

int CollisionPointAABB(GLfloat curseur_x,GLfloat curseur_y,AABB box)
{
        if (curseur_x >= box.x
            && curseur_x < box.x + box.w
            && curseur_y >= box.y
            && curseur_y < box.y + box.h)
                return 1;
        else
                return 0;
}

int CollisionPointCercle(GLfloat x, GLfloat y, Cercle C)
{
        int d2 = (x-C.x)*(x-C.x) + (y-C.y)*(y-C.y);
        if (d2>C.rayon*C.rayon)
                return 0;
        else
                return 1;
}

AABB GetBoxAutourCercle(Cercle c){
        AABB box;
        box.x = c.x - c.rayon;
        box.y = c.y - c.rayon;
        box.w = c.rayon * 2;
        box.h = c.rayon * 2;
        return box;
}

int ProjectionSurSegment(GLfloat Cx, GLfloat Cy, GLfloat Ax, GLfloat Ay, GLfloat Bx, GLfloat By)
{
        GLfloat ACx = Cx-Ax;
        GLfloat ACy = Cy-Ay;
        GLfloat ABx = Bx-Ax;
        GLfloat ABy = By-Ay;
        GLfloat BCx = Cx-Bx;
        GLfloat BCy = Cy-By;
        GLfloat s1 = (ACx*ABx) + (ACy*ABy);
        GLfloat s2 = (BCx*ABx) + (BCy*ABy);
        if (s1 * s2 > 0)
                return 0;
        return 1;
}

int CollisionCercleAABB(Cercle C1, AABB box1)
{
        AABB boxCercle = GetBoxAutourCercle(C1); // retourner la bounding box de l'image porteuse, ou calculer la bounding box.
        if (CollisionAABBvsAABB(box1,boxCercle)==0)
                return 0;  // premier test
        if (CollisionPointCercle(box1.x,box1.y,C1)==1
            || CollisionPointCercle(box1.x,box1.y+box1.h,C1)==1
            || CollisionPointCercle(box1.x+box1.w,box1.y,C1)==1
            || CollisionPointCercle(box1.x+box1.w,box1.y+box1.h,C1)==1)
                return 1;  // deuxieme test
        if (CollisionPointAABB(C1.x,C1.y,box1)==1)
                return 1;  // troisieme test
        int projvertical = ProjectionSurSegment(C1.x,C1.y,box1.x,box1.y,box1.x,box1.y+box1.h);
        int projhorizontal = ProjectionSurSegment(C1.x,C1.y,box1.x,box1.y,box1.x+box1.w,box1.y);
        if (projvertical==1 || projhorizontal==1)
                return 1;  // cas E
        return 0; // cas B
}
