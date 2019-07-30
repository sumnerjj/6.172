#include "raytracer.h"
#include "bmp_io.h"
#include "random.h"
#include <cmath>
#include <cstring>
#include <iostream>

Raytracer::Raytracer(bool soft_shadows, bool direct_illumination,
                     bool global_illumination, bool caustics)
      : _lightSource(NULL)
{
   _root = new SceneDagNode();

   this->soft_shadows = soft_shadows;
   this->direct_illumination = direct_illumination;
   this->global_illumination = global_illumination;
   this->caustics = caustics;

   initMatrix(_modelToWorld);
   initMatrix(_worldToModel);
}

Raytracer::~Raytracer()
{
   delete _root;
   delete _lightSource;
}

SceneDagNode* Raytracer::addObject(SceneDagNode* parent, SceneObject* obj)
{
   SceneDagNode* node = new SceneDagNode(obj);
   node->parent = parent;
   node->next = NULL;
   node->child = NULL;

   // Add the object to the parent's child list, this means
   // whatever transformation applied to the parent will also
   // be applied to the child.
   if (parent->child == NULL) {
      parent->child = node;
   } else {
      parent = parent->child;
      while (parent->next != NULL) {
         parent = parent->next;
      }
      parent->next = node;
   }

   return node;
}

LightListNode* Raytracer::addLightSource(LightSource* light)
{
   LightListNode* tmp = _lightSource;
   _lightSource = new LightListNode(light, tmp);
   return _lightSource;
}

void Raytracer::rotate(SceneDagNode* node, char axis, double angle)
{
   Matrix4x4 rotation;
   double toRadian = 2 * M_PI / 360.0;
   int i;

   initMatrix(rotation);
   for (i = 0; i < 2; i++) {
      switch (axis) {
      case 'x':
         rotation[0][0] = 1;
         rotation[1][1] = cos(angle * toRadian);
         rotation[1][2] = -sin(angle * toRadian);
         rotation[2][1] = sin(angle * toRadian);
         rotation[2][2] = cos(angle * toRadian);
         rotation[3][3] = 1;
         break;
      case 'y':
         rotation[0][0] = cos(angle * toRadian);
         rotation[0][2] = sin(angle * toRadian);
         rotation[1][1] = 1;
         rotation[2][0] = -sin(angle * toRadian);
         rotation[2][2] = cos(angle * toRadian);
         rotation[3][3] = 1;
         break;
      case 'z':
         rotation[0][0] = cos(angle * toRadian);
         rotation[0][1] = -sin(angle * toRadian);
         rotation[1][0] = sin(angle * toRadian);
         rotation[1][1] = cos(angle * toRadian);
         rotation[2][2] = 1;
         rotation[3][3] = 1;
         break;
      }
      if (i == 0) {
         mulMatrix(node->trans, node->trans, rotation);
         angle = -angle;
      } else {
         mulMatrix(node->invtrans, rotation, node->invtrans);
      }
   }
}

void Raytracer::translate(SceneDagNode* node, Vector3D trans)
{
   Matrix4x4 translation;

   initMatrix(translation);
   translation[0][3] = trans.x;
   translation[1][3] = trans.y;
   translation[2][3] = trans.z;
   mulMatrix(node->trans, node->trans, translation);
   translation[0][3] = -trans.x;
   translation[1][3] = -trans.y;
   translation[2][3] = -trans.z;
   mulMatrix(node->invtrans, translation, node->invtrans);
   if (node->obj)
      node->obj->ident = false;
}

void Raytracer::scale(SceneDagNode* node, Point3D origin, double factor[3])
{
   Matrix4x4 scale;

   initMatrix(scale);
   scale[0][0] = factor[0];
   scale[0][3] = origin.x - factor[0] * origin.x;
   scale[1][1] = factor[1];
   scale[1][3] = origin.y - factor[1] * origin.y;
   scale[2][2] = factor[2];
   scale[2][3] = origin.z - factor[2] * origin.z;
   mulMatrix(node->trans, node->trans, scale);
   scale[0][0] = 1 / factor[0];
   scale[0][3] = origin.x - 1 / factor[0] * origin.x;
   scale[1][1] = 1 / factor[1];
   scale[1][3] = origin.y - 1 / factor[1] * origin.y;
   scale[2][2] = 1 / factor[2];
   scale[2][3] = origin.z - 1 / factor[2] * origin.z;
   mulMatrix(node->invtrans, scale, node->invtrans);
   if (node->obj)
      node->obj->ident = false;
}

void Raytracer::initInvViewMatrix(Matrix4x4 mat, Point3D eye, Vector3D view,
                                  Vector3D up)
{
   Vector3D w;
   view.normalize();
   up = up - up.dot(view) * view;
   up.normalize();
   w = view.cross(up);

   initMatrix(mat);
   mat[0][0] = w.x;
   mat[1][0] = w.y;
   mat[2][0] = w.z;
   mat[0][1] = up.x;
   mat[1][1] = up.y;
   mat[2][1] = up.z;
   mat[0][2] = -view.x;
   mat[1][2] = -view.y;
   mat[2][2] = -view.z;
   mat[0][3] = eye.x;
   mat[1][3] = eye.y;
   mat[2][3] = eye.z;
}

void Raytracer::traverseEntireScene(Ray3D& ray, bool casting)
{
   traverseScene(_root, ray , casting);
}

void Raytracer::traverseScene(SceneDagNode* node, Ray3D& ray, bool casting)
{
   SceneDagNode *childPtr;

   // Applies transformation of the current node to the global
   // transformation matrices.
   if (node->obj && !node->obj->ident) {
      mulMatrix(_modelToWorld, _modelToWorld, node->trans);
      mulMatrix(_worldToModel, node->invtrans, _worldToModel);
   }

   if (node->obj) {
      // Perform intersection.
      if (casting) {
         if (!node->obj->light)
            node->obj->intersect(ray, _worldToModel, _modelToWorld);
      } else
         node->obj->intersect(ray, _worldToModel, _modelToWorld);
   }
   // Traverse the children.
   childPtr = node->child;
   while (childPtr != NULL) {
      traverseScene(childPtr, ray, casting);
      childPtr = childPtr->next;
   }

   // Removes transformation of the current node from the global
   // transformation matrices.
   if (node->obj && !node->obj->ident) {
      mulMatrix(_worldToModel, node->trans, _worldToModel);
      mulMatrix(_modelToWorld, _modelToWorld, node->invtrans);
   }
}

void Raytracer::computeShading(Ray3D& ray, int depth, bool getDirectly)
{
   if (!depth) {
      ray.col = default_col;
      return;
   }

   ray.intersection.normal.normalize();

   LightListNode* curLight = _lightSource;
   for (;;) {
      if (curLight == NULL) break;
      // Each light source provides its own shading function.
      curLight->light->shade(ray, getDirectly);
      curLight = curLight->next;
   }

   if (ray.intersection.mat->isDiffuse) {
      ray.col.clamp();
      return;
   }

   double n;

   if (ray.intersection.mat->refractive.max() > 0.01) {

      if (ray.dir.dot(ray.intersection.normal) < 0) {
         n = 1.0 / ray.intersection.mat->refr_index;
      } else {
         ray.intersection.normal = -ray.intersection.normal;
         n = ray.intersection.mat->refr_index;
      }

      double cosI = ray.intersection.normal.dot(ray.dir);
      double sinT2 = n * n * (1.0 - cosI * cosI);

      if (sinT2 < 1.0) {
         Vector3D T = n * ray.dir - (n * cosI + sqrt(1.0 - sinT2)) * ray.intersection.normal;

         Ray3D new_ray(ray.intersection.point, T);
         traverseEntireScene(new_ray, false);

         if (!new_ray.intersection.none)
            computeShading(new_ray, depth - 1, getDirectly);
         else
            new_ray.col = default_col;

         ray.col = ray.col + ray.intersection.mat->refractive * new_ray.col;
      } else {
         // Total internal reflection
         Vector3D R = ray.dir - (2 * ray.dir.dot(ray.intersection.normal)) *
                      ray.intersection.normal;

         Ray3D new_ray(ray.intersection.point, R);
         traverseScene(_root, new_ray, false);

         if (!new_ray.intersection.none)
            computeShading(new_ray, depth - 1, getDirectly);
         else
            new_ray.col = default_col;

         ray.col = ray.col + ray.intersection.mat->refractive * new_ray.col;
      }
   }

   // Raytrace reflections
   if (ray.dir.dot(ray.intersection.normal) < 0) {
      Vector3D R = ray.dir - (2 * ray.dir.dot(ray.intersection.normal)) *
                   ray.intersection.normal;

      Ray3D new_ray(ray.intersection.point, R);
      traverseScene(_root, new_ray, false);

      if (!new_ray.intersection.none)
         computeShading(new_ray, depth - 1, getDirectly);
      else
         new_ray.col = default_col;

      ray.col = ray.col + ray.intersection.mat->specular * new_ray.col;
   }

   ray.col.clamp();
}


void Raytracer::initPixelBuffer()
{
   int numbytes = _scrWidth * _scrHeight * sizeof(unsigned char);
   _rbuffer = new unsigned char[numbytes];
   _gbuffer = new unsigned char[numbytes];
   _bbuffer = new unsigned char[numbytes];
   for (int i = 0; i < _scrHeight; i++) {
      for (int j = 0; j < _scrWidth; j++) {
         _rbuffer[i*_scrWidth+j] = int(default_col.r * 255);
         _gbuffer[i*_scrWidth+j] = int(default_col.r * 255);
         _bbuffer[i*_scrWidth+j] = int(default_col.r * 255);
      }
   }
}

void Raytracer::flushPixelBuffer(const char *file_name)
{
   bmp_write(file_name, _scrWidth, _scrHeight, _rbuffer, _gbuffer, _bbuffer);
   delete [] _rbuffer;
   delete [] _gbuffer;
   delete [] _bbuffer;
}

void Raytracer::render(int width, int height, Point3D eye, Vector3D view,
                       Vector3D up, double fov, const char* fileName)
{
   Matrix4x4 viewToWorld;
   _scrWidth = width;
   _scrHeight = height;

   default_col = Colour(0.4, 0.4, 0.4);

   initPixelBuffer();
   initInvViewMatrix(viewToWorld, eye, view, up);

   // Sets up ray origin and direction in view space,
   // image plane is at z = -1.
   Point3D origin(0, 0, 150);
   Point3D imagePlane;
   imagePlane.z = 49.9;
   int progress = 0;

   cout << "Rendering: 0%% ";

   // Construct a ray for each pixel.
   for (int i = 0; i < _scrHeight; i++) {
      imagePlane.y = (99 * i / double(height) - 49.5);
      for (int j = 0; j < _scrWidth; j++) {
         imagePlane.x = (99 * j / double(width) - 49.5);

         // Initialize ray with the proper origin and direction.
         Vector3D dir = imagePlane - origin;
         dir.normalize();

         Ray3D ray(imagePlane, dir);

         traverseScene(_root, ray, false);

         // Don't bother shading if the ray didn't hit anything.
         if (!ray.intersection.none) {
            computeShading(ray, 6, false);
            _rbuffer[i*width+j] = int(ray.col.r * 255);
            _gbuffer[i*width+j] = int(ray.col.g * 255);
            _bbuffer[i*width+j] = int(ray.col.b * 255);
         }

         printProgress((int) (((++progress + 1) * 100.0 /
                               (_scrWidth * _scrHeight))));
      }
   }
   cout << endl;
   flushPixelBuffer(fileName);
}

void Raytracer::printProgress(int percent)
{
   printf("\b\b\b\b");
   cout << percent << "%";
   if (percent < 10) {
      cout << " ";
   }
   if (percent < 100) {
      cout << " ";
   }
   fflush(stdout);
}
