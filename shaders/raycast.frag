#version 330
precision highp float;

out vec4 fragmentColor;
in vec4 rayDir;

uniform struct {
    mat4 rayDirMatrix;
    vec3 position;
} camera;

float noise(vec3 r) {
    uvec3 s = uvec3(
    0x1D4E1D4E,
    0x58F958F9,
    0x129F129F);
    float f = 0.0;
    for (int i = 0; i < 16; i++) {
        vec3 sf = vec3(s & uvec3(0xFFFF)) / 65536.0 - vec3(0.5, 0.5, 0.5);

        f += sin(dot(sf, r));
        s = s >> 1;
    }
    return f / 32.0 + 0.5;
}


vec3 noiseGrad(vec3 r) {
    uvec3 s =
    uvec3(0x1D4E1D4E, 0x58F958F9, 0x129F129F);
    vec3 f = vec3(0, 0, 0);
    for (int i = 0; i < 16; i++) {
        vec3 sf =
        vec3(s & uvec3(0xffff)) / 65536.0
        - vec3(0.5, 0.5, 0.5);

        f += cos(dot(sf, r)) * sf;
        s = s >> 1;
    }
    return f;
}

float f(vec3 p) {
    return p.y - noise(p * 50.0);
}

vec3 fGrad(vec3 p){
    return vec3(0, 1, 0) - noiseGrad(p * 50.0);
}


void main(void) {
    vec3 d = normalize(rayDir.xyz);

    float t1 = -camera.position.y / d.y;
    float t2 = (1.0 - camera.position.y) / d.y;
    float tstart = max(min(t1, t2), 0.0);
    float tend = max(t1, t2);

    bool found = false;
    vec3 p, color;

    //LABTODO: ray marching
    if (tstart < tend) {
        p = camera.position + d * tstart;
        vec3 step = d * min((tend - tstart) / 580.0, 0.01);
        for (int i = 0; i < 128; i++) {
            p += step;
            step *= 1.02;
            if (f(p) < 0.0) {
                found = true; break;
            }
        }
        if (found) {
            vec3 lastStep = step / 1.02;
            vec3 midPoint = p - lastStep / 2.0;
            step = lastStep / 2.0;
            for (int i = 0; i < 16; i++) {
                if (f(midPoint) < 0.0) {
                    p = midPoint;
                    midPoint -= step;
                } else {
                    midPoint += step;
                }
                step /= 2.0;
            }
        }
    }



    if (!found) {
        color = vec3(1,1,1);
    } else {
        color = normalize(fGrad(p));
    }


    fragmentColor = vec4(color, 1);
}