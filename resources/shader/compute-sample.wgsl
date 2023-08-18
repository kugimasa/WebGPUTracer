const kPI = 3.14159265359;
const k_1_PI = 0.318309886184;
const kNoHit = 0xffffffffu;
const kFovy = 40.0f;
const kYup = vec3f(0.0, 1.0, 0.0);
const kNumReflectionRays = 5;
const kRayMin = 0.0001;
const kRayMax = 1e20;
const kBG = vec3f(0.2,0.2, 0.2);

struct Ray {
  start : vec4f,
  dir : vec4f,
  aspect : f32,
  time : f32,
  seed : u32,
};

/// shape: tri(0), quad(1), sphere(2)
struct HitInfo {
  dist : f32,
  shape : u32,
  id : u32,
  pos : vec3f,
  norm : vec3f,
  uv : vec2f,
};

struct Tri {
  vert : vec4f,
  e1 : vec4f,
  e2 : vec4f,
  norm : vec4f,
  color : vec3f,
  emissive : f32,
};

struct Quad {
  norm : vec4f,
  right : vec4f,
  up : vec4f,
  color : vec3f,
  emissive : f32,
};

struct Sphere {
  center : vec3f,
  radius : f32,
  color : vec3f,
  emissive : f32,
};

fn point_at(r : Ray, t : f32) -> vec3f {
  return r.start.xyz + t * r.dir.xyz;
}

fn face_norm(r : Ray, norm : vec3f) -> vec3f {
  return select(-norm, norm, dot(r.dir.xyz, norm) < 0.0);
}

fn sphere_uv(norm : vec3f) -> vec2f {
  let theta = acos(-norm.y);
  let phi = atan2(-norm.z, norm.x) + kPI;
  let u = phi * k_1_PI * 0.5;
  let v = theta * k_1_PI;
  return vec2f(u, v);
}

// random function from
// https://compute.toys/view/145
var<private> seed : u32;
fn rand() -> f32 {
    seed = seed * 747796405u + 2891336453u;
    let word = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    return f32((word >> 22u) ^ word) * bitcast<f32>(0x2f800004u);
}

fn rand_unit_sphere() -> vec3f {
    var u = rand();
    var v = rand();
    var theta = u * 2.0 * kPI;
    var phi = acos(2.0 * v - 1.0);
    var r = pow(rand(), 1.0/3.0);
    var sin_theta = sin(theta);
    var cos_theta = cos(theta);
    var sin_phi = sin(phi);
    var cos_phi = cos(phi);
    var x = r * sin_phi * sin_theta;
    var y = r * sin_phi * cos_theta;
    var z = r * cos_phi;
    return vec3f(x, y, z);
}

@group(0) @binding(0) var<uniform> ray : Ray;
@group(1) @binding(0) var<storage> tris : array<Tri>;
@group(1) @binding(1) var<storage> quads : array<Quad>;
@group(1) @binding(2) var<storage> spheres : array<Sphere>;

fn setup_camera_ray(uv: vec2f) -> Ray {
    let theta = radians(kFovy);
    let half_h = tan(theta * 0.5);
    let half_w = ray.aspect * half_h;
    let w = normalize(ray.start.xyz - ray.dir.xyz);
    let u = normalize(cross(kYup, w));
    let v = cross(w, u);
    let lower_left_corner = ray.start.xyz - half_w * u - half_h * v - w;
    let horizontal = 2.0 * half_w * u;
    let vertical = 2.0 * half_h * v;
    let ray_dir = vec4(ray.start.xyz - (lower_left_corner + uv.x * horizontal + uv.y * vertical), 0.0);
    return Ray(ray.start, ray_dir, ray.aspect, ray.time, ray.seed);
}

fn raytrace(r : Ray) -> HitInfo {
  var hit = HitInfo();
  hit.dist = kRayMax;
  hit.shape = kNoHit;
  hit.id = kNoHit;
  for (var tri = 0u; tri < arrayLength(&tris); tri++) {
    hit = intersect_tri(r, tri, hit);
  }
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    hit = intersect_quad(r, quad, hit);
  }
  for (var sphere = 0u; sphere < arrayLength(&spheres); sphere++) {
    hit = intersect_sphere(r, sphere, hit);
  }
  return hit;
}

/// Möller–Trumbore intersection algorithm
fn intersect_tri(r : Ray, id : u32, closest : HitInfo) -> HitInfo {
  var hit = false;
  // 一時変数に格納
  let tri = tris[id];
  let start = r.start.xyz;
  let dir = r.dir.xyz;
  let vert = tri.vert.xyz;
  let e1 = tri.e1.xyz;
  let e2 = tri.e2.xyz;
  let p_vec = cross(dir, e2);
  let det = dot(e1, p_vec);
  let inv_det = 1.0 / det;
  // 交差判定
  let t_vec = start - vert;
  let u = dot(t_vec, p_vec) * inv_det;
  let q_vec = cross(t_vec, e1);
  let v = dot(dir, q_vec) * inv_det;
  let t = dot(e2, q_vec) * inv_det;
  let pos = point_at(r, t);
  let norm = face_norm(r, tri.norm.xyz);
  let ray_dist = distance(pos, start);
  hit = ray_dist < closest.dist &&
        (0.0 <= u && u <= 1.0) &&
        (0.0 <= v && (u + v) <= 1.0);

  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.shape, 0u, hit),
    select(closest.id, id, hit),
    select(closest.pos, pos, hit),
    select(closest.norm, norm, hit),
    closest.uv,
  );
}

fn intersect_quad(r : Ray, id : u32, closest : HitInfo) -> HitInfo {
  let q = quads[id];
  let plane_dist = dot(q.norm, vec4(r.start.xyz, 1.0));
  let ray_dist = plane_dist / -dot(q.norm.xyz, r.dir.xyz);
  let pos = r.start.xyz + r.dir.xyz * ray_dist;
  let uv = vec2f(dot(vec4f(pos, 1.0), q.right), dot(vec4f(pos, 1.0), q.up)) * 0.5 + 0.5;
  let hit = plane_dist > 0.0 &&
            ray_dist > 0.0 &&
            ray_dist < closest.dist &&
            all((uv > vec2f()) & (uv < vec2f(1.0)));
  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.shape, 1u, hit),
    select(closest.id, id, hit),
    select(closest.pos, pos, hit),
    select(closest.norm, q.norm.xyz, hit),
    select(closest.uv, uv, hit),
  );
}

fn intersect_sphere(r : Ray, id : u32, closest : HitInfo) -> HitInfo {
  let sphere = spheres[id];
  let oc = r.start.xyz - sphere.center;
  let dir = r.dir.xyz;
  let a = dot(dir, dir);
  let half_b = dot(oc, dir);
  let c = dot(oc, oc) - sphere.radius * sphere.radius;
  let discriminant = half_b * half_b - a * c;
  let sqrt_d = sqrt(discriminant);
  // 最近傍のrootを探す
  var root = (-half_b - sqrt_d) / a;
  if (root < kRayMin || kRayMax < root) {
    root = (-half_b + sqrt_d) / a;
  }
  let pos = point_at(r, root);
  let norm = face_norm(r, (pos - sphere.center) / sphere.radius);
  let uv = sphere_uv(norm);
  let ray_dist = distance(pos, r.start.xyz);
  let hit = ray_dist < closest.dist &&
            discriminant >= 0.0;
  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.shape, 2u, hit),
    select(closest.id, id, hit),
    select(closest.pos, pos, hit),
    select(closest.norm, pos, hit),
    select(closest.uv, uv, hit),
  );
}

fn sample_hit(hit : HitInfo) -> vec3f {
  var hit_color = kBG;
  switch (hit.shape) {
    // Tri
    case 0u: {
      let tri = tris[hit.id];
      hit_color = tri.color;
    }
    // Quad
    case 1u: {
      let quad = quads[hit.id];
      hit_color = quad.color;
    }
    // Sphere
    case 2u: {
      let sphere = spheres[hit.id];
      hit_color = sphere.color;
    }
    // 背景
    default : {
      hit_color = kBG;
    }
  }
  return hit_color;
}

@group(2) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(2) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  let screen_size = vec2u(textureDimensions(frameBuffer));
  if (all(invocation_id.xy < screen_size)) {
    seed = invocation_id.x + invocation_id.y * screen_size.x + u32(ray.seed) * screen_size.x * screen_size.y;;
    let uv = vec2f(invocation_id.xy) / vec2f(screen_size);
    let r = setup_camera_ray(uv);
    let hit = raytrace(r);
    var hit_color = sample_hit(hit);
    hit_color = vec3(rand(), rand(), rand());
    textureStore(frameBuffer, invocation_id.xy, vec4(hit_color, 1.0));
  }
}