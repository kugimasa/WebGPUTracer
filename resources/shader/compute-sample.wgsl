const kPI = 3.14159265359;
const k_1_PI = 0.318309886184;
const kNoHit = 0xffffffffu;
const kXup = vec3f(1.0, 0.0, 0.0);
const kYup = vec3f(0.0, 1.0, 0.0);
const kRayDepth = 5;
const kRayMin = 1e-6;
const kRayMax = 1e20;
const kSPP = 150;
const kZero = vec3f(0.0, 0.0, 0.0);
const kOne = vec3f(1.0, 1.0, 1.0);

struct Ray {
  start : vec3f,
  dir : vec3f,
};

struct CameraParam {
  start : vec4f,
  end : vec4f,
  aspect : f32,
  fovy : f32,
  time : f32,
  seed : u32,
};

/// FIXME: Windowsビルドが落ちるため、`dab65be`の変更を元に戻す必要があるかもしれない
/// shape: tri(0), quad(1), sphere(2)
struct HitInfo {
  dist : f32,
  emissive : bool,
  shape : u32,
  pos : vec3f,
  norm : vec3f,
  uv : vec2f,
  col : vec3f,
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

struct Quad {
  pos : vec4f,
  right : vec4f,
  up : vec4f,
  norm : vec4f,
  w : vec3f,
  d : f32,
  col : vec3f,
  emissive : f32,
};

struct Sphere {
  center : vec3f,
  radius : f32,
  col : vec3f,
  emissive : f32,
};

/// CPPと揃える必要がある
struct SphereLights {
  l1 : Sphere,
  l2 : Sphere,
}

fn fabs(x: f32) -> f32 {
  return select(x, -x, x < 0.0);
}

fn point_at(r: Ray, t: f32) -> vec3f {
  return r.start.xyz + t * r.dir.xyz;
}

fn face_norm(r: Ray, norm: vec3f) -> vec3f {
  return select(-norm, norm, dot(r.dir.xyz, norm.xyz) < 0.0);
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
    let u = rand();
    let v = rand();
    let theta = u * 2.0 * kPI;
    let phi = acos(2.0 * v - 1.0);
    let r = pow(rand(), 1.0/3.0);
    let sin_theta = sin(theta);
    let cos_theta = cos(theta);
    let sin_phi = sin(phi);
    let cos_phi = cos(phi);
    let x = r * sin_phi * sin_theta;
    let y = r * sin_phi * cos_theta;
    let z = r * cos_phi;
    return vec3f(x, y, z);
}

fn rand_to_sphere(radius: f32, square_dist: f32) -> vec3f {
  let r1 = rand();
  let r2 = rand();
  let z = 1.0 + r2 * (sqrt(1.0 - radius * radius / square_dist) - 1.0);
  let phi = 2.0 * kPI * r1;
  let x = cos(phi) * sqrt(1.0 - z * z);
  let y = sin(phi) * sqrt(1.0 - z * z);
  return vec3f(x, y, z);
}

fn rand_cos_dir() -> vec3f {
  let r1 = rand();
  let r2 = rand();
  let z = sqrt(1.0 - r2);
  let phi = 2.0 * kPI * r1;
  let x = cos(phi) * sqrt(r2);
  let y = sin(phi) * sqrt(r2);
  return vec3f(x, y, z);
}

fn build_onb_from_w(w: vec3f) -> ONB {
  var onb : ONB;
  onb.w = normalize(w);
  let a = select(kXup, kYup, (sign(onb.w.x) * onb.w.x) > 0.9);
  onb.v = normalize(cross(onb.w, a));
  onb.u = cross(onb.w, onb.v);
  return onb;
}

fn onb_local(onb: ONB, a: vec3f) -> vec3f {
  return a.x * onb.u + a.y * onb.v + a.z * onb.w;
}

fn sample_direction(hit: HitInfo) -> vec3f {
  if (rand() < 0.5) {
    // 光源の形状に基づいたサンプリング
    return sample_from_light(hit);
  } else {
    // BxDFに基づいたサンプリング
    return sample_from_bxdf(hit);
  }
}

fn sample_from_light(hit: HitInfo) -> vec3f {
  // MEMO: 今はSphereLightのみ
  // ヒット位置とライト位置を比較
  let l1_dist = distance(hit.pos, sphere_lights.l1.center);
  let l2_dist = distance(hit.pos, sphere_lights.l2.center);
  var sphere = sphere_lights.l2;
  if (l1_dist < l2_dist) {
    sphere = sphere_lights.l1;
  }
  return sample_from_sphere(sphere, hit.pos);
}

fn sample_from_sphere(sphere: Sphere, pos: vec3f) -> vec3f {
  let dir = sphere.center - pos;
  let onb = build_onb_from_w(dir);
  let square_dist = dot(dir, dir);
  return onb_local(onb, rand_to_sphere(sphere.radius, square_dist));
}

fn sample_from_bxdf(hit: HitInfo) -> vec3f {
    let bxdf = 0u;
    var dir: vec3f;
    switch (bxdf) {
      // Lambertian
      case 0u: {
        dir = sample_from_cosine(hit);
      }
      default: {
        dir = vec3f(0.0, -1.0, 0.0);
      }
    }
    return dir;
}

fn sample_from_cosine(hit: HitInfo) -> vec3f {
  let onb = build_onb_from_w(hit.norm);
  let a = rand_cos_dir();
  return onb_local(onb, a);
}

fn mixture_pdf(hit: HitInfo, dir: vec3f) -> f32 {
  let l1_dist = distance(hit.pos, sphere_lights.l1.center);
  let l2_dist = distance(hit.pos, sphere_lights.l2.center);
  let l1_w = l2_dist / (l1_dist + l2_dist);
  let l2_w = l1_dist / (l1_dist + l2_dist);
  return 0.5 * cosine_pdf(hit, dir) + 0.5 * (l1_w * sphere_pdf(hit, sphere_lights.l1, dir) + l2_w * sphere_pdf(hit, sphere_lights.l2, dir));
}

fn sphere_pdf(hit: HitInfo, sphere: Sphere, dir: vec3f) -> f32 {
  let squared_dist = dot(sphere.center - hit.pos, sphere.center - hit.pos);
  let cos_theta_max = sqrt(1.0 - sphere.radius * sphere.radius / squared_dist);
  let solid_angle = 2.0 * kPI * (1.0 - cos_theta_max);
  return 1.0 / solid_angle;
}

fn cosine_pdf(hit: HitInfo, dir: vec3f) -> f32 {
  let onb = build_onb_from_w(hit.norm);
  let cos = dot(normalize(dir), onb.w);
  return select(cos * k_1_PI, 0.0, cos <= 0.0);
}

fn scattering_pdf(hit: HitInfo, dir: vec3f) -> f32 {
  let cos = dot(hit.norm, normalize(dir));
  return select(cos * k_1_PI, 0.0, cos < 0.0);
}

fn schlick_fresnel(col: vec3f, cos: f32) -> vec3f {
  let pow5 = pow(1.0 - cos, 5.0);
  return col + pow5 * (1.0 - col);
}

@group(0) @binding(0) var<uniform> camera : CameraParam;
@group(1) @binding(0) var<storage> quads : array<Quad>;
@group(1) @binding(1) var<storage> spheres : array<Sphere>;
@group(1) @binding(2) var<uniform> sphere_lights : SphereLights;

fn setup_camera_ray(uv: vec2f) -> Ray {
    let theta = radians(camera.fovy);
    let half_h = tan(theta * 0.5);
    let half_w = camera.aspect * half_h;
    let origin = camera.start.xyz;
    let end = camera.end.xyz;
    let w = normalize(origin - end);
    let u = normalize(cross(kYup, w));
    let v = cross(w, u);
    let lower_left_corner = origin - half_w * u - half_h * v - w;
    let horizontal = 2.0 * half_w * u;
    let vertical = 2.0 * half_h * v;
    let ray_dir = origin - (lower_left_corner + uv.x * horizontal + uv.y * vertical);
    return Ray(origin, ray_dir);
}

fn raytrace(path: Path, depth: i32) -> Path {
  let r = path.ray;
  let hit = sample_hit(r);
  let emissive = hit.emissive;
  // 光源の場合、トレースを終了
  if (emissive) {
    if (depth == 0) {
      var light_col = hit.col / length(hit.col);
      let vdotn = dot(-r.dir.xyz, hit.norm.xyz);
      // TODO: 正しい計算&Bloom...?
      light_col = light_col + light_col * schlick_fresnel(vec3f(0.02), vdotn);
      return Path(r, light_col, true);
    }
    // 照明計算
    let ray_col = hit.col * path.col;
    return Path(r, ray_col, true);
  }
  // 反射オブジェクトの場合
  else {
    // 反射
    let scatter_dir = sample_direction(hit);
    let pdf_val = mixture_pdf(hit, scatter_dir);
//    let scatter_dir = sample_from_cosine(hit);
//    let pdf_val = cosine_pdf(hit, scatter_dir);
    // パスを更新
    let scattered_ray = Ray(hit.pos, scatter_dir);
    let scattered_col = path.col * hit.col * scattering_pdf(hit, scatter_dir) / pdf_val;
    return Path(scattered_ray, scattered_col, false);
  }
}

fn sample_hit(r: Ray) -> HitInfo {
  var hit = HitInfo();
  hit.dist = kRayMax;
  hit.shape = kNoHit;
  hit.emissive = false;
  hit.col = kZero;
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    hit = intersect_quad(r, quad, hit);
  }
  for (var idx = 0u; idx < arrayLength(&spheres); idx++) {
    let sphere = spheres[idx];
    hit = intersect_sphere(r, sphere, hit);
  }
  // 光源との交差判定
  hit = intersect_sphere(r, sphere_lights.l1, hit);
  hit = intersect_sphere(r, sphere_lights.l2, hit);
  return hit;
}

/// quad form RayTracingTheNextWeek
/// https://raytracing.github.io/books/RayTracingTheNextWeek.html#quadrilaterals/interiortestingoftheintersectionusinguvcoordinates
fn intersect_quad(r: Ray, id: u32, closest: HitInfo) -> HitInfo {
  let quad = quads[id];
  let denom = dot(quad.norm.xyz, r.dir.xyz);
  if (fabs(denom) < kRayMin) {
    return closest;
  }
  let t = (quad.d - dot(quad.norm.xyz, r.start.xyz)) / denom;
  if (t < kRayMin || kRayMax < t) {
    return closest;
  }
  let pos = point_at(r, t);
  let ray_dist = distance(pos, r.start.xyz);
  if (ray_dist >= closest.dist) {
    return closest;
  }
  let hit_vec = pos - quad.pos.xyz;
  let a = dot(quad.w, cross(hit_vec, quad.up.xyz));
  let b = dot(quad.w, cross(quad.right.xyz, hit_vec));
  if ((a < 0.0) || (1.0 < a) || (b < 0.0) || (1.0 < b)) {
    return closest;
  }
  let norm = faceForward(quad.norm.xyz, r.dir.xyz, quad.norm.xyz);
  let uv = vec2f(a, b);
  return HitInfo(ray_dist, bool(quad.emissive), 1u, pos, norm, uv, quad.col);
}

fn intersect_sphere(r: Ray, sphere: Sphere, closest: HitInfo) -> HitInfo {
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
  let ray_dist = distance(pos, r.start.xyz);
  if (ray_dist >= closest.dist) {
    return closest;
  }
  let sphere_norm = (pos - sphere.center) / sphere.radius;
  let norm = faceForward(sphere_norm, r.dir.xyz, sphere_norm);
  let uv = sphere_uv(norm);
  return HitInfo(ray_dist, bool(sphere.emissive), 2u, pos, norm, uv, sphere.col);
}

@group(2) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(2) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  let screen_size = vec2u(textureDimensions(frameBuffer));
  if (all(invocation_id.xy < screen_size)) {
    seed = invocation_id.x + invocation_id.y * screen_size.x + u32(camera.seed) * screen_size.x * screen_size.y;;
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