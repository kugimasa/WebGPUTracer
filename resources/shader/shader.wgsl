struct VertexInput {
  @location(0) position: vec3f,
  @location(1) color: vec3f,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) color: vec3f,
};

struct RenderParam {
  color: vec4f,
  time: f32,
};

@group(0) @binding(0) var<uniform> uRenderParam: RenderParam;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  // Window Aspect
  let ratio = 512.0 / 512.0;
  var offset = vec2f(0.0);
  let angle = uRenderParam.time;

  // Rotate
  let alpha = cos(angle);
  let beta = sin(angle);
  var pos = vec3f(in.position.x,
                  alpha * in.position.y + beta * in.position.z,
                  alpha * in.position.z - beta * in.position.y);
  out.position = vec4f(pos.x, pos.y * ratio, pos.z * 0.5 + 0.5, 1.0);
  out.color = in.color;
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  let color = in.color * uRenderParam.color.rgb;
  // Gamma-correction
  let corrected_col = pow(color, vec3f(2.2));
  return vec4f(corrected_col, uRenderParam.color.a);
}