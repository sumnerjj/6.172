#ifndef UTIL_H
#define UTIL_H

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#ifndef NULL
#define NULL 0
#endif

#include <float.h>

typedef double Matrix4x4[4][4];

// Initializes mat to the identity matrix.
void initMatrix(Matrix4x4 mat);

// Implements ret = mat1*mat2 .
void mulMatrix(Matrix4x4 ret, Matrix4x4 mat1, Matrix4x4 mat2);


struct Vector3D
{
   Vector3D() : x(0), y(0), z(0)
   {}
   Vector3D(double x, double y, double z) : x(x), y(y), z(z)
   {}
   double dot(const Vector3D& v) const;
   Vector3D cross(const Vector3D& v) const;
   Vector3D operator+(const Vector3D& v) const;
   Vector3D operator-(const Vector3D& v) const;
   Vector3D operator-() const;
   double normalize();
   double mag();

   // Returns a transformed vector by right-multiplying it to mat.
   Vector3D transform(Matrix4x4 mat) const;

   // Returns a transformed vector by right-multiplying it to transpose(mat),
   // this is useful for transforming normals back to world space.
   Vector3D transformAsNormal(Matrix4x4 mat) const;

   double x;
   double y;
   double z;
};

// Scalar multiplication.
Vector3D operator*(double s, const Vector3D& v);

struct Point3D
{
   Point3D() : x(0), y(0), z(0)
   {}
   Point3D(double x, double y, double z) : x(x), y(y), z(z)
   {}
   Point3D operator+(const Vector3D& v) const;
   Point3D operator-(const Vector3D& v) const;
   Vector3D operator-(const Point3D& p) const;
   double operator[](int ind) const;
   double &operator[](int ind);
   Point3D transform(Matrix4x4 mat) const;
   double dot(const Point3D& v) const;
   double distanceTo(const Point3D& p) const;
   double dot(const Vector3D& v) const;

   double x;
   double y;
   double z;
};

struct Colour
{
   Colour() : r(0), g(0), b(0)
   {}
   Colour(double c) : r(c), g(c), b(c)
   {}
   Colour(double r, double g, double b) : r(r), g(g), b(b)
   {}
   Colour operator*(const Colour& c) const;
   Colour operator+(const Colour& c) const;
   Colour operator-(const Colour& c) const;
   Colour operator+(const double f) const;
   void operator+=(const Colour& c);
   void operator*=(const Colour& c);
   double max();

   // clamp the colour channels to 1.0
   void clamp();

   double r;
   double g;
   double b;
};

Colour operator*(double s, const Colour& c);
Colour operator/(const Colour& c, double s);

struct Material
{
   Material(Colour diffuse, Colour specular, double exp,
            Colour refractive, double refr_index, bool isDiffuse,
            bool isSpecular = false, bool light = false) :
         diffuse(diffuse), specular(specular), refractive(refractive),
         specular_exp(exp), refr_index(refr_index), isDiffuse(isDiffuse),
         isSpecular(isSpecular), light(light)
   {}
   // Probability of diffusion .
   Colour diffuse;
   // Probability of reflection.
   Colour specular;
   // Probability of refraction.
   Colour refractive;
   // Specular exponent (for Phong shading).
   double specular_exp;
   // Material colour.
   Colour col;
   // Refractive index.
   double refr_index;

   bool isDiffuse;
   bool isSpecular;
   bool light;
};

struct Intersection
{
   Intersection() : none(true), t_value(FLT_MAX)
   {}
   // Location of intersection.
   Point3D point;
   // Normal at the intersection.
   Vector3D normal;
   // Material at the intersection.
   Material* mat;
   // This is used when you need to intersect multiply objects and
   // Set to true when no intersection has occurred.
   bool none;
   // Distance to nearest intersection.
   double t_value;
   // Intersected with light?
   bool light;
};

// Ray structure.
struct Ray3D
{
   Ray3D()
   {}
   Ray3D(Point3D p, Vector3D v) : origin(p), dir(v), col(0, 0, 0)
   {}
   // Origin and direction of the ray.
   Point3D origin;
   Vector3D dir;
   // Intersection status, should be computed by the intersection
   // function.
   Intersection intersection;
   // Current colour of the ray, should be computed by the shading
   // function.
   Colour col;
};


#endif
