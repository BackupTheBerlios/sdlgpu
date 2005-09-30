#ifndef _SDL_GPU_H_
#define _SDL_GPU_H_

#include "SDL/SDL.h"
#include "GPU_glextensions.h"

/* USO:
 * 1- Initialize SDL
 * 2- Set OpenGL attrs
 * 3- SInitialize screen
 * 4- Initialize SDL_gpu
 * 5- Continue seting Opengl
 */

struct GPU_Glsprite {
	unsigned int texID;
	SDL_Rect rect;
	GLenum textarget;
	int textureflags;
	int sizex, sizey;
	//SDL_Surface *image;
};

struct GPU_Framebuffer{
	GLuint framebufferID;  // color render target
	GLuint depthbufferID; // depth render target
	GLuint *texID; // texture
	GLenum textarget;
	int textureflags;
	int sizex, sizey;
	int ntex;
	
	unsigned int numshaders;
	struct GPU_GlShader **shaders;
};

struct GPU_GlShader{
	
	int shadertype;
	GLhandleARB fragment, vertex, program;
	
};

enum { TARGET_TEXTURE_2D = 1 << 0,
	   TARGET_TEXTURE_RECT = 1 << 1,
	   TARGET_TEXTURE_CUBEMAP = 1 << 2,
	   
	   FORMAT_RGB = 1 << 3,
	   FORMAT_RGBA = 1 << 4,
	   
	   TYPE_UNSIGNED_BYTE = 1 << 5,
	   TYPE_FLOAT = 1 << 6,
	   
	   CLAMP_TO_EDGE = 1 << 7,
   	   CLAMP = 1 << 8,
   	   REPEAT = 1 << 9
};


void GPU_CheckGLErrors( );

int GPU_QueryExtension(char *extname);
int GPU_Init();

struct GPU_GlShader* GPU_ShaderInit(const char *vertex, const char* fragment);
int GPU_SendTexture(struct GPU_GlShader *shader, char * texname, unsigned int position);
int GPU_SendUniform1f(struct GPU_GlShader *shader, char *uniform, float value);
void GPU_PrintShaderLog(struct GPU_GlShader *shader);

struct GPU_Framebuffer *GPU_FramebufferInit(int textureflags, int sizex, int sizey);
void GPU_FreeFramebuffer(struct GPU_Framebuffer *buf);
void GPU_BlitFramebufferOnScreen(struct GPU_Framebuffer *buf, struct GPU_Glsprite *sprite, SDL_Rect *dest);
void GPU_BlitOnFramebuffer(struct GPU_Framebuffer *buf, struct GPU_Glsprite *sprite, SDL_Rect *dest);
SDL_Surface* GPU_FramebufferToSurface(struct GPU_Framebuffer *buf);
int GPU_CheckFrameBufferStatus();

struct GPU_Glsprite *GPU_AllocGlsprite(SDL_Surface *pixels, SDL_Color *colorkey, int textureflags);
void GPU_BlitGlsprite( struct GPU_Glsprite *sprite, SDL_Rect *dest );
int GPU_FreeGlsprite( struct GPU_Glsprite *sprite);

#endif //_SDL_GPU_H_
