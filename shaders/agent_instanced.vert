#version 330
// Phase 6: instanced agent quads. Per-instance data comes from a vertex buffer
// filled straight from the render snapshot.
layout(location = 0) in vec2 aCorner;     // unit quad corner in [-1, 1]
layout(location = 1) in vec2 iPos;        // world position
layout(location = 2) in float iRadius;
layout(location = 3) in vec4 iColor;

uniform mat4 mvp;

out vec2 vUv;
out vec4 vColor;

void main() {
    vUv = aCorner;
    vColor = iColor;
    gl_Position = mvp * vec4(iPos + aCorner * iRadius, 0.0, 1.0);
}
