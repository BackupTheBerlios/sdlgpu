#include "SDL_gpu.h"
#include "SDL/SDL_opengl.h"
#include "GPU_shaders.h"
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/* from Opengl Programming Guide
 * return 1 if the extension extName is supported, or 0 is isn't
 */
int GPU_QueryExtension(char *extName)
{
	char *p =  glGetString(GL_EXTENSIONS);
	char *end;
	if (p == NULL)
		return 0;
	end = p + strlen(p);
	
	while(p< end){
		int n = strcspn(p, " ");
		if ((strlen(extName) == n) && (strncmp(extName, p, n) == 0)){
			return 1;
		}
		p += (n+1);
	}
	return 0;
}

/* first check if the video cards have the GL_ARB_shading_language_100 extension
 * if yes, then initialize them.
 * return -1 on an error or 0 on success
 */

int GPU_Init()
{

	if (!GPU_QueryExtension("GL_ARB_shading_language_100"))
		return -1;
	
	if (!GPU_QueryExtension("GL_EXT_framebuffer_object"))
		return -2;
	
	GPU_InitializeExtensions();
	return 0;
}

int GPU_CheckFramebufferStatus()
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return 0;
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            printf("Unsupported framebuffer format\n");
            return 1;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Framebuffer incomplete, missing attachment\n");
            return 2;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
            printf("Framebuffer incomplete, duplicate attachment\n");
            return 3;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Framebuffer incomplete, attached images must have same dimensions\n");
            return 4;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            printf("Framebuffer incomplete, attached images must have same format\n");
            return 5;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Framebuffer incomplete, missing draw buffer\n");
            return 6;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Framebuffer incomplete, missing read buffer\n");
            return 7;
            break;
        default:
            assert(0);
    }
}

void GPU_FreeFramebuffer(struct GPU_Framebuffer *buf)
{
	
	glDeleteRenderbuffersEXT(1, &(buf->depthbufferID));
	glDeleteTextures(1, &(buf->texID));
	glDeleteFramebuffersEXT(1, &(buf->framebufferID));
	
}

struct GPU_Framebuffer * GPU_FramebufferInit(int textureflags, int sizex, int sizey)
{
	struct GPU_Framebuffer *framebuffer = malloc( sizeof(struct GPU_Framebuffer ) );
	framebuffer->depthbufferID = 0;
	framebuffer->framebufferID = 0;
	framebuffer->texID = 0;
	framebuffer->sizex = sizex;
	framebuffer->sizey = sizey;
	framebuffer->numshaders = 0;
	framebuffer->shaders = 0;
	framebuffer->textureflags = textureflags;
	
	if ( textureflags &TARGET_TEXTURE_2D )
		framebuffer->textarget = GL_TEXTURE_2D;
	else if ( textureflags &TARGET_TEXTURE_RECT )
		framebuffer->textarget = GL_TEXTURE_RECTANGLE_ARB;
	else if (textureflags &TARGET_TEXTURE_CUBEMAP )
		framebuffer->textarget = GL_TEXTURE_CUBE_MAP_ARB;
	else
		return 0;
	
	GLint internalformat;
	GLenum format;
	if ( textureflags &FORMAT_RGB ){
		internalformat = GL_RGB;
		format = GL_RGB;
	}else if ( textureflags &FORMAT_RGBA ){
		internalformat = GL_RGBA;
		format = GL_RGBA;
	}else{
		return 0;
	}
	
	GLenum type;
	if ( textureflags &TYPE_UNSIGNED_BYTE )
		type = GL_UNSIGNED_BYTE;
	else if ( textureflags &TYPE_FLOAT )
		type = GL_FLOAT;
	else
		return 0;
		
	
	//initialize framebuffer struct
    glGenFramebuffersEXT(1, &(framebuffer->framebufferID)); 
    glGenTextures(1, &(framebuffer->texID));
    glGenRenderbuffersEXT(1, &(framebuffer->depthbufferID)); 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer->framebufferID);

    //initialize texture
    glBindTexture(framebuffer->textarget, framebuffer->texID);
    glTexImage2D(framebuffer->textarget, 0, internalformat, sizex, sizey, 0, format, type, NULL);
    glTexParameterf(framebuffer->textarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf(framebuffer->textarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, framebuffer->textarget, framebuffer->texID, 0);

    // initialize depth renderbuffer
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, framebuffer->depthbufferID);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, sizex, sizey);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
                                 GL_RENDERBUFFER_EXT, framebuffer->depthbufferID); 
                                 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindTexture(framebuffer->textarget, 0);
	
	GPU_CheckFramebufferStatus();
	
	return framebuffer;
}

void GPU_BlitOnFramebuffer(struct GPU_Framebuffer *buf, struct GPU_Glsprite *sprite, SDL_Rect *dest)
{
	
    glBindTexture(buf->textarget, 0);
    
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buf->framebufferID);
    {
      glViewport(0, 0, sprite->sizex, sprite->sizey);

      glClearColor(0, 0, 0, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
      glBindTexture(sprite->textarget, sprite->texID);
      glEnable(sprite->textarget);
		
	  glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
		
      glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex3f(-1, -1, -0.5f);
            glTexCoord2f(1, 0); glVertex3f( 1, -1, -0.5f);
            glTexCoord2f(1, 1); glVertex3f( 1,  1, -0.5f);
            glTexCoord2f(0, 1); glVertex3f(-1,  1, -0.5f);
     glEnd();
     

     glPopMatrix();

    glViewport(0, 0, 800, 600);
    
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDisable(sprite->textarget);
	
}

void GPU_BlitFramebufferOnScreen(struct GPU_Framebuffer *buf, struct GPU_Glsprite *sprite, SDL_Rect *dest)
{
	//adjust this to determine where render the image on the window
    glViewport(0, 0, buf->sizex, buf->sizey); 
    	
    glBindTexture(sprite->textarget, sprite->texID);
    glEnable(buf->textarget);
    glBegin(GL_QUADS);
    {
            glTexCoord2f(0, 0); glVertex3f(-1, -1, -0.5f);
            glTexCoord2f(1, 0); glVertex3f( 1, -1, -0.5f);
            glTexCoord2f(1, 1); glVertex3f( 1,  1, -0.5f);
            glTexCoord2f(0, 1); glVertex3f(-1,  1, -0.5f);
    }
    glEnd();	
    glViewport(0, 0, 800, 600); 
    glDisable(buf->textarget);														

}	

SDL_Surface* GPU_FramebufferToSurface(struct GPU_Framebuffer *buf){

	if (buf == NULL)
		return 0;
		
	SDL_Surface *surface;
	Uint32 rmask, gmask, bmask, amask;

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	surface = SDL_CreateRGBSurface(SDL_HWSURFACE, buf->sizex, buf->sizey, 24, rmask, gmask, bmask, 0);
	if(surface == NULL) {
		return 0;
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buf->framebufferID);
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glReadPixels(0, 0, surface->w, surface->h, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	return surface;
}


struct GPU_Glsprite *GPU_AllocGlsprite(SDL_Surface *pixels, SDL_Color *colorkey, int textureflags)
{
    struct GPU_Glsprite *newsprite;
	SDL_Surface *image;
	Uint32 key;
	Uint32 rmask, gmask, bmask, amask;
	
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000; gmask = 0x00ff0000; bmask = 0x0000ff00;	amask = 0x000000ff;
#else
	rmask = 0x000000ff;	gmask = 0x0000ff00;	bmask = 0x00ff0000;	amask = 0xff000000;
#endif

	newsprite = malloc( sizeof(struct GPU_Glsprite));
	newsprite->rect.x = newsprite->rect.y = 0;
	
	//if have a surface
	if ((pixels)) {

		int max_size;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
		//then check if opengl can handle it
		if ((pixels->w > max_size) || (pixels->h > max_size)) {
			SDL_FreeSurface(pixels);
			return 0;
		}

		newsprite->rect.w = pixels->w;
		newsprite->rect.h = pixels->h;

        newsprite->textureflags = textureflags;
        newsprite->sizex = pixels->w;
        newsprite->sizey = pixels->h;
	    if ( textureflags &TARGET_TEXTURE_2D )
		   newsprite->textarget = GL_TEXTURE_2D;
		else if ( textureflags &TARGET_TEXTURE_RECT )
			newsprite->textarget = GL_TEXTURE_RECTANGLE_ARB;
		else if (textureflags &TARGET_TEXTURE_CUBEMAP )
			newsprite->textarget = GL_TEXTURE_CUBE_MAP_ARB;
		else
			return 0;

		if (colorkey != NULL) {
			key = SDL_MapRGB(pixels->format, colorkey->r, colorkey->g, colorkey->b);
			image = SDL_CreateRGBSurface(SDL_HWSURFACE, pixels->w, pixels->h, 32, rmask, gmask, bmask, amask);
			SDL_FillRect(image, NULL, key); /* Disabling GL_BLEND transparent areas will be like colorkey and not black */
			SDL_SetColorKey(pixels, SDL_SRCCOLORKEY, key);
			SDL_BlitSurface(pixels, 0, image, 0);
			SDL_FreeSurface(pixels);
		}
		else {
			image = pixels;
		}
		
		/* OpenGL Texture creation */	
		glGenTextures(1, & ((newsprite)->texID));
		glBindTexture(newsprite->textarget, (newsprite)->texID);
		glTexParameteri(newsprite->textarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(newsprite->textarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		if (colorkey != NULL)
			glTexImage2D(newsprite->textarget, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
		else
			glTexImage2D(newsprite->textarget, 0, GL_RGB, image->w, image->h, 0, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);

		//newsprite->image = image;
		SDL_FreeSurface(image);
		return newsprite;
	}
	
	else{
		printf("sem pixels");
		return 0;
	}
}

void GPU_BlitGlsprite(struct GPU_Glsprite *sprite, SDL_Rect *dest)
{

  glViewport(0, 0, 512, 512); 
	glBindTexture(GL_TEXTURE_2D, sprite->texID);
	glEnable(sprite->textarget);
	glBegin(GL_QUADS);

			            glTexCoord2f(0, 0); glVertex3f(-1, -1, -0.5f);
            glTexCoord2f(1, 0); glVertex3f( 1, -1, -0.5f);
            glTexCoord2f(1, 1); glVertex3f( 1,  1, -0.5f);
            glTexCoord2f(0, 1); glVertex3f(-1,  1, -0.5f);
	glEnd();																		
	
	glDisable(sprite->textarget);
	  glViewport(0, 0, 800, 600); 
}

int GPU_FreeGlsprite(struct GPU_Glsprite *sprite)
{
	GLuint texnum;
	
	if (sprite != 0) {
		texnum = sprite->texID;
		if(glIsTexture(texnum)) {
			glDeleteTextures(1, &texnum);
		}
		free(sprite);
		return 0;
	}
	else
		return -1;
}

struct GPU_GlShader* GPU_ShaderInit(const char *vertex, const  char* fragment){
    
    struct GPU_GlShader *newshader = malloc(sizeof(struct GPU_GlShader));
    newshader->program = glCreateProgramObjectARB();
    
	if (vertex)	{
		newshader->vertex = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
  		glShaderSourceARB(newshader->vertex, 1, &vertex, NULL);
		glCompileShaderARB(newshader->vertex);
		glAttachObjectARB(newshader->program, newshader->vertex);
	}
		
	if (fragment){
		newshader->fragment = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		glShaderSourceARB(newshader->fragment, 1, &fragment, NULL);
		glCompileShaderARB(newshader->fragment);
		glAttachObjectARB(newshader->program, newshader->fragment);
	}
		
	glLinkProgramARB(newshader->program);
	
	if (vertex || fragment)
    	return newshader;
	else
    	return 0;
	
}

int GPU_SendTexture(struct GPU_GlShader *shader, char *texname, unsigned int position)
{
    GLint loc = glGetUniformLocationARB(shader->program, texname);
    if (loc == -1)
    	return -1;
	glUniform1iARB( loc, position);	
	return 0;
}

int GPU_SendUniform1f(struct GPU_GlShader *shader, char *uniform, float value)
{
    GLint loc = glGetUniformLocationARB(shader->program, uniform);
    if (loc == -1){
    	printf("Error: No uniform named: %s \n", uniform);
    	return -1;
    }
	glUniform1fARB( loc, value);	
	return 0;

}

void GPU_CheckGLErrors() 
{
	GLuint errnum;
	const char *errstr;
	while (errnum = glGetError()) 
	{
        printf("GL ERROR: %s \n",errstr);
        fflush(stdout);
	}
	return;
}

void GPU_PrintShaderLog(struct GPU_GlShader *shader)
{

	int infologlen = 0;
	int charswritten = 0;
	GLcharARB *infolog;
	
	glGetObjectParameterivARB( shader->program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologlen );
	
	if (infologlen > 0){
		infolog = (GLcharARB*)malloc(infologlen);
		if (infolog == NULL)
			return;
		glGetInfoLogARB(shader->program, infologlen, &charswritten, infolog);
		printf("Shader log: \n%s\n", infolog);
		free(infolog);
	}
	
}
