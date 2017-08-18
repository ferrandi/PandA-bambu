#include <math.h>

// Print data at the exit point
void print(int x);
void print2(int arg0, int arg1);
// Used to simulate set_pixel x,y,color
void print3(int arg0, int arg1, int arg2);

typedef float float3 __attribute__((vector_size(16)));
typedef float float4 __attribute__((vector_size(16)));

float dot3f(float3 a, float3 b)
{
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Similar to OpenCL, there are
// built-in vector types float1,...,float4
// and functions like dot3f for dot-product
struct Node;
struct Intersection;

// Stores one object of the scene
struct Node
{
  // position
  float4 pos;

  // intersection function
  struct Intersection (*intersect)(struct Node *node, float3 start, float3 dir);

  // left child node
  int left;

  // right child node
  int right;

  // color rgb
  float3 color;
};

// Represents an intersection between a
// ray and the scene
struct Intersection
{
  float dist;   // distance from the start
  float3 color; // color at this position
};

// intersection functions for all three types
struct Intersection plane_intersect
(struct Node *node, float3 start, float3 dir);
struct Intersection sphere_intersect
(struct Node *node, float3 start, float3 dir);
struct Intersection node_intersect
(struct Node *node, float3 start, float3 dir);

// raytracing function
float3 raytrace_color(float3 start, float3 dir);

// Array containing scene objects
struct Node nodes[7] =
{
  { (float4){0,1,0,0}, plane_intersect,
    0, 0, (float3){1,0,0} },
  { (float4){0,1,2,1}, sphere_intersect,
    0, 0, (float3){0,0,1} },
  { (float4){1,0,0,1}, node_intersect,
    0, 1, (float3){1,1,1} },
  { (float4){-2,1,2,1}, sphere_intersect,
    0, 0, (float3){0,1,0} },
  { (float4){1,0,0,1}, node_intersect,
    3, 2, (float3){1,1,1} },
  { (float4){2,1,2,1}, sphere_intersect,
    0, 0, (float3){0,1,1} },
  { (float4){1,0,0,1}, node_intersect,
    4, 5, (float3){1,1,1} }
};

// Helper function to construct an
// intersection structure
struct Intersection intersection
(float dist, float3 color)
{
  struct Intersection result;
  result.dist = dist;
  result.color = color;
  return result;
}

// Generates a checkerboard pattern mapped
// onto the XY plane
float3 get_color(float3 pos)
{
  int ix = fabs(pos[0]);
  int iy = fabs(pos[1]);
  int iz = fabs(pos[2]);
  return ((ix ^ iz) & 1) ?
      (float3){1,0,0} :
      (float3){0.25,0,0};
}

// Calculates intersection between a
// plane and the ray start + t * dir
struct Intersection plane_intersect
(struct Node *node, float3 start, float3 dir)
{
  // Solves the following system:
  // ray(t) = start + t * dir
  // plane: dot(plane.xyz, x) + plane.w = 0

  float4 plane = node->pos;
  float n_dot_dir = dot3f(plane, dir);
  float n_dot_start = dot3f(plane, start);

  // Solve for parameter t of the ray
  // We ignore the case n_dot_dir==0
  float t = (-plane[3] - n_dot_start)
            / n_dot_dir;
  // p is the intersection point
  float3 p = start + t * dir;
  // Calculate color at position p
  float3 c = get_color(p);

  return intersection(t, c);
}

// Calculates intersection between a
// sphere and the ray start + t * dir
struct Intersection sphere_intersect
(struct Node *node, float3 start, float3 dir)
{
  float4 pos = node->pos;

  // The position vector contains the center
  // the sphere and the squared radius in w
  float3 center = pos;
  float squared_radius = pos[3];

  // Solves the following system
  // ray(t) = start + t * dir
  // sphere: (p-center)^2 == squared_radius
  float3 v = start - center;

  float a = dot3f(dir, dir);
  float b = 2 * dot3f(v, dir);
  float c = dot3f(v,v) - squared_radius;
  // Determinant of the quadratic equation
  float det = b * b - 4 * a * c;
  // There is not intersection point
  if (det < 0)
  {
    return intersection(det, (float3){0,0,0});
  }

  // One or two intersection points exists
  float s = sqrt(det);

  // Calculate parameter t of both points
  float t0 = (-b + s) / (2 * a);
  float t1 = (-b - s) / (2 * a);

  // Choose nearest intersection point
  float dist = t0 < t1 ? t0 : t1;

  // Calculate actual position
  float3 n = v + dist * dir;
  // Perform simple lighting
  float diffuse = fabs(n[0] - n[1] + n[2]);

  // Clamp diffuse to [0..1]
  diffuse = diffuse < 0 ? 0 : diffuse;
  diffuse = diffuse > 1.0 ? 1.0 : diffuse;

  // Multiply diffuse light with sphere color
  return intersection(dist,
                      diffuse * node->color);
}

// Intersection function for scene node
// Invokes intersection function of it childs
// and returns the nearest intersection point
struct Intersection node_intersect
(struct Node *node, float3 start, float3 dir)
{
  struct Intersection a, b;
  int left = node->left;
  int right = node->right;

  // Call intersect on child nodes
  a = nodes[left].
      intersect(&nodes[left], start, dir);
  b = nodes[right].
      intersect(&nodes[right], start, dir);

  if (a.dist < 0)
    return b;

  if (b.dist < 0)
    return a;

  return (a.dist < b.dist) ? a : b;
}

// Calculates the direction of the ray
// at screen position (x,y)
float3 view_vec(int x, int y)
{
  float vx = (2.0/31.0) * x - 1.0;
  float vy = (-2.0/31.0) * y + 1.0;
  return (float3){vx, vy, 1};
}

// Packs floating point RGB color
// into integer value
int rgb(float r, float g, float b)
{
  return (int)(r * 255)
      + ((int)(g * 255) << 8)
      + ((int)(b * 255) << 16);
}

// Sends a ray into the scene and returns
// the color of the nearest intersection
// or black if no intersection was found
float3 raytrace_color
(float3 start, float3 dir)
{
  struct Intersection i, i0, i1;
  i = nodes[6].intersect
      (&nodes[6], start, dir);
  return i.dist < 0.0
      ? (float3){0,0,0}
      : i.color;
}

// entry function invoked by the test bench
// Calculates the color for pixel (x,y)
void raytrace(int x, int y)
{
  // Get ray direction for pixel (x,y)
  float3 v = view_vec(x,y);

  // Camera position is (0,1,0)
  float3 start = (float3){0,1,0};

  float3 color = raytrace_color(start, v);

  // Convert color into packed integer
  int c = rgb(color[0], color[1], color[2]);

  // Set color of pixel (x,y) to c
  print3(x,y, c);
}
