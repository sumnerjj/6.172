#include <math.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

#include "photonmap.h"
#include "config.h"

/* This structure is used only to locate the
 * nearest photons */
struct NearestPhotons
{
   int max;
   int found;
   Point3D pos;
   Vector3D normal;
   double *dist2;
   const Photon **index;
};

PhotonMap *createPhotonMap(int max_photons)
{
   PhotonMap *map = new PhotonMap();
   map->stored_photons = 0;
   map->prev_scale = 1;
   map->max_photons = max_photons;

   map->photons = new Photon[max_photons + 1];

   if (map->photons == NULL) {
      fprintf(stderr, "Out of memory initializing photon map\n");
      exit(-1);
   }

   map->bbox_min[0] = map->bbox_min[1] = map->bbox_min[2] = 1e8f;
   map->bbox_max[0] = map->bbox_max[1] = map->bbox_max[2] = -1e8f;

   return map;
}

void destroyPhotonMap(BalancedPhotonMap *map)
{
   delete [] map->photons;
   delete map;
}


/* photonDir returns the direction of a photon
 * at a given surface position */
static void photonDir(Vector3D *dir, const Photon *p)
{
   dir->x = sin(p->theta) * cos(p->phi * 2);
   dir->y = sin(p->theta) * sin(p->phi * 2);
   dir->z = cos(p->theta);
}


/* Store puts a photon into the flat array that will form
 * the final kd-tree.
 *
 * Call this function to store a photon.  */
void storePhoton(
   PhotonMap *map,
   const Colour &power,
   const Point3D &pos,
   const Vector3D &dir)
{
   int i;
   Photon *node;
   if (map->stored_photons >= map->max_photons) {
      Photon *newMap = (Photon *) realloc(map->photons, sizeof(Photon) *
                                          (2 * map->max_photons + 1));
      if (newMap == NULL) {
         static int done = 0;
         if (!done)
            fprintf(stderr, "Photon Map Full\n");
         done = 1;
         return;
      }
      map->photons = newMap;
      map->max_photons *= 2;
   }

   map->stored_photons++;
   node = &map->photons[map->stored_photons];

   for (i = 0; i < 3; i++) {
      node->pos[i] = pos[i];

      if (node->pos[i] < map->bbox_min[i])
         map->bbox_min[i] = node->pos[i];
      if (node->pos[i] > map->bbox_max[i])
         map->bbox_max[i] = node->pos[i];

   }
   node->power = power;
   node->theta = acos(dir.z);
   node->phi = atan2(dir.y, dir.x) / 2;
   if (node->phi < 0)
      node->phi += M_PI;
}

/* scalePhotonPower is used to scale the power of all
 * photons once they have been emitted from the light
 * source. scale = 1/(#emitted photons).
 * Call this function after each light source is processed.  */
void scalePhotonPower(PhotonMap *map,
                      const double scale)
{
   int i;
   for (i = map->prev_scale; i <= map->stored_photons; i++) {
      map->photons[i].power *= scale;
   }
   map->prev_scale = map->stored_photons;
}

// MedianSplit splits the photon array into two separate
// pieces around the median with all photons below the
// the median in the lower half and all photons above
// than the median in the upper half. The comparison
// criteria is the axis (indicated by the axis parameter)
// (inspired by routine in "Algorithms in C++" by Sedgewick)
static void medianSplit(
   Photon **p,
   const int start,               // start of photon block in array
   const int end,                 // end of photon block in array
   const int median,              // desired median number
   const int axis)                // axis to split along
{
#define swap(ph,a,b) { Photon *ph2=ph[a]; ph[a]=ph[b]; ph[b]=ph2; }
   int left = start;
   int right = end;

   while (right > left) {
      const double v = p[right]->pos[axis];
      int i = left - 1;
      int j = right;
      for (;;) {
         while (p[++i]->pos[axis] < v);
         while (p[--j]->pos[axis] > v && j > left);
         if (i >= j)
            break;
         swap(p, i, j);
      }

      swap(p, i, right);
      if (i >= median)
         right = i - 1;
      if (i <= median)
         left = i + 1;
   }
}

// See "Realistic image synthesis using Photon Mapping" chapter 6
// for an explanation of this function
static void balanceSegment(
   PhotonMap *map,
   Photon **pbal,
   Photon **porg,
   const int index,
   const int start,
   const int end)
{
   // Compute new median
   int axis;
   int median = 1;

   while ((4 * median) <= (end - start + 1))
      median += median;

   if ((3 * median) <= (end - start + 1)) {
      median += median;
      median += start - 1;
   } else
      median = end - median + 1;

   // Find axis to split along
   axis = 2;
   if ((map->bbox_max[0] - map->bbox_min[0]) >
       (map->bbox_max[1] - map->bbox_min[1]) &&
       (map->bbox_max[0] - map->bbox_min[0]) >
       (map->bbox_max[2] - map->bbox_min[2])) {
      axis = 0;
   } else if ((map->bbox_max[1] - map->bbox_min[1]) >
              (map->bbox_max[2] - map->bbox_min[2])) {
      axis = 1;
   }

   // Partition photon block around the median
   medianSplit(porg, start, end, median, axis);

   pbal[index] = porg[median];
   pbal[index]->plane = axis;

   // Recursively balance the left and right block
   if (median > start) {
      // Balance left segment
      if (start < median - 1) {
         const double tmp = map->bbox_max[axis];
         map->bbox_max[axis] = pbal[index]->pos[axis];
         balanceSegment(map, pbal, porg, 2 * index, start, median - 1);
         map->bbox_max[axis] = tmp;
      } else {
         pbal[2 * index] = porg[start];
      }
   }

   if (median < end) {
      // Balance right segment
      if (median + 1 < end) {
         const double tmp = map->bbox_min[axis];
         map->bbox_min[axis] = pbal[index]->pos[axis];
         balanceSegment(map, pbal, porg, 2 * index + 1, median + 1, end);
         map->bbox_min[axis] = tmp;
      } else {
         pbal[2 * index + 1] = porg[end];
      }
   }
}

/* Balance creates a left balanced kd-tree from the flat photon array.
 * This function should be called before the photon map
 * is used for rendering.  */
BalancedPhotonMap *balancePhotonMap(PhotonMap *map)
{
   BalancedPhotonMap *bmap;
   if (map->stored_photons > 1) {
      int i;
      int d, j, foo;
      Photon foo_photon;

      // Allocate two temporary arrays for the balancing procedure
      Photon **pa1 = new Photon*[map->stored_photons + 1];
      Photon **pa2 = new Photon*[map->stored_photons + 1];

      for (i = 0; i <= map->stored_photons; i++)
         pa2[i] = &map->photons[i];

      balanceSegment(map, pa1, pa2, 1, 1, map->stored_photons);
      delete [] pa2;

      // Reorganize balanced kd-tree (make a heap)
      j = 1;
      foo = 1;
      foo_photon = map->photons[j];

      for (i = 1; i <= map->stored_photons; i++) {
         d = pa1[j] - map->photons;
         pa1[j] = NULL;
         if (d != foo)
            map->photons[j] = map->photons[d];
         else {
            map->photons[j] = foo_photon;

            if (i < map->stored_photons) {
               for (;foo <= map->stored_photons; foo++)
                  if (pa1[foo] != NULL)
                     break;
               foo_photon = map->photons[foo];
               j = foo;
            }
            continue;
         }
         j = d;
      }
      delete [] pa1;
   }

   bmap = new BalancedPhotonMap();
   bmap->stored_photons = map->stored_photons;
   bmap->half_stored_photons = map->stored_photons / 2 - 1;
   bmap->photons = map->photons;
   delete map;

   return bmap;
}


/* Locate_photons finds the nearest photons in the
 * photon map given the parameters in np */
static void locatePhotons(
   BalancedPhotonMap *map,
   NearestPhotons *const np,
   const int index,
   const Vector3D &normal)
{
   const Photon *p = &map->photons[index];
   double dist1;
   double distx1;
   double disty1;
   double distz1;
   double dist2;
   double discFix;

   if (index < map->half_stored_photons) {

      dist1 = np->pos[p->plane] - p->pos[p->plane];

      // If dist1 is positive search right plane
      if (dist1 > 0.0) {
         locatePhotons(map, np, 2 * index + 1, normal);
         if (dist1 * dist1 < np->dist2[0])
            locatePhotons(map, np, 2 * index , normal);
         // Else, dist1 is negative search left first
      } else {
         locatePhotons(map, np, 2 * index , normal);
         if (dist1 * dist1 < np->dist2[0])
            locatePhotons(map, np, 2 * index + 1 , normal);
      }
   }

   // Compute squared distance between current photon and np->pos
   dist2 = (p->pos - np->pos).mag() * (p->pos - np->pos).mag();

   // Adjust the distance for photons that are not on the same plane as this
   // point.
   distx1 = p->pos.x - np->pos.x;
   disty1 = p->pos.y - np->pos.y;
   distz1 = p->pos.z - np->pos.z;
   discFix = normal.x * distx1 + normal.y * disty1 + normal.z * distz1;
   discFix = fabs(discFix);
   dist2 += discFix * dist2 * 0.010;

   if (dist2 < np->dist2[0]) {
      // We found a photon :) Insert it in the candidate list
      if (np->found < np->max) {
         // Array not full
         np->found++;
         np->dist2[np->found] = dist2;
         np->index[np->found] = p;
      } else {

         // Array full.  Have to remove the furthest photon.
         int j;
         float max_dist = -1;
         int max_index = -1;
         j = 1;
         while (j <= np->found) {
            if (np->dist2[j] > max_dist) {
               max_dist = np->dist2[j];
               max_index = j;
            }
            j++;
         }
         if (max_index != -1) {
            np->dist2[max_index] = dist2;
            np->index[max_index] = p;
         }
      }
   }
}

/* Irradiance_estimate computes an irradiance estimate
 * at a given surface position */
void irradianceEstimate(
   BalancedPhotonMap *map,
   Colour *irrad,                // Returned irradiance
   const Point3D &pos,           // Surface position
   const Vector3D &normal,       // Surface normal at pos
   const double max_dist,        // Max distance to look for photons
   const int nphotons)           // Number of photons to use
{
   NearestPhotons np;
   Vector3D pdir;
   int i;
   *irrad = Colour(0, 0, 0);

   // We used to use allocas here, but cilk doesn't like those, so we use
   // stack-allocated vectors which automatically free their storage on return.
   std::vector<double> dist2(nphotons + 1);
   std::vector<const Photon*> index(nphotons + 1);
   np.dist2 = &dist2[0];
   np.index = &index[0];
   np.pos = pos;
   np.max = nphotons;
   np.found = 0;
   np.dist2[0] = max_dist * max_dist;

   // Locate the nearest photons
   locatePhotons(map, &np, 1 , normal);

   // If less than 2 photons return
   if (np.found < 2) {
      return;
   }

   // Sum irradiance from all photons
   for (i = 1; i <= np.found; i++) {
      const Photon *p = np.index[i];
      photonDir(&pdir, p);
      if (pdir.dot(normal) < 0.0) {
         *irrad += p->power;
      }
   }

   // Take into account (estimate of) density
   *irrad *= (1.0f / M_PI) / (np.dist2[0]);
}
