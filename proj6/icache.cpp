#include <cmath>

#include "icache.h"
#include "util.h"
#include "config.h"

#define clamp(val, min, max) (val > max ? max : (val < min ? min : val))
#define min(v1, v2) (v1 < v2 ? v1 : v2)
#define max(v1, v2) (v1 > v2 ? v1 : v2)

ICache::ICache(double tolerance, double minSpacing)
{
   this->tolerance = tolerance;
   this->minSpacing = minSpacing;
   invTolerance = 1.0 / tolerance;
   maxSpacing = ICACHE_MAX_SPACING_RATIO * minSpacing;

   Point3D pos = Point3D(0.0, 0.0, 0.0);
   first = NULL;
}

ICache::~ICache()
{
   delete first;
}

void ICache::insert(Point3D& pos, Vector3D& norm, double r0, Colour& irr)
{
   r0 = clamp(r0 * tolerance, minSpacing, maxSpacing) * invTolerance;

   Sample *s = new Sample(pos, norm, r0, irr, tolerance);
   s->next = first;
   first = s;
}

bool ICache::getIrradiance(Point3D& pos, Vector3D& norm, Colour *c)
{
   Sample x = Sample(pos, norm);
   double w = find(x);

   if (x.has_irr) {
      *c = x.irr / w;
      return true;
   } else return false;
}

double ICache::find(Sample& x)
{
   double weight = 0.0;

   for (Sample *s = first; s != NULL; s = s->next) {

      double wi = s->weight(x);
      wi = min(1e10, wi);
      if (wi > invTolerance) {
         if (x.has_irr)
            x.irr += s->getIrradiance(x) * wi;
         else {
            x.irr = s->getIrradiance(x) * wi;
            x.has_irr = true;
         }

         weight += wi;
      }
   }

   return weight;
}

ICache::Sample::Sample(Point3D& pos, Vector3D& norm)
{
   this->pos = pos;
   this->norm = norm;
   this->norm.normalize();
   has_irr = false;
   next = NULL;
}

ICache::Sample::Sample(Point3D& pos, Vector3D& norm, double r0, Colour& irr, double tolerance)
{
   this->pos = pos;
   this->norm = norm;
   this->norm.normalize();
   invR0 = 1.0 / r0;
   this->r0 = r0;
   this->tolerance = tolerance;
   this->irr = irr;
   has_irr = true;
   next = NULL;
}

ICache::Sample::~Sample()
{
   delete next;
}

double ICache::Sample::weight(Sample& x)
{
   Vector3D v = pos - x.pos;

   if (fabs(v.x) < (r0 * tolerance) &&
         fabs(v.y) < (r0 * tolerance) &&
         fabs(v.z) < (r0 * tolerance)) {
      double a = x.norm.dot(norm);
      double d = x.pos.distanceTo(pos);
      double w = 1.0 / ((d * invR0) + sqrt(1.0 - a));
      return w;
   } else
      return 0;
}

Colour ICache::Sample::getIrradiance(Sample& x)
{
   return irr;
}
