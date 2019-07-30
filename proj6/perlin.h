#ifndef PERLIN_H
#define PERLIN_H

#include <stdlib.h>
#include <stdio.h>

#define SAMPLE_SIZE 1024

class Perlin
{
public:
   Perlin(int octaves, double freq, double amp, int seed);

   double Get(double x, double y, double time)
   {
      double vec[3];
      vec[0] = x;
      vec[1] = y;
      vec[2] = time;
      double ret = perlin_noise_3D(vec);
      return ret;
   };

private:
   void init_perlin(int n, double p);
   double perlin_noise_2D(double vec[2]);
   double perlin_noise_3D(double vec[3]);

   double noise1(double arg);
   double noise2(double vec[2]);
   double noise3(double vec[3]);
   void normalize2(double v[2]);
   void normalize3(double v[3]);
   void init(void);

   int   mOctaves;
   double mFrequency;
   double mAmplitude;
   int   mSeed;

   int p[SAMPLE_SIZE + SAMPLE_SIZE + 2];
   double g3[SAMPLE_SIZE + SAMPLE_SIZE + 2][3];
   double g2[SAMPLE_SIZE + SAMPLE_SIZE + 2][2];
   double g1[SAMPLE_SIZE + SAMPLE_SIZE + 2];
   bool  mStart;
};

#endif
