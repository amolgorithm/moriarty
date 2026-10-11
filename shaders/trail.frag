#version 330
// Phase 6: GPU trail fade, replaces the alpha-rect fade in Renderer::draw.
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec3 bgColor;
uniform float fade;
out vec4 fragColor;

void main() {
    vec3 c = texture(texture0, fragTexCoord).rgb;
    fragColor = vec4(mix(c, bgColor, fade), 1.0);
}
