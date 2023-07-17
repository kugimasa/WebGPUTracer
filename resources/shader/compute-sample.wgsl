const PI = 3.14159265359;
const kNoHit = 0xffffffffu;

struct Quad {
  center : vec4f,
  right : vec4f,
  up : vec4f,
  color : vec3f,
  emissive : f32,
};

struct Ray {
  start : vec3f,
  end : vec3f,
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

@group(0) @binding(0) var<uniform> ray : Ray;
@group(0) @binding(1) var<storage> quads : array<Quad>;

fn raytrace(ray : Ray) -> HitInfo {
  var hit = HitInfo();
  hit.dist = 1e20;
  hit.quad_idx = kNoHit;
  // loop for num of quads
  for (var quad = 0u; quad < arrayLength(&quads); quad++) {
    hit = HitInfo(0.0f, quad, vec3f(), vec2f());
  }
  return hit;
}

fn sample_hit(hit : HitInfo) -> vec3f {
  if (hit.quad_idx == kNoHit) {
    return vec3(0.0f, 0.0f, 0.0f);
  }
  return vec3(1.0f, 0.0f, 0.0f);
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
//    let uv = vec2f(invocation_id.xy) / vec2f(textureDimensions(frameBuffer).xy);
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
    let hit = raytrace(ray);

    let hit_color = sample_hit(hit);
    textureStore(frameBuffer, invocation_id.xy, vec4(hit_color, 1.0));
  }
}