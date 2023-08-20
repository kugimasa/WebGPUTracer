const kPI = 3.14159265359;
const k_1_PI = 0.318309886184;
const kNoHit = 0xffffffffu;
const kFovy = 40.0f;
const kXup = vec3f(1.0, 0.0, 0.0);
const kYup = vec3f(0.0, 1.0, 0.0);
const kZup = vec3f(0.0, 0.0, 1.0);
const kRayDepth = 5;
const kRayMin = 0.0001;
const kRayMax = 1e20;
const kSPP = 10;
const kBG = vec3f(0.2,0.2, 0.2);
const kZero = vec3f(0.0, 0.0, 0.0);
const kOne = vec3f(1.0, 1.0, 1.0);

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
  pos : vec3f,
  norm : vec3f,
  uv : vec2f,
  col : vec3f,
  flags : vec4f, // (emissive, _, _, _)
};

struct ONB {
  u : vec3f,
  v : vec3f,
  w : vec3f,
}

struct Path {
  ray : Ray,
  col : vec3f,
  end : bool,
}

struct Tri {
  vert : vec4f,
  e1 : vec4f,
  e2 : vec4f,
  norm : vec4f,
  col : vec3f,
  emissive : f32,
};

struct Quad {
  norm : vec4f,
  right : vec4f,
  up : vec4f,
  col : vec3f,
  emissive : f32,
};

struct Sphere {
  center : vec3f,
  radius : f32,
  col : vec3f,
  emissive : f32,
};

fn point_at(r: Ray, t: f32) -> vec3f {
  return r.start.xyz + t * r.dir.xyz;
}

fn face_norm(r: Ray, norm: vec3f) -> vec3f {
  return select(-norm, norm, dot(r.dir.xyz, norm) < 0.0);
}

fn sphere_uv(norm: vec3f) -> vec2f {
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

fn rand_to_sphere(radius: f32, square_dist: f32) -> vec3f {
  var r1 = rand();
  var r2 = rand();
  var z = 1.0 + r2 * (sqrt(1.0 - radius * radius / square_dist) - 1.0);
  var phi = 2.0 * kPI * r1;
  var x = cos(phi) * sqrt(1.0 - z * z);
  var y = sin(phi) * sqrt(1.0 - z * z);
  return vec3f(x, y, z);
}

fn rand_cos_dir() -> vec3f {
  var r1 = rand();
  var r2 = rand();
  var z = sqrt(1.0 - r2);
  var phi = 2.0 * kPI * r1;
  var x = cos(phi) * sqrt(r2);
  var y = sin(phi) * sqrt(r2);
  return vec3f(x, y, z);
}

fn build_onb_from_w(w: vec3f) -> ONB {
  var onb : ONB;
  onb.w = normalize(w);
  var a = select(kXup, kYup, (sign(onb.w.x) * onb.w.x) > 0.9);
  onb.v = normalize(cross(onb.w, a));
  onb.u = cross(onb.w, onb.v);
  return onb;
}

fn onb_local(onb: ONB, a: vec3f) -> vec3f {
  return a.x * onb.u + a.y * onb.v + a.z * onb.w;
}

fn sample_sphere_light_dir(pos: vec3f) -> vec3f {
  // FIXME: 決めうちでライトを取得している
  var sphere = spheres[0u];
  var dir = sphere.center - pos;
  var onb = build_onb_from_w(dir);
  var square_dist = dot(dir, dir);
  return onb_local(onb, rand_to_sphere(sphere.radius, square_dist));
}

fn sample_scatter_dir(onb: ONB) -> vec3f {
  var a = rand_cos_dir();
  return onb_local(onb, a);
}

fn sphere_pdf(pos: vec3f, dir: vec3f) -> f32 {
  // FIXME: 決めうちでライトを取得している
  var sphere = spheres[0u];
  var squared_dist = dot(sphere.center - pos, sphere.center - pos);
  var cos_theta_max = sqrt(1.0 - sphere.radius * sphere.radius / squared_dist);
  var solid_angle = 2.0 * kPI * (1.0 - cos_theta_max);
  return 1.0 / solid_angle;
}

fn cosine_pdf(onb: ONB, dir: vec3f) -> f32 {
  var cos = dot(normalize(dir), onb.w);
  return select(cos * k_1_PI, 0.0, cos <= 0.0);
}

fn scattering_pdf(norm: vec3f, dir: vec3f) -> f32 {
  var cos = dot(norm, normalize(dir));
  return select(cos * k_1_PI, 0.0, cos < 0.0);
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

fn raytrace(path: Path, depth: i32) -> Path {
  var r = path.ray;
  var hit = sample_hit(r);
  let emissive = hit.flags.x;
  // 光源の場合、トレースを終了
  if (emissive > 0.0) {
    if (depth == 0) {
      return Path(r, hit.col, true);
    }
    // 照明計算
    var ray_col = hit.col * path.col;
    return Path(r, ray_col, true);
  }
  // 反射オブジェクトの場合
  else {
    // 反射
    var onb = build_onb_from_w(hit.norm);
    // var scatter_dir = sample_scatter_dir(onb);
    // var pdf_val = cosine_pdf(onb, scatter_dir);
    var scatter_dir = sample_sphere_light_dir(hit.pos);
    var pdf_val = sphere_pdf(hit.pos, scatter_dir);
    // パスを更新
    var scattered_ray = Ray(vec4f(hit.pos, 1.0), vec4f(scatter_dir, 1.0), r.aspect, r.time, r.seed);
    var scattered_col = path.col * hit.col * scattering_pdf(hit.norm, scatter_dir) / pdf_val;
    return Path(scattered_ray, scattered_col, false);
  }
}

fn sample_hit(r: Ray) -> HitInfo {
  var hit = HitInfo();
  hit.dist = kRayMax;
  hit.col = kBG;
  hit.flags = vec4f(0.0, 0.0, 0.0, 0.0);
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
fn intersect_tri(r: Ray, id: u32, closest: HitInfo) -> HitInfo {
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
  if (det <= 0.0) {
    return closest;
  }
  let inv_det = 1.0 / det;
  // 交差判定
  let t_vec = start - vert;
  let u = dot(t_vec, p_vec) * inv_det;
  if (u < 0.0 || 1.0 < u) {
    return closest;
  }
  let q_vec = cross(t_vec, e1);
  let v = dot(dir, q_vec) * inv_det;
  if (v < 0.0 || 1.0 < u + v) {
    return closest;
  }
  let t = dot(e2, q_vec) * inv_det;
  if (t < kRayMin || kRayMax < t) {
    return closest;
  }
  let pos = point_at(r, t);
  let norm = face_norm(r, tri.norm.xyz);
  let ray_dist = distance(pos, start);
  let flags = vec4f(tri.emissive, 0.0, 0.0, 0.0);
  hit = ray_dist < closest.dist &&
        (0.0 <= u && u <= 1.0) &&
        (0.0 <= v && (u + v) <= 1.0);

  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.pos, pos, hit),
    select(closest.norm, norm, hit),
    closest.uv,
    select(closest.col, tri.col, hit),
    select(closest.flags, flags, hit),
  );
}

fn intersect_quad(r: Ray, id: u32, closest: HitInfo) -> HitInfo {
  let q = quads[id];
  let plane_dist = dot(q.norm, vec4(r.start.xyz, 1.0));
  let ray_dist = plane_dist / -dot(q.norm.xyz, r.dir.xyz);
  let pos = r.start.xyz + r.dir.xyz * ray_dist;
  let uv = vec2f(dot(vec4f(pos, 1.0), q.right), dot(vec4f(pos, 1.0), q.up)) * 0.5 + 0.5;
  let flags = vec4f(q.emissive, 0.0, 0.0, 0.0);
  let hit = plane_dist > 0.0 &&
            ray_dist > 0.0 &&
            ray_dist < closest.dist &&
            all((uv > vec2f()) & (uv < vec2f(1.0)));
  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.pos, pos, hit),
    select(closest.norm, q.norm.xyz, hit),
    select(closest.uv, uv, hit),
    select(closest.col, q.col, hit),
    select(closest.flags, flags, hit),
  );
}

fn intersect_sphere(r: Ray, id: u32, closest: HitInfo) -> HitInfo {
  let sphere = spheres[id];
  let oc = r.start.xyz - sphere.center;
  let dir = r.dir.xyz;
  let a = dot(dir, dir);
  let half_b = dot(oc, dir);
  let c = dot(oc, oc) - sphere.radius * sphere.radius;
  let discriminant = half_b * half_b - a * c;
  if (discriminant < 0.0) {
    return closest;
  }
  let sqrt_d = sqrt(discriminant);
  // 最近傍のrootを探す
  var root = (-half_b - sqrt_d) / a;
  if (root < kRayMin || kRayMax < root) {
    root = (-half_b + sqrt_d) / a;
    if (root < kRayMin || kRayMax < root) {
      return closest;
    }
  }
  let pos = point_at(r, root);
  let norm = face_norm(r, (pos - sphere.center) / sphere.radius);
  let uv = sphere_uv(norm);
  let ray_dist = distance(pos, r.start.xyz);
  if (ray_dist >= closest.dist) {
    return closest;
  }
  let flags = vec4f(sphere.emissive, 0.0, 0.0, 0.0);
  return HitInfo(ray_dist, pos, norm, uv, sphere.col, flags);
}

@group(2) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(2) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  let screen_size = vec2u(textureDimensions(frameBuffer));
  if (all(invocation_id.xy < screen_size)) {
    seed = invocation_id.x + invocation_id.y * screen_size.x + u32(ray.seed) * screen_size.x * screen_size.y;;
    var col : vec3f;
    for (var spp = 0; spp < kSPP; spp++) {
      let u = (f32(invocation_id.x) + rand()) / f32(screen_size.x);
      let v = (f32(invocation_id.y) + rand()) / f32(screen_size.y);
      let r = setup_camera_ray(vec2f(u, v));
      var path = Path(r, kOne, false);
      for (var i = 0; i < kRayDepth; i++) {
        path = raytrace(path, i);
        if (path.end) {
          break;
        }
      }
      col += max(path.col, kZero) / f32(kSPP);
    }
    textureStore(frameBuffer, invocation_id.xy, vec4(col, 1.0));
  }
}