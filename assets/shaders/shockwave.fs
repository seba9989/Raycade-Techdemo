#version 330
// Fala uderzeniowa na polu gry (node::visual::RenderGroup): pierścień przesuwa piksele od środka
// wybuchu. Tekstura RenderGroup jest w OpenGL odwrócona w pionie, dlatego położenie piksela
// w układzie grupy to (u, 1 - v) * size.
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 size;      // rozmiar grupy w pikselach
uniform vec2 center;    // środek fali w układzie grupy
uniform float time;     // sekundy od wybuchu (< 0 - brak fali)
uniform float strength; // siła przesunięcia w pikselach
out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    if (time >= 0.0 && time < 0.8) {
        vec2 pos = vec2(uv.x, 1.0 - uv.y) * size;
        vec2 d = pos - center;
        float dist = length(d);
        float radius = time * 520.0;
        float ring = dist - radius;
        if (abs(ring) < 28.0 && dist > 0.001) {
            float power = (1.0 - abs(ring) / 28.0) * (1.0 - time / 0.8);
            vec2 offset = normalize(d) * power * strength;
            uv -= vec2(offset.x, -offset.y) / size;
        }
    }
    finalColor = texture(texture0, uv) * colDiffuse * fragColor;
}
