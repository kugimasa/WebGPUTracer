const kPI = 3.14159265359;
const kNoHit = 0xffffffffu;
const kFovy = 40.0f;
const kYup = vec3f(0.0, 1.0, 0.0);

struct Quad {
  center : vec4f,
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
  quad_idx : u32,
  pos : vec3f,
  uv : vec2f,
};

fn point_at(r : Ray, t : f32) -> vec3f {
  return r.start.xyz + t * r.dir.xyz;
}

@group(0) @binding(0) var<uniform> ray : Ray;
@group(0) @binding(1) var<storage> quads : array<Quad>;

fn raytrace(ray : Ray, uv : vec2f) -> vec3f {
  // Setup ray
  let theta = radians(kFovy);
  let half_h = tan(theta * 0.5);
  let half_w = ray.aspect * half_h;
  let w = normalize(ray.start.xyz - ray.dir.xyz);
  let u = normalize(cross(kYup, w));
  let v = cross(w, u);
  let lower_left_corner = ray.start.xyz - half_w * u - half_h * v - w;
  let horizontal = 2.0 * half_w * u;
  let vertical = 2.0 * half_h * v;
  let ray_dir = vec4(lower_left_corner + uv.x * horizontal + uv.y * vertical - ray.start.xyz, 0.0);
  let r = Ray(ray.start, ray_dir, ray.aspect, ray.time, ray.rand);
//  var hit = HitInfo();
//  hit.dist = 1e20;
//  hit.quad_idx = kNoHit;
//  // loop for num of quads
//  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
//    hit = intersect_ray_quad(ray, quad, hit);
//  }
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

@group(1) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(1) @binding(1) var frameBuffer: texture_storage_2d<rgba8unorm,write>;

const NumReflectionRays = 5;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) invocation_id: vec3<u32>) {
  if (all(invocation_id.xy < textureDimensions(frameBuffer))) {
//    init_rand(invocation_id);
//
//    // Calculate the fragment's NDC coordinates for the intersection of the near
//    // clip plane and far clip plane
//    let ndcXY = (uv - 0.5) * vec2(2.0, -2.0);
//
//    // Transform the coordinates back into world space
//    var near = camera_uniforms.inv_mvp * vec4f(ndcXY, 0.0, 1.0);
//    var far = camera_uniforms.inv_mvp * vec4f(ndcXY, 1.0, 1.0);
//    near /= near.w;
//    far /= far.w;
//
//    // Create a ray that starts at the near clip plane, heading in the fragment's
//    // z-direction, and raytrace to find the nearest quad that the ray intersects.
//    let ray = Ray(near.xyz, normalize(far.xyz - near.xyz));
    let uv = vec2f(invocation_id.xy) / vec2f(textureDimensions(frameBuffer).xy);
    var hit_color = raytrace(ray, uv);
    textureStore(frameBuffer, invocation_id.xy, vec4(hit_color, 1.0));
  }
}