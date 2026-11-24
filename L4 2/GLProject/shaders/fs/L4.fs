#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTex;
uniform int   uMode;      // 0 = Lit, 1 = Card, 2 = Poster (torn), 3 = Unlit/alpha
uniform vec4  uTint;
uniform float uSpec;
uniform float uShine;
uniform vec3  uViewPos;
uniform vec3  uLightPos[2];
uniform vec3  uLightColor[2];
uniform vec3  uAmbient;

// ---------- small value-noise, used for the torn edges of the poster ----------
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
float vnoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}

// ---------- Blinn-Phong with two point lights ----------
vec3 shade(vec3 albedo, vec3 N)
{
    vec3 V = normalize(uViewPos - vWorldPos);
    if (dot(N, V) < 0.0) N = -N;

    vec3 result = uAmbient * albedo;
    for (int i = 0; i < 2; ++i)
    {
        vec3  L    = uLightPos[i] - vWorldPos;
        float dist = max(length(L), 0.0001);
        L /= dist;
        float atten = 1.0 / (1.0 + 0.025 * dist + 0.004 * dist * dist);
        // نعومة إضافية على الزوايا حتى لا تنطفئ البطاقة بشدة عند رفعها
        float diff  = mix(max(dot(N, L), 0.0), 1.0, 0.25);
        vec3  H     = normalize(L + V);
        float spec  = pow(max(dot(N, H), 0.0), uShine) * uSpec;
        result += (albedo * diff + vec3(spec)) * uLightColor[i] * atten;
    }
    return result;
}

void main()
{
    vec3 N = normalize(vNormal);
    vec4 texel = texture(uTex, vUV);

    // ---- Unlit, alpha blended (zone slot outlines) ----
    if (uMode == 3)
    {
        vec4 c = texel * uTint;
        if (c.a < 0.01) discard;
        FragColor = c;
        return;
    }

    // ---- Card: rounded rectangle with a golden frame ----
    // ---- Card: rounded rectangle with a golden frame ----
    if (uMode == 1)
    {
        vec2  p = (vUV - 0.5) * vec2(1.0, 1.5);
        float r = 0.09;
        vec2  q = abs(p) - (vec2(0.5, 0.75) - r);
        float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
        if (d > 0.0) discard;

        vec3 albedo = texel.rgb * uTint.rgb;
        if (d > -0.045)
        {
            float shine = 0.75 + 0.25 * sin((vUV.x + vUV.y) * 18.0);
            albedo = vec3(0.86, 0.68, 0.24) * shine;
            if (d > -0.012) albedo = vec3(0.25, 0.16, 0.05);
        }
        FragColor = vec4(shade(albedo, N) + albedo * 0.22, 1.0);
        return;
    }
    // ---- Face-down Card using backG.png texture + golden frame ----
    if (uMode == 4)
    {
        vec2  p = (vUV - 0.5) * vec2(1.0, 1.5);
        float r = 0.09;
        vec2  q = abs(p) - (vec2(0.5, 0.75) - r);
        float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
        if (d > 0.0) discard;

        vec3 albedo = texel.rgb * uTint.rgb;
        if (d > -0.045)
        {
            float shine = 0.75 + 0.25 * sin((vUV.x + vUV.y) * 18.0);
            albedo = vec3(0.86, 0.68, 0.24) * shine;
            if (d > -0.012) albedo = vec3(0.25, 0.16, 0.05);
        }
        FragColor = vec4(shade(albedo, N) + albedo * 0.05, 1.0);
        return;
    }

    // ---- Poster: lit paper with torn corners / edges ----
    if (uMode == 2)
    {
        float n = vnoise(vUV * vec2(14.0, 18.0)) * 0.6 + vnoise(vUV * vec2(55.0, 70.0)) * 0.4;
        float jag = (n - 0.5) * 0.16;

        // 1) top-right corner ripped off along a diagonal
        float tearA = (vUV.x + vUV.y) - (1.60 + jag);                  // > 0 -> torn away
        // 2) bite out of the bottom-left corner
        float tearB = length((vUV - vec2(0.0, 0.0)) * vec2(0.95, 1.20)) - (0.26 + jag * 0.8); // < 0 -> torn away
        // 3) strip missing on the left edge (middle height)
        float stripLimit = 0.075 + jag * 0.5;
        bool  tearC = (vUV.x < stripLimit) && (vUV.y > 0.38) && (vUV.y < 0.62);

        if (tearA > 0.0 || tearB < 0.0 || tearC) discard;

        vec3 albedo = texel.rgb * vec3(1.0, 0.97, 0.90);
        // pale fibrous paper along the rips
        bool  inStripRows = (vUV.y > 0.38) && (vUV.y < 0.62);
        float rimC = inStripRows ? (vUV.x - stripLimit) * 1.2 : 1.0;
        float rim  = min(min(-tearA, tearB), rimC);
        float rimMix = 1.0 - smoothstep(0.0, 0.035, rim);
        albedo = mix(albedo, vec3(0.93, 0.90, 0.82), rimMix * 0.85);
        // horizontal fold crease + slight ageing
        albedo *= 1.0 - 0.22 * smoothstep(0.010, 0.0, abs(vUV.y - 0.52));
        albedo *= 0.92 + 0.08 * n;
        FragColor = vec4(shade(albedo, N), 1.0);
        return;
    }

    // ---- Default: textured + lit ----
    vec3 albedo = texel.rgb * uTint.rgb;
    FragColor = vec4(shade(albedo, N), 1.0);
}
