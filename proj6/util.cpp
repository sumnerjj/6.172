#include <cmath>
#include "util.h"

double Vector3D::dot(const Vector3D& v) const
{
   return x*v.x + y*v.y + z*v.z;
}

Vector3D Vector3D::cross(const Vector3D& v) const
{
   Vector3D ret;
   ret.x = y * v.z - z * v.y;
   ret.y = z * v.x - x * v.z;
   ret.z = x * v.y - y * v.x;
   return ret;
}


Vector3D Vector3D::operator+(const Vector3D& v) const
{
   Vector3D ret;
   ret.x = x + v.x;
   ret.y = y + v.y;
   ret.z = z + v.z;
   return ret;
}

Vector3D Vector3D::operator-(const Vector3D& v) const
{
   Vector3D ret;
   ret.x = x - v.x;
   ret.y = y - v.y;
   ret.z = z - v.z;
   return ret;
}

Vector3D Vector3D::operator-() const
{
   Vector3D ret;
   ret.x = -x;
   ret.y = -y;
   ret.z = -z;
   return ret;
}

double Vector3D::normalize()
{
   double denom = 0.0;
   double _x = (x > 0.0) ? x : - x;
   double _y = (y > 0.0) ? y : - y;
   double _z = (z > 0.0) ? z : - z;
   if (_x > _y) {
      if (_x > _z) {
         if (1.0 + _x > 1.0) {
            _y = _y / _x;
            _z = _z / _x;
            denom = 1.0 / (_x * sqrt(1.0 + _y * _y + _z * _z));
         }
      } else { /* _z > _x > _y */
         if (1.0 + _z > 1.0) {
            _y = _y / _z;
            _x = _x / _z;
            denom = 1.0 / (_z * sqrt(1.0 + _y * _y + _x * _x));
         }
      }
   } else {
      if (_y > _z) {
         if (1.0 + _y > 1.0) {
            _z = _z / _y;
            _x = _x / _y;
            denom = 1.0 / (_y * sqrt(1.0 + _z * _z + _x * _x));
         }
      } else { /* _x < _y < _z */
         if (1.0 + _z > 1.0) {
            _y = _y / _z;
            _x = _x / _z;
            denom = 1.0 / (_z * sqrt(1.0 + _y * _y + _x * _x));
         }
      }
   }
   if (1.0 + _x + _y + _z > 1.0) {
      *this = denom * (*this);
      return 1.0 / denom;
   }
   return 0;

}

Vector3D Vector3D::transform(Matrix4x4 mat) const
{
   Vector3D r;
   r.x = x * mat[0][0] + y * mat[0][1] + z * mat[0][2];
   r.y = x * mat[1][0] + y * mat[1][1] + z * mat[1][2];
   r.z = x * mat[2][0] + y * mat[2][1] + z * mat[2][2];
   return r;
}

Vector3D Vector3D::transformAsNormal(Matrix4x4 mat) const
{
   Vector3D r;
   r.x = x * mat[0][0] + y * mat[1][0] + z * mat[2][0];
   r.y = x * mat[0][1] + y * mat[1][1] + z * mat[2][1];
   r.z = x * mat[0][2] + y * mat[1][2] + z * mat[2][2];
   return r;
}

double Vector3D::mag()
{
   return sqrt(x * x + y * y + z * z);
}


Vector3D operator*(double s, const Vector3D& v)
{
   Vector3D ret;
   ret.x = s * v.x;
   ret.y = s * v.y;
   ret.z = s * v.z;
   return ret;
}

Point3D Point3D::operator+(const Vector3D& v) const
{
   Point3D ret;
   ret.x = x + v.x;
   ret.y = y + v.y;
   ret.z = z + v.z;
   return ret;
}

Point3D Point3D::operator-(const Vector3D& v) const
{
   Point3D ret;
   ret.x = x - v.x;
   ret.y = y - v.y;
   ret.z = z - v.z;
   return ret;
}

Vector3D Point3D::operator-(const Point3D& p) const
{
   Vector3D ret;
   ret.x = x - p.x;
   ret.y = y - p.y;
   ret.z = z - p.z;
   return ret;
}

double Point3D::operator[](int ind) const
{
   return reinterpret_cast<const double *>(this)[ind];
}

double &Point3D::operator[](int ind)
{
   return reinterpret_cast<double *>(this)[ind];
}

double Point3D::dot(const Vector3D& v) const
{
   return x*v.x + y*v.y + z*v.z;
}

double Point3D::dot(const Point3D& p) const
{
   return x*p.x + y*p.y + z*p.z;
}

double Point3D::distanceTo(const Point3D& p) const
{
   double dx = p.x - x;
   double dy = p.y - y;
   double dz = p.z - z;
   return sqrt(dx * dx + dy * dy + dz * dz);
}

Colour Colour::operator*(const Colour& c) const
{
   Colour ret;
   ret.r = r * c.r;
   ret.g = g * c.g;
   ret.b = b * c.b;
   return ret;
}

Colour Colour::operator+(const Colour& c) const
{
   Colour ret;
   ret.r = r + c.r;
   ret.g = g + c.g;
   ret.b = b + c.b;
   return ret;
}

Colour Colour::operator-(const Colour& c) const
{
   Colour ret;
   ret.r = r - c.r;
   ret.g = g - c.g;
   ret.b = b - c.b;
   return ret;
}

Colour Colour::operator+(const double f) const
{
   Colour ret;
   ret.r = r + f;
   ret.g = g + f;
   ret.b = b + f;
   return ret;
}

void Colour::operator+=(const Colour & c)
{
   r += c.r;
   g += c.g;
   b += c.b;
}
void Colour::operator*=(const Colour & c)
{
   r *= c.r;
   g *= c.g;
   b *= c.b;
}

Colour operator*(double s, const Colour& c)
{
   Colour ret;
   ret.r = s * c.r;
   ret.g = s * c.g;
   ret.b = s * c.b;
   return ret;
}

Colour operator/(const Colour& c, double s)
{
   Colour ret;
   double s_i = 1.0 / s;

   ret.r = c.r * s_i;
   ret.g = c.g * s_i;
   ret.b = c.b * s_i;
   return ret;
}

void Colour::clamp()
{
   r = (r < 1.0) ? r : 1.0;
   g = (g < 1.0) ? g : 1.0;
   b = (b < 1.0) ? b : 1.0;
}

double Colour::max()
{
   if (r > g) {
      if (r > b) return r;
      else return g > b ? g : b;
   } else {
      if (g > b) return g;
      else return r > b ? r : b;
   }
}

Point3D Point3D::transform(Matrix4x4 mat) const
{
   Point3D r;
   r.x = x * mat[0][0] + y * mat[0][1] + z * mat[0][2];
   r.y = x * mat[1][0] + y * mat[1][1] + z * mat[1][2];
   r.z = x * mat[2][0] + y * mat[2][1] + z * mat[2][2];
   r.x += mat[0][3];
   r.y += mat[1][3];
   r.z += mat[2][3];
   return r;
}

void initMatrix(Matrix4x4 h)
{
   int i, j;

   for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
         h[i][j] = (i == j);
}


void mulMatrix(Matrix4x4 ret, Matrix4x4 mat1, Matrix4x4 mat2)
{
   int i, j;
   Matrix4x4 r;

   for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
         r[i][j] = mat1[i][0] * mat2[0][j] +
                   mat1[i][1] * mat2[1][j] +
                   mat1[i][2] * mat2[2][j] +
                   mat1[i][3] * mat2[3][j];

   for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
         ret[i][j] = r[i][j];
}
