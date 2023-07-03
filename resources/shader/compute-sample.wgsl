@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
@group(0) @binding(1) var outputBuffer: texture_storage_2d<rgba8unorm,write>;

fn f1(x: f32) -> f32 {
    return (2.0 * x + 1.0) * 0.1;
}

fn f2(x: f32) -> f32 {
    return (x + 2.0) * 0.1;
}

@compute @workgroup_size(8, 8)
fn compute_sample(@builtin(global_invocation_id) id: vec3<u32>) {
    let in = inputBuffer[id.x];
    let color = vec4(
        clamp(f1(in), 0, 1),
        clamp(f2(in), 0, 1),
        clamp(in, 0, 1),
        1.0
    );
    textureStore(outputBuffer, id.xy, color);
}