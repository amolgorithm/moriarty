#version 330
// Phase 6: overlay for the heat / resource fields uploaded as a single-channel texture.
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec3 coolColor;
uniform vec3 hotColor;
out vec4 fragColor;

void main() {
    float h = texture(texture0, fragTexCoord).r;
    fragColor = vec4(mix(coolColor, hotColor, h), h * 0.5);
}
