
#include <SDL/SDL_image.h>
#include "SDL_gpu.h"
#include "GPU_shaders.h";

int main(int argc, char **argv)
{
	SDL_Surface *screen;
	SDL_Event event;
	int quit = 0;
	struct GPU_Glsprite *glsprite;
	struct GPU_Glsprite *glsprite2;
	struct GPU_Glsprite *glsprite3;
	struct GPU_Framebuffer *buffer;
	struct GPU_Framebuffer *buffer2;
	struct GPU_GlShader *shader; 
	struct GPU_GlShader *shader2; 
	SDL_Rect dest;
	dest.x = 62;
	dest.y = 0;

// 1- inicialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit(-1);
	}
	atexit(SDL_Quit);

	
//2-Set Opengl attrs
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


//3- initialize screen
	if((screen = SDL_SetVideoMode(800, 600, 0, SDL_OPENGL | SDL_HWSURFACE)) == 0) {
		fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
        	exit(-1);
	}


//4- initialize SDL_gpu

	if (GPU_Init() == -1)
		exit(-1);
	
//	glViewport(0, 0, 800, 600);
//5- continue initializing opengl
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(0, 800, 0, 600, -1, 1);
	//gluOrtho2D(-1, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

//CheckErrorsGL();
// initialize the glsprite
    SDL_Surface *img = (SDL_Surface *) IMG_Load("lena.png");
    
//	SDL_Color colorkey = {0xFF, 0x00, 0xFF, 0};
	if ( (glsprite = (GPU_AllocGlsprite(img, 0, TARGET_TEXTURE_2D | FORMAT_RGB | TYPE_UNSIGNED_BYTE))) == 0 ) {
		fprintf(stderr, "Error loading the image file!\n");
		exit(-1);		
	}

   // initializeShaders();
	
	buffer = GPU_FramebufferInit( TARGET_TEXTURE_2D | FORMAT_RGB | TYPE_UNSIGNED_BYTE, img->w, img->h );
	shader = GPU_ShaderInit(0, wobbleFS);
	glUseProgramObjectARB( shader->program);
	GPU_PrintShaderLog(shader);
//	GPU_SendUniform1f(shader, "Alpha", 0.2f);
	GPU_SendTexture(shader, "texture", 0);
	GPU_SendUniform1f(shader, "texsize", 512.0);
	GPU_SendUniform1f(shader, "numtiles", 40000.0);
	GPU_SendUniform1f(shader, "edgewidth", 0.2);


	GPU_BlitOnFramebuffer(buffer, glsprite,  &dest);
	glUseProgramObjectARB( 0);
	
	SDL_Surface *img2 =GPU_FramebufferToSurface(buffer);
	SDL_SaveBMP(img2, "teste2.bmp" );
//	SDL_SaveBMP(GPU_FramebufferToSurface(buffer), "teste0.bmp" );


	if ( (glsprite2 = (GPU_AllocGlsprite(img2, 0, TARGET_TEXTURE_2D | FORMAT_RGB | TYPE_UNSIGNED_BYTE))) == 0 ) {
		fprintf(stderr, "Error loading the image file!\n");
		exit(-1);		
	}
	
//	GPU_FreeFramebuffer(buffer);
//	free(buffer);
		
	//buffer2 = GPU_FramebufferInit( TARGET_TEXTURE_2D | FORMAT_RGB | TYPE_UNSIGNED_BYTE, img->w, img->h );
	
/*	shader2 = GPU_ShaderInit(0, brightnessShader);
	GPU_SendTexture(shader2, "texture", 0);	
	//GPU_AddShader(buffer, shader2);
	glUseProgramObjectARB( shader2->program);
	GPU_BlitOnFramebuffer(buffer, glsprite2,  &dest);
	glUseProgramObjectARB( 0);

	if ( (glsprite3 = (GPU_AllocGlsprite(GPU_FramebufferToSurface(buffer), 0, TARGET_TEXTURE_2D | FORMAT_RGB | TYPE_UNSIGNED_BYTE))) == 0 ) {
		fprintf(stderr, "Error loading the image file!\n");
		exit(-1);		
	}

/* main loop */
	while (!quit) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_KEYDOWN:
				switch( event.key.keysym.sym ){
				case SDLK_ESCAPE:
				case SDLK_q:
					quit = 1;
					break;
				case SDLK_F1:
					SDL_WM_ToggleFullScreen(screen);
					break;
				default:
					break;
				}
			}
		}

        /* render */
		 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GPU_BlitGlsprite(glsprite2, 0);
		//GPU_BlitFramebufferOnScreen(buffer, glsprite2,  &dest);
		
		glFlush();
		SDL_GL_SwapBuffers();
		
	}

	SDL_SaveBMP(GPU_FramebufferToSurface(buffer), "buffer2shaders.bmp" );
	
// quit
    GPU_FreeGlsprite(glsprite);
	SDL_Quit();
	return 0;	
}
