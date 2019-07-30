#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include <iostream>

#include "util.h"
#include "stdlib.h"
#include "assert.h"
#include "perlin.h"
#include "config.h"

typedef enum {OUT = 1, IN = -1} Direction;

// Primitive base class
class SceneObject
{
public:
   bool light;
   bool ident;
   // Returns true if an intersection occurred, false otherwise.
   virtual bool intersect(Ray3D&, Matrix4x4, Matrix4x4) = 0;
   virtual ~SceneObject()
   { };
};

// Square face for light.
// The location of this square has been hard-coded in to save on rendering
// some time.
class Square : public SceneObject
{
private:
   Material *mat;
public:
   Square(Material *mat)
   {
      this->mat = mat;
      light = mat->light;
      ident = true;
   };
   bool intersect(Ray3D& ray, Matrix4x4 worldToModel,
                  Matrix4x4 modelToWorld);
};

// The scene's outer box
// The location of this object has been hard-coded in to save on rendering
// time.
class Cube : public SceneObject
{
private:
   Material *mat1;
   Material *mat2;
   Material *mat3;
   Material *mat4;
   Material *mat5;
   Material *mat6;
   Direction d;
public:

   Cube(Direction d, Material *mat1, Material *mat2, Material *mat3, Material *mat4,
        Material *mat5, Material *mat6)
   {
      this->d = d;
      this->mat1 = mat1;
      this->mat2 = mat2;
      this->mat3 = mat3;
      this->mat4 = mat4;
      this->mat5 = mat5;
      this->mat6 = mat6;
      light = mat1->light;
      ident = true;
   };

   bool intersect(Ray3D& ray, Matrix4x4 worldToModel,
                  Matrix4x4 modelToWorld);
};

// A unit sphere.
// Since there is more than one sphere in the scene, we did not hard-code the
// location of this object.  We use the worldToModel and modelToWorld matrices
// to transform the coordinates for each of the different spheres.
class Sphere : public SceneObject
{
private:
   Material *mat;
   Direction d;
public:

   Sphere(Material *mat)
   {
      this->mat = mat;
      light = mat->light;
      ident = true;
   };
   bool intersect(Ray3D& ray, Matrix4x4 worldToModel, Matrix4x4 modelToWorld);
};

// The water scene's displaced surface
// The location of this object has been hard-coded in to save on rendering
// time.
class DisplacedSurface : public SceneObject
{
private:
   Material *mat;
   Direction d;
   double *coords;

   // Number of patches in each dimension
   int xcoords;
   int zcoords;

   Point3D **points;

   double top_bound;
   double bottom_bound;
   double middle;
   double max_disp;
   Perlin perlin_noise;
public:

   DisplacedSurface(Material *mat, double middle) :
         perlin_noise(1, 0.14, 1.0, 3)
   {
      this->mat = mat;

      xcoords = NUM_TRIAGLES_IN_EACH_DIM;
      zcoords = NUM_TRIAGLES_IN_EACH_DIM;
      this->middle = middle;
      max_disp = 1.0;

      points = (Point3D **) malloc((xcoords + 1) * sizeof(Point3D *));
      for (int i = 0; i < zcoords + 1; i++) {
         points[i] = (Point3D *) malloc((zcoords + 1) * sizeof(Point3D));
      }

      top_bound = middle + max_disp;
      bottom_bound = middle - max_disp;
      makeSurface();
      ident = true;
   };

   ~DisplacedSurface()
   {
      for (int i = 0; i < zcoords + 1; i++) {
         free(points[i]);
      }
      free(points);
   };

   void makeSurface();

   bool intersect(Ray3D& ray, Matrix4x4 worldToModel, Matrix4x4 modelToWorld);
   Point3D findNextGridHit(
      int xcoord, int zcoord,
      Point3D &origin, Vector3D &dir);
   bool checkIntersectionGrid(
      int xcoord, int zcoord,
      Point3D &origin, Vector3D &dir,
      Ray3D &ray,
      Matrix4x4 modelToWorld);
};

#endif
