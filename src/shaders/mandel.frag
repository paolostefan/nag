#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2 u_resolution;
uniform float u_center_x;
uniform float u_center_y;
uniform float u_zoom;
uniform int u_iterations;

void main() {
    // Map pixel coordinates to complex plane
    vec2 uv = (gl_FragCoord.xy / u_resolution) * 2.0 - 1.0;
    uv.x *= u_resolution.x / u_resolution.y; // Aspect ratio correction

    // Apply zoom and center
    vec2 c = vec2(u_center_x, u_center_y) + uv / u_zoom;

    // Mandelbrot iteration
    vec2 z = vec2(0.0);
    int iter = 0;

    for (int i = 0; i < u_iterations; i++)
    {
        iter = i;

        // z = z^2 + c
        float x_temp = z.x * z.x - z.y * z.y + c.x;
        z.y = 2.0 * z.x * z.y + c.y;
        z.x = x_temp;

        // Check escape condition (|z| > 3)
        if (dot(z, z) > 9.0)
        break;
    }

    // Color based on iteration count
    if (iter == u_iterations - 1)
    {
        // Inside the set - black
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        // Outside the set - colorful gradient
        float t = float(iter) / float(u_iterations);

        // Smooth coloring (continuous potential method)
        //        float log_zn = log(dot(z, z)) / 2.0;
        //        float nu = log(log_zn / log(2.0)) / log(2.0);
        //        t = t + 1.0 - nu;

        // Normalize and apply color offset for animation
        //        t = fract(t * 0.1);
        frag_color = vec4(t, t, t, 1.0);
    }
}