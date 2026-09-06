// Common Engine Provided Uniforms

#ifndef TEXTURE_UNIFORM_
#define TEXTURE_UNIFORM_
uniform sampler2D u_Texture;         // previous pass output
#endif


#ifndef RESOLUTION_UNIFORM_
#define RESOLUTION_UNIFORM_
uniform vec2 u_Resolution;           // render target size in px
#endif

#ifndef TEXEL_UNIFORM_
#define TEXEL_UNIFORM_
uniform vec2 u_TexelSize;            // 1 / u_Resolution
#endif

#ifndef TIME_UNIFORM_
#define TIME_UNIFORM_
uniform float u_Time;                // seconds since engine start
#endif

#ifndef SCENE_UNIFORM_
#define SCENE_UNIFORM_
uniform sampler2D u_Scene;           // original scene texture (1, when wantsSceneTexture)
#endif

