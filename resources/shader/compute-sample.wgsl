const kPI = 3.14159265359;
const kNoHit = 0xffffffffu;
const kFovy = 40.0f;
const kYup = vec3f(0.0, 1.0, 0.0);

struct Quad {
  plane : vec4f,
  right : vec4f,
  up : vec4f,
  color : vec3f,
  emissive : f32,
};

struct Ray {
  start : vec4f,
  dir : vec4f,
  aspect : f32,
  time : f32,
  rand : f32,
};

struct HitInfo {
  dist : f32,
  quad : u32,
  tri : u32,
  pos : vec3f,
  uv : vec2f,
};

// FIXME: vec3f で積めれないか検討
struct Tri {
  vert : vec4f,
  e1 : vec4f,
  e2 : vec4f,
  norm : vec4f,
  color : vec3f,
  emissive : f32,
};

fn point_at(r : Ray, t : f32) -> vec3f {
  return r.start.xyz + t * r.dir.xyz;
}

fn rand_unit_sphere() -> vec3f {
//    var u = rand();
//    var v = rand();
//    var theta = u * 2.0 * kPI;
//    var phi = acos(2.0 * v - 1.0);
//    var r = pow(rand(), 1.0/3.0);
//    var sin_theta = sin(theta);
//    var cos_theta = cos(theta);
//    var sin_phi = sin(phi);
//    var cos_phi = cos(phi);
//    var x = r * sin_phi * sin_theta;
//    var y = r * sin_phi * cos_theta;
//    var z = r * cos_phi;
    return vec3f(1.0, 0.0, 0.0);
}

@group(0) @binding(0) var<uniform> ray : Ray;
@group(0) @binding(1) var<storage> quads : array<Quad>;
@group(1) @binding(0) var<storage> tris : array<Tri>;

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
    return Ray(ray.start, ray_dir, ray.aspect, ray.time, ray.rand);
}

fn raytrace(r : Ray) -> HitInfo {
  var hit = HitInfo();
  hit.dist = 1e20;
  hit.quad = kNoHit;
  hit.tri = kNoHit;
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    hit = intersect_quad(r, quad, hit);
  }
  for (var tri = 0u; tri < arrayLength(&tris); tri++) {
    hit = intersect_tri(r, tri, hit);
  }
  return hit;
}

fn raytrace_sphere(r : Ray) -> vec3f {
  var t = intersect_sphere(vec3f(0.0, 0.0, 13.0), 3.0, r);
  if (t > 0.0) {
    let n = normalize(point_at(r, t) - vec3(0.0, 0.0, -1.0));
    return 0.5 * vec3f(n.x + 1.0, n.y + 1.0, n.z + 1.0);
  }
  t = 0.5 * (normalize(r.dir.xyz).y + 1.0);
  return mix(vec3f(0.1, 0.2, 0.8), vec3f(0.2, 0.1, 0.2), t);
}

fn intersect_sphere(center : vec3f, radius : f32, r : Ray) -> f32 {
  let oc = r.start.xyz - center;
  let dir = r.dir.xyz;
  let a = dot(dir, dir);
  let b = 2.0 * dot(oc, dir);
  let c = dot(oc, oc) - radius * radius;
  let discriminant = b * b - 4.0 * a * c;
  if (discriminant < 0.0) {
    return -1.0;
  }
  return (-b - sqrt(discriminant)) / (2.0 * a);
}

fn intersect_quad(r : Ray, quad : u32, closest : HitInfo) -> HitInfo {
  let q = quads[quad];
  let plane_dist = dot(q.plane, vec4(r.start.xyz, 1.0));
  let ray_dist = plane_dist / -dot(q.plane.xyz, r.dir.xyz);
  let pos = r.start.xyz + r.dir.xyz * ray_dist;
  let uv = vec2f(dot(vec4f(pos, 1.0), q.right), dot(vec4f(pos, 1.0), q.up)) * 0.5 + 0.5;
  let hit = plane_dist > 0.0 &&
            ray_dist > 0.0 &&
            ray_dist < closest.dist &&
            all((uv > vec2f()) & (uv < vec2f(1.0)));
  return HitInfo(
    select(closest.dist, ray_dist, hit),
    select(closest.quad, quad,     hit),
    closest.tri,
    select(closest.pos,  pos,      hit),
    select(closest.uv,   uv,       hit),
  );
}

/// Möller–Trumbore intersection algorithm
fn intersect_tri(r : Ray, tri_idx : u32, closest : HitInfo) -> HitInfo {
  var hit = false;
  // 一時変数に格納
  let tri = tris[tri_idx];
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
  hit = (0.0 <= u && u <= 1.0) && (0.0 <= v && (u + v) <= 1.0);
  let pos = point_at(r, t);
  let ray_dist = distance(pos, start);
  return HitInfo(
    select(closest.dist, ray_dist, hit),
    closest.quad,
    select(closest.tri,  tri_idx,  hit),
    select(closest.pos,  pos,      hit),
    closest.uv,
  );
}

// まずはオブジェクトと交差したかをみる
fn sample_hit(hit : HitInfo) -> vec3f {
  let quad = quads[hit.quad];
  let tri = tris[hit.tri];
  return quad.color + tri.color;
}

@group(2) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(2) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

const NumReflectionRays = 5;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  if (all(invocation_id.xy < textureDimensions(frameBuffer))) {
    let uv = vec2f(invocation_id.xy) / vec2f(textureDimensions(frameBuffer).xy);
    let r = setup_camera_ray(uv);
    let hit = raytrace(r);
    var hit_color = sample_hit(hit);
//    var normal = quads[hit.quad].plane.xyz;
//        let bounce = reflect(ray.dir.xyz, normal);
//        var reflection : vec3f;
//        for (var i = 0; i < NumReflectionRays; i++) {
//          let reflection_dir = normalize(bounce + rand_unit_sphere()*0.1);
//          let reflection_ray = Ray(vec4(hit.pos + bounce * 1e-5, 1.0), vec4(reflection_dir, 1.0), r.aspect, r.time, r.rand);
//          let reflection_hit = raytrace(reflection_ray);
//          reflection += sample_hit(reflection_hit);
//        }
//    let color = mix(reflection / f32(NumReflectionRays), hit_color, 0.95);
    textureStore(frameBuffer, invocation_id.xy, vec4(hit_color, 1.0));
  }
}