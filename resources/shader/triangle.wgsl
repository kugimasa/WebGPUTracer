@group(0) @binding(0) var<uniform> uIsWgpuNative: f32;

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
  var p = vec2f(0.0, 0.0);
  if (in_vertex_index == 0u) {
      p = vec2f(-0.5, -0.5);
  } else if (in_vertex_index == 1u) {
      p = vec2f(0.5, -0.5);
  } else {
      p = vec2f(0.0, 0.5);
  }
  return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
  var r = 239.0 / 255.0;
  var g = 118.0 / 255.0;
  var b = 122.0 / 255.0;
  var c = vec3f(r, g, b);
  if (uIsWgpuNative > 0.5f) {
    // We apply a gamma-correction to the color
    c = pow(c, vec3f(2.2));
  }
  return vec4f(c, 1.0);
}