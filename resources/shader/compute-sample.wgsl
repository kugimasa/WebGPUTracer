const kPI = 3.14159265359;
const k_1_PI = 0.318309886184;
const kNoHit = 0xffffffffu;
const kXup = vec3f(1.0, 0.0, 0.0);
const kYup = vec3f(0.0, 1.0, 0.0);
const kRayDepth = 10;
const kRayMin = 0.001;
const kRayMax = 1e20;
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
  spp : u32,
  seed : u32,
};

/// shape: tri(0), quad(1), sphere(2)
struct HitInfo {
  dist : f32,
  emissive : bool,
  front_face : bool,
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

fn fabs(x: f32) -> f32 {
  return select(x, -x, x < 0.0);
}

fn point_at(r: Ray, t: f32) -> vec3f {
  return r.start + t * r.dir;
}

fn face_norm(r: Ray, norm: vec3f) -> vec3f {
  return select(-norm, norm, dot(r.dir, norm.xyz) < 0.0);
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
    if (rand() > 0.5) {
      return sample_from_bxdf(hit);
    }
    else {
      return sample_from_light(hit);
    }
}

fn sample_from_sphere(sphere: Sphere, pos: vec3f) -> vec3f {
  let dir = sphere.center - pos;
  let onb = build_onb_from_w(dir);
  let square_dist = dot(dir, dir);
  return onb_local(onb, rand_to_sphere(sphere.radius, square_dist));
}

fn sample_from_light(hit: HitInfo) -> vec3f {
  // FIXME: Sampling from rect light directly
  let on_light = vec3(rand() * 130.0 + 213.0, 554.0, rand() * 105.0 + 227.0);
  var to_light = on_light - hit.pos;
  return to_light;
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
  return 0.5 * cosine_pdf(hit, normalize(dir)) + 0.5 * light_area_pdf(hit, dir);
}

fn sphere_pdf(hit: HitInfo, sphere: Sphere, dir: vec3f) -> f32 {
  let squared_dist = dot(sphere.center - hit.pos, sphere.center - hit.pos);
  let cos_theta_max = sqrt(1.0 - sphere.radius * sphere.radius / squared_dist);
  let solid_angle = 2.0 * kPI * (1.0 - cos_theta_max);
  return 1.0 / solid_angle;
}

fn light_area_pdf(hit: HitInfo, dir: vec3f) -> f32 {
  var rec = HitInfo();
  rec.dist = kRayMax;
  rec.shape = kNoHit;
  rec.emissive = false;
  rec.front_face = false;
  rec.col = kZero;
  var idx = kNoHit;
  // FIXME: origin of the ray is wrong ... ?
  var r = Ray(hit.pos, normalize(dir));
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    rec = intersect_quad(r, quad, rec);
    if (rec.emissive) {
      idx = quad;
    }
  }
  if (idx == kNoHit || rec.dist == kRayMax) {
    return 0.0;
  }
  // MEMO: Calculate from hit
  // let to_light = normalize(dir);
  // let n = cross(quads[idx].right.xyz, quads[idx].up.xyz);
  // let distance_squared = rec.dist * rec.dist * length(to_light) * length(to_light);
  // let light_cosine = fabs(dot(to_light, rec.norm) / length(to_light));
  // return distance_squared / (light_cosine * length(n));
  // MEMO: Calculating light area light directly
  let light_area = 130.0 * 105.0;
  let distance_squared = length(dir) * length(dir);
  let light_cosine = fabs(normalize(dir).y) + kRayMin;
  return distance_squared / (light_cosine * light_area);
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

fn pixel_sample_square(offset: vec2f, u: vec3f, v: vec3f) -> vec3f {
    let recip_sqrt_spp = 1.0 / sqrt(f32(camera.spp));
    let px = -0.5 + recip_sqrt_spp * (offset.x + rand());
    let py = -0.5 + recip_sqrt_spp * (offset.y + rand());
    return (px * u) + (py * v);
}

fn setup_camera_ray(pos: vec2f, offset: vec2f, screen_size: vec2f) -> Ray {
    let theta = radians(camera.fovy);
    let origin = camera.start.xyz;
    let end = camera.end.xyz;
    let focal_length = length(origin - end);
    let h = tan(theta * 0.5);
    let viewport_height = 2.0 * h * focal_length;
    let viewport_width = viewport_height * camera.aspect;

    let w = normalize(origin - end);
    let u = normalize(cross(kYup, w));
    let v = cross(w, u);

    let viewport_u = viewport_width * u;
    let viewport_v = viewport_height * -v;
    let pixel_delta_u = viewport_u / screen_size.x;
    let pixel_delta_v = viewport_v / screen_size.y;
    let viewport_upper_left = origin - focal_length * w - viewport_u * 0.5 - viewport_v * 0.5;
    let pixel_origin = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    let pixel_center = pixel_origin + (pos.x * pixel_delta_u) + (pos.y * pixel_delta_v);
    let pixel_sample = pixel_center + pixel_sample_square(offset, pixel_delta_u, pixel_delta_v);
    let ray_dir = pixel_sample - origin;
    return Ray(origin, ray_dir);
}

fn raytrace(path: Path, depth: i32) -> Path {
  let r = path.ray;
  let hit = sample_hit(r);
  let emissive = hit.emissive;
  // If light end trace
  if (emissive) {
    if (depth == 0) {
      return Path(r, hit.col, true);
    }
    // Light estimation
    let ray_col = f32(hit.front_face) * hit.col * path.col;
    return Path(r, ray_col, true);
  }
  // Non-light object
  else {
    // Reflection
    var scatter_dir = sample_direction(hit);
    let pdf_val = mixture_pdf(hit, scatter_dir);
    // let scatter_dir = sample_from_bxdf(hit);
    // let pdf_val = cosine_pdf(hit, scatter_dir);
//    var scatter_dir = sample_from_light(hit);
//    let pdf_val = light_area_pdf(hit, scatter_dir);
    scatter_dir = normalize(scatter_dir);
    // Update path
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
  hit.front_face = false;
  hit.col = kZero;
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    hit = intersect_quad(r, quad, hit);
  }
  for (var idx = 0u; idx < arrayLength(&spheres); idx++) {
    let sphere = spheres[idx];
    hit = intersect_sphere(r, sphere, hit);
  }
  return hit;
}

/// quad form RayTracingTheNextWeek
/// https://raytracing.github.io/books/RayTracingTheNextWeek.html#quadrilaterals/interiortestingoftheintersectionusinguvcoordinates
fn intersect_quad(r: Ray, id: u32, closest: HitInfo) -> HitInfo {
  let quad = quads[id];
  let denom = dot(quad.norm.xyz, r.dir);
  if (fabs(denom) < kRayMin) {
    return closest;
  }
  let t = (quad.d - dot(quad.norm.xyz, r.start)) / denom;
  if (t < kRayMin || kRayMax < t) {
    return closest;
  }
  let pos = point_at(r, t);
  let ray_dist = distance(pos, r.start);
  if (ray_dist >= closest.dist) {
    return closest;
  }
  let hit_vec = pos - quad.pos.xyz;
  let a = dot(quad.w, cross(hit_vec, quad.up.xyz));
  let b = dot(quad.w, cross(quad.right.xyz, hit_vec));
  if ((a < 0.0) || (1.0 < a) || (b < 0.0) || (1.0 < b)) {
    return closest;
  }
  let front_face = dot(r.dir, quad.norm.xyz) < 0.0;
  let norm = select(-quad.norm.xyz, quad.norm.xyz, front_face);
  let uv = vec2f(a, b);
  return HitInfo(ray_dist, bool(quad.emissive > 0.0f), front_face, 1u, pos, norm, uv, quad.col);
}

fn intersect_sphere(r: Ray, sphere: Sphere, closest: HitInfo) -> HitInfo {
  let oc = r.start - sphere.center;
  let dir = r.dir;
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
  let ray_dist = distance(pos, r.start);
  if (ray_dist >= closest.dist) {
    return closest;
  }
  let sphere_norm = (pos - sphere.center) / sphere.radius;
  let front_face = dot(r.dir, sphere_norm) < 0.0;
  let norm = select(-sphere_norm, sphere_norm, front_face);
  let uv = sphere_uv(norm);
  return HitInfo(ray_dist, bool(sphere.emissive > 0.0f), front_face, 2u, pos, norm, uv, sphere.col);
}

@group(2) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(2) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(16, 16)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  let screen_size = vec2u(textureDimensions(frameBuffer));
  if (all(invocation_id.xy < screen_size)) {
    seed = invocation_id.x + invocation_id.y * screen_size.x + u32(camera.seed) * screen_size.x * screen_size.y;;
    var col : vec3f;
    var sqrt_spp = u32(sqrt(f32(camera.spp)));
    for (var s_j = 0u; s_j < sqrt_spp; s_j++) {
      for (var s_i = 0u; s_i < sqrt_spp; s_i++) {
        let pos = vec2f(f32(invocation_id.x), f32(invocation_id.y));
        let offset = vec2f(f32(s_i), f32(s_j));
        let r = setup_camera_ray(pos, offset, vec2f(screen_size));
        var path = Path(r, kOne, false);
        for (var i = 0; i < kRayDepth; i++) {
          path = raytrace(path, i);
          if (path.end) {
            break;
          }
        }
        col += max(path.col, kZero) / f32(camera.spp);
      }
    }
    textureStore(frameBuffer, invocation_id.xy, vec4(col, 1.0));
  }
}