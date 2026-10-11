#version 330
in vec2 vUv;
in vec4 vColor;
out vec4 fragColor;

void main() {
    float d = length(vUv);
    float a = 1.0 - smoothstep(0.55, 1.0, d);
    fragColor = vec4(vColor.rgb, vColor.a * a);
}
