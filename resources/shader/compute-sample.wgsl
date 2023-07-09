@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(0) @binding(1) var outputBuffer: texture_storage_2d<rgba8unorm,write>;

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) id: vec3<u32>) {
    let uv = vec2f(id.xy) / vec2f(textureDimensions(outputBuffer).xy);
    let color = vec4(uv.x, uv.y, 0.0, 1.0);
    textureStore(outputBuffer, id.xy, color);
}