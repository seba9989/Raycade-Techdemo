#version 330
// Kineskop (Engine::PostShader): lekka krzywizna, linie skanowania, rozszczepienie kolorów
// i winieta. Działa przy przenoszeniu gotowego obrazu gry do okna.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
out vec4 finalColor;

vec2 curve(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    uv *= 1.0 + vec2(uv.y * uv.y, uv.x * uv.x) * 0.035;
    return uv * 0.5 + 0.5;
}

void main() {
    vec2 uv = curve(fragTexCoord);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec2 size = vec2(textureSize(texture0, 0));
    float shift = 1.2 / size.x;
    vec3 color;
    color.r = texture(texture0, uv + vec2(shift, 0.0)).r;
    color.g = texture(texture0, uv).g;
    color.b = texture(texture0, uv - vec2(shift, 0.0)).b;
    float scan = 0.85 + 0.15 * sin(uv.y * size.y * 3.14159);
    vec2 v = uv * (1.0 - uv);
    float vignette = clamp(pow(v.x * v.y * 18.0, 0.22), 0.0, 1.0);
    finalColor = vec4(color * scan * vignette * 1.12, 1.0) * colDiffuse * fragColor;
}
