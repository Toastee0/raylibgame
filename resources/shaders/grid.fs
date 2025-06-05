#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Input uniform values
uniform sampler2D gridTexture;
uniform vec2 gridSize;
uniform float zoom;
uniform vec2 pan;
uniform float time;

void main()
{
    // Apply zoom and pan to the texture coordinates
    vec2 texCoord = (fragTexCoord * zoom + pan / gridSize);
    
    // Sample the grid texture
    vec4 texelColor = texture(gridTexture, texCoord);
    
    // Add some subtle animation to certain materials (e.g., water, fire)
    // This is based on the alpha channel which we'll use to encode material type
    if (texelColor.a > 0.3 && texelColor.a < 0.5) {  // Water
        float ripple = sin(texCoord.x * 50.0 + texCoord.y * 30.0 + time * 2.0) * 0.02;
        texelColor.rgb += vec3(ripple, ripple, ripple * 2.0);
    } else if (texelColor.a > 0.6 && texelColor.a < 0.8) {  // Fire
        float flicker = sin(texCoord.y * 40.0 + time * 8.0) * 0.1 + 
                        cos(texCoord.x * 40.0 + time * 5.0) * 0.05;
        texelColor.rgb += vec3(flicker, flicker * 0.5, 0.0);
    }
    
    // Apply fragment color (tint)
    finalColor = texelColor * fragColor;
}
