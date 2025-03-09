#version 450
precision highp float;

out vec4 fragmentColor;
in vec4 rayDir;

uniform struct {
    mat4 rayDirMatrix;
    vec3 position;
} camera;

uniform sampler3D voxelTexture;

uniform struct {
    vec3 direction;
    vec3 color;
    float ambient;
    float shininess;
} light;

vec3 computeNormal(vec3 p) {
    float eps = 0.01;
    
    vec3 gradient;
    gradient.x = texture(voxelTexture, p + vec3(eps, 0, 0)).a - 
                 texture(voxelTexture, p - vec3(eps, 0, 0)).a;
    gradient.y = texture(voxelTexture, p + vec3(0, eps, 0)).a - 
                 texture(voxelTexture, p - vec3(0, eps, 0)).a;
    gradient.z = texture(voxelTexture, p + vec3(0, 0, eps)).a - 
                 texture(voxelTexture, p - vec3(0, 0, eps)).a;
    
    return normalize(gradient);
}

void main(void) {
    vec3 d = normalize(rayDir.xyz);
    float t1 = -camera.position.y / d.y;
    float t2 = (1.0 - camera.position.y) / d.y;
    float tstart = max(min(t1, t2), 0.0);
    float tend = max(t1, t2);

    bool found = false;
    vec3 p;

    if (tstart < tend) {
        p = camera.position + d * tstart;
        vec3 step = d * min((tend - tstart) / 580.0, 0.01);
        for (int i = 0; i < 128; i++) {
            p += step;
            step *= 1.02;
            if(p.x > 0 && p.y > 0 && p.z > 0 && p.x < 1 && p.y < 1 && p.z < 1) {

                vec4 voxelColor = texture(voxelTexture, p);
                if (voxelColor.a > 0.0) {
                    found = true; 
                    break;
                }
            }
        }
        if (found) {
            vec3 lastStep = step / 1.02;
            vec3 midPoint =  p - lastStep / 2.0;
            step = lastStep / 2.0;
            for (int i = 0; i < 16; i++) {
            vec4 voxelColor = texture(voxelTexture, midPoint);
                if (voxelColor.a > 0.0) {
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
        discard;
    } else {
        vec4 voxelColor = texture(voxelTexture, p);
        vec3 normal = computeNormal(p);
        vec3 viewDir = normalize(camera.position - p);
        
        vec3 ambient = light.ambient * voxelColor.rgb;
        
        vec3 lightDir = normalize(-light.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * voxelColor.rgb;
      
        vec3 result = (ambient + diffuse) * voxelColor.rgb;
        
        fragmentColor = vec4(result, voxelColor.a);
    }
}