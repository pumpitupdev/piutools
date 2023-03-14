// Graphics Handling for S3D Engine
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// X11 Stuff
#include <X11/Xlib.h>

// GL Stuff
#include <GL/gl.h>
#include <GL/glx.h>

// For Frame Limiter
#include <unistd.h> 
#include <sys/time.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


typedef Window (*XCreateWindow_t)(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
typedef int (*glxSwapBuffers_t)(Display*, GLXDrawable);
typedef void (*glDrawPixels_t)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
typedef void (*GLTexImage2D_t)(GLenum target, GLint level, GLint internalformat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLenum format, GLenum type, const void *pixels);

static XCreateWindow_t next_XCreateWindow;
static glxSwapBuffers_t next_glxSwapBuffers;
static glDrawPixels_t next_glDrawPixels;
static GLTexImage2D_t next_glTexImage2D;

enum GFX_S3D_SCALING_MODES{
    GFX_SCALING_MODE_NONE,
    GFX_SCALING_MODE_KEEP_ASPECT,
    GFX_SCALING_MODE_STRETCH
};

typedef struct _GFX_S3D_OPTIONS{
      unsigned char scaling_mode;
      unsigned short frame_limit;
      unsigned char resizable_window;
      unsigned char colormap_fix;
      unsigned short screen_width;
      unsigned short screen_height;
      unsigned long gl_threadfix;
      unsigned char texture_seam_fix;
}PatchGfxS3DOptions,*PPatchGfxS3DOptions;

PatchGfxS3DOptions options_gfx_s3d = {
    .frame_limit = 0,
    .resizable_window = 0,
    .colormap_fix = 1,
    .screen_width = 0,
    .screen_height = 0,
    .scaling_mode = 0,
    .gl_threadfix = 0,
    .texture_seam_fix = 0
};

// Some Locals
static unsigned short initial_display_width = 0;
static unsigned short initial_display_height = 0;
static unsigned short target_display_width = 0;
static unsigned short target_display_height = 0;
static float zoom_factor_x = 0.0f;
static float zoom_factor_y = 0.0f;
static unsigned int last_swap_time = 0;


// -- HELPERS --

// Typical sleep until elapsed exit time
static void wait_frame_limit(unsigned int fps){
    unsigned int current_time = 0;
    unsigned int swap_interval_usec = 1000000 / fps; // swap every 1/60th of a second
    // Wait until it's time to swap again
    do {
        // Get the current time
        struct timeval tv;
        gettimeofday(&tv, NULL);
        current_time = tv.tv_sec * 1000000 + tv.tv_usec;

        // Calculate the time elapsed since the last swap
        unsigned int elapsed_usec = current_time - last_swap_time;

        // If less than 1/60th of a second has elapsed, wait
        if (elapsed_usec < swap_interval_usec) {
            usleep(swap_interval_usec - elapsed_usec);
        }
    } while (current_time - last_swap_time < swap_interval_usec);

    // Record the time of this swap
    last_swap_time = current_time;
}

static void calculate_zoom_factors(int src_width, int src_height, int dest_width, int dest_height, float *zoom_x, float *zoom_y) {
    *zoom_x = (float) dest_width / (float) src_width;
    *zoom_y = (float) dest_height / (float) src_height;
}

// Check if our window dimensions changed and update our settings.
static void GetCurrentWindowDimensions(Display *dpy, GLXDrawable drawable){
  Window root;
  int x, y;
  unsigned int width, height, border_width, depth;
  XGetGeometry(dpy, drawable, &root, &x, &y, &width, &height, &border_width, &depth);
  if (width != target_display_width || height != target_display_height) {
    target_display_width = width;
    target_display_height = height;
    // Recalculate Zoom Factor
    calculate_zoom_factors(initial_display_width,initial_display_height,target_display_width,target_display_height,&zoom_factor_x,&zoom_factor_y);
  }
}


// A modified version of the S3DResize function from the engine to support multimode adjustment.
static void S3DResizeEx(void){
  if (options_gfx_s3d.scaling_mode == GFX_SCALING_MODE_STRETCH) {
    glViewport(0, 0, target_display_width, target_display_height);
  } else if (options_gfx_s3d.scaling_mode == GFX_SCALING_MODE_KEEP_ASPECT) {
    float targetAspectRatio = initial_display_width / (float) initial_display_height;

    // Figure out the largest area that fits in this resolution at the desired aspect ratio
    int width = target_display_width;
    int height = (int) (width / targetAspectRatio + 0.5f);

    if (height > target_display_height) {
      // It doesn't fit our height, we must switch to pillarbox then
      height = target_display_height;
      width = (int) (height * targetAspectRatio + 0.5f);
    }

    // Calculate pillarbox dimensions
    int pillarbox_x = (target_display_width - width) / 2;
    int pillarbox_y = (target_display_height - height) / 2;
    if (width != target_display_width || height != target_display_height) {
      target_display_width = width;
      target_display_height = height;
      calculate_zoom_factors(initial_display_width,initial_display_height,target_display_width,target_display_height,&zoom_factor_x,&zoom_factor_y);
    }

    // Set up the new viewport with pillarboxing
    glViewport(pillarbox_x, pillarbox_y, width, height);
  }
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, initial_display_width, 0.0f, initial_display_height, -500.0f, 500.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static Window s3d_XCreateWindow(Display *display,Window parent,int x,int y,unsigned int width,unsigned int height,unsigned int border_width,int depth,unsigned int _class,Visual *visual,unsigned long valuemask,XSetWindowAttributes *attributes){
    /* 
    During the Exceed-Era, S3D did not include CWColorMap for creation of a window. This is thought to be due to NVIDIA GPUs providing a hardware colormap.
    While in several cases, the window would be created fine on an NVIDIA GPU-driven XServer, AM later re-added CWColorMap to support more GPUs.
    */
    
    if(options_gfx_s3d.colormap_fix){
        if((valuemask & CWColormap) == 0){
            valuemask |= CWColormap;
        }
    }

    // This is a super hack because glDrawPixels isn't properly resolved unless you're in runtime.
    next_glDrawPixels = (glDrawPixels_t)glXGetProcAddress((const GLubyte *)"glDrawPixels");
    next_glTexImage2D = (GLTexImage2D_t)glXGetProcAddress("glTexImage2D");
    // Cache startup dimensions.
    initial_display_width = width;
    initial_display_height = height;
    target_display_width = options_gfx_s3d.screen_width;
    target_display_height = options_gfx_s3d.screen_height;

    // If our target dimensions were zero, we will disable the flag to do any
    // resizing.
    if (target_display_height == 0) {
      target_display_height = height;
    }
    if (target_display_width == 0) {
      target_display_width = width;
    }
    // We also need zoom factor
    calculate_zoom_factors(initial_display_width,initial_display_height,target_display_width,target_display_height,&zoom_factor_x,&zoom_factor_y);

    // If we're not using the resizable window flag and our resolutions match,
    // we won't do any adjustment.
    if (options_gfx_s3d.resizable_window != 0 || target_display_width != width || target_display_height != height) {
        width = target_display_width;
        height = target_display_height;
    }

    return next_XCreateWindow(display,parent,x,y,width,height,border_width,depth,_class,visual,valuemask,attributes);
}

 #ifndef GL_UNSIGNED_SHORT_5_6_5
 #define GL_UNSIGNED_SHORT_5_6_5 0x8363
 #endif

// Check for fullscreen draw calls and scale them appropriately.
static void s3d_glDrawPixels(GLsizei width,GLsizei height,GLenum format,GLenum type,const GLvoid *pixels){
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // If our texture is the original screen size, we'll zoom, flip it because
  // reasons, and then draw it to our updated zoom factor.
  if (width == initial_display_width && height == initial_display_height) {

               
    // Do this - I don't remember why but do it.
    glRasterPos2i(0, 0);
    glPixelZoom(zoom_factor_x, zoom_factor_y);
    
    // Create a new buffer to store the flipped image data
    if(type == GL_UNSIGNED_BYTE) {
      GLubyte *flippedPixels = malloc(width * height * sizeof(GLubyte) * 3);

      // Flip the image data
      for (int i = 0; i < height; i++) {
        GLubyte *srcLine = ((GLubyte *) pixels) + i * width * 3;
        GLubyte *dstLine = flippedPixels + (height - i - 1) * width * 3;
        memcpy(dstLine, srcLine, width * 3);
      }
      // Draw our Updated Image Data
      next_glDrawPixels(width, height, format, type, flippedPixels);
      // Free the flipped image data buffer
      free(flippedPixels);
    }else if (type == GL_UNSIGNED_SHORT_5_6_5) {
      GLushort *flippedPixels = malloc(width * height * sizeof(GLushort));

      // Flip the image data
      for (int i = 0; i < height; i++) {
          GLushort *srcLine = ((GLushort *) pixels) + i * width;
          GLushort *dstLine = flippedPixels + (height - i - 1) * width;
          memcpy(dstLine, srcLine, width * sizeof(GLushort));
      }

      // Draw our updated image data
      next_glDrawPixels(width, height, format, type, flippedPixels);

      // Free the flipped image data buffer
      free(flippedPixels);
  }

    
    // Reset pixel zoom
    glPixelZoom(1.0, 1.0);

  } else {
    // Call the original glDrawPixels function
    next_glDrawPixels(width, height, format, type, pixels);
  }
}

static void s3d_glXSwapBuffers(Display *dpy, GLXDrawable drawable){
    // If we use a resizable window, we must refresh the target dimensions.
    if (options_gfx_s3d.resizable_window) {
      GetCurrentWindowDimensions(dpy, drawable);
    }

    if (options_gfx_s3d.scaling_mode) {
        S3DResizeEx();
    }

    next_glxSwapBuffers(dpy, drawable);

  // We'll wait until the next frame if we need to slow things down.
  if (options_gfx_s3d.frame_limit) {
    wait_frame_limit(options_gfx_s3d.frame_limit);
  }
}

void s3d_glTexImage2D(GLenum target, GLint level, GLint internalformat,
                     GLsizei width, GLsizei height, GLint border,
                     GLenum format, GLenum type, const void *pixels) {
    // Set the texture wrapping mode to GL_CLAMP_TO_EDGE
    if(options_gfx_s3d.texture_seam_fix == 1){
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Call the original glTexImage2D function
    next_glTexImage2D(target, level, internalformat, width, height,
                          border, format, type, pixels);
}

static HookEntry entries[] = {
    {"libX11.so.6","XCreateWindow",(void*)s3d_XCreateWindow,(void*)&next_XCreateWindow},
    {"libGL.so.1","glDrawPixels",(void*)s3d_glDrawPixels,(void*)&next_glDrawPixels},
    {"libGLX.so.0","glXSwapBuffers",(void*)s3d_glXSwapBuffers,(void*)&next_glxSwapBuffers},
    {"libGL.so.1","glTexImage2D",(void*)s3d_glTexImage2D,(void*)&next_glTexImage2D},   
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    char *ptr;  
    if(strcmp(section,"S3D_OPENGL") == 0){
        if(strcmp(name,"frame_limit") == 0){
            if(value != NULL){options_gfx_s3d.frame_limit = strtoul(value,&ptr,10);}
        }
        if(strcmp(name,"resizable_window") == 0){
            if(value != NULL){options_gfx_s3d.resizable_window = strtoul(value,&ptr,10);}
        }   
        if(strcmp(name,"colormap_fix") == 0){
            if(value != NULL){options_gfx_s3d.colormap_fix = strtoul(value,&ptr,10);}
        }        
        if(strcmp(name,"screen_width") == 0){
            if(value != NULL){options_gfx_s3d.screen_width = strtoul(value,&ptr,10);}
        }         
        if(strcmp(name,"screen_height") == 0){
            if(value != NULL){options_gfx_s3d.screen_height = strtoul(value,&ptr,10);}
        }    
        if(strcmp(name,"scaling_mode") == 0){
            if(value != NULL){options_gfx_s3d.scaling_mode = strtoul(value,&ptr,10);}
        }  
        if(strcmp(name,"texture_seam_fix") == 0){
            if(value != NULL){options_gfx_s3d.texture_seam_fix = strtoul(value,&ptr,10);}
        }                  
        if(strcmp(name,"gl_single_threadfix") == 0){
            if(value != NULL){
              options_gfx_s3d.gl_threadfix = strtoul(value,&ptr,10);
              if(options_gfx_s3d.gl_threadfix == 1){
                setenv("__GL_SINGLETHREADED","1",1);
              }
            }
        } 
                            
    }
    return 1;
}


int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    if(ini_parse(config_path,parse_config,NULL) != 0){return 0;}
    if(options_gfx_s3d.colormap_fix){
      DBG_printf("[%s] GFX ColorMap Fix Enabled: %d",__FILE__,options_gfx_s3d.colormap_fix);
    }
    if(options_gfx_s3d.scaling_mode){
        DBG_printf("[%s] GFX Scaling Mode Enabled: %d",__FILE__,options_gfx_s3d.scaling_mode);
    }

    if(options_gfx_s3d.screen_height || options_gfx_s3d.screen_width){
        DBG_printf("[%s] GFX Forced Resolution Enabled: %d x %d",__FILE__,options_gfx_s3d.screen_width,options_gfx_s3d.screen_height);
    }

    if(options_gfx_s3d.frame_limit){
        DBG_printf("[%s] Enabled frame limit: %d FPS",__FILE__,options_gfx_s3d.frame_limit);
    }

    if(options_gfx_s3d.resizable_window){
        DBG_printf("[%s] Enabled resizable Window: %d",__FILE__,options_gfx_s3d.resizable_window);
    }
    
    if(options_gfx_s3d.gl_threadfix == 1){
        DBG_printf("[%s] Enabled Single Threaded OpenGL Fix",__FILE__);      
    }

    if(options_gfx_s3d.texture_seam_fix == 1){
        DBG_printf("[%s] Texture Seam Clamp Fix",__FILE__);      
    }    
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}
