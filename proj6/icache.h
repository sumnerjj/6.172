#ifndef ICACHE_H
#define ICACHE_H

#include <cmath>
#include "util.h"

class ICache
{
private:
   struct Sample
   {
      Point3D pos;
      Vector3D norm;
      double invR0;
      double r0;
      double tolerance;
      Colour irr;
      bool has_irr;
      Sample *next;

      Sample(Point3D& pos, Vector3D& norm);
      Sample(Point3D& pos, Vector3D& norm, double r0, Colour& irr, double tolerance);
      ~Sample();
      double weight(Sample& x);
      Colour getIrradiance(Sample& x);
   };

   Sample *first;
   double tolerance;
   double invTolerance;
   double minSpacing;
   double maxSpacing;
   double find(Sample& x);

   friend struct ICache::Sample;

public:

   ICache(double tolerance, double minSpacing);
   ~ICache();
   void insert(Point3D& p, Vector3D& n, double r0, Colour& irr);
   bool getIrradiance(Point3D& p, Vector3D& n, Colour *c);
};

#endif
