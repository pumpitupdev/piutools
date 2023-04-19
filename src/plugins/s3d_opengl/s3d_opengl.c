// Graphics Handling for S3D Engine
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

// X11 Stuff
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlibint.h>

// GL Stuff
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

// For Frame Limiter
#include <time.h>

#include <PIUTools_SDK.h>


typedef Window (*XCreateWindow_t)(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
typedef int (*glxSwapBuffers_t)(Display*, GLXDrawable);
typedef void (*glDrawPixels_t)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
typedef void (*GLTexImage2D_t)(GLenum target, GLint level, GLint internalformat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLenum format, GLenum type, const void *pixels);
typedef Display* (*XOpenDisplay_t)(char*);
static XOpenDisplay_t next_XOpenDisplay;
static XCreateWindow_t next_XCreateWindow;
static glxSwapBuffers_t next_glxSwapBuffers;
static glDrawPixels_t next_glDrawPixels;
static GLTexImage2D_t next_glTexImage2D;
static pthread_t window_change_event_thread;

enum GFX_S3D_SCALING_MODES{
    GFX_SCALING_MODE_NONE,
    GFX_SCALING_MODE_KEEP_ASPECT,
    GFX_SCALING_MODE_STRETCH
};

typedef struct _GFX_S3D_OPTIONS{
      unsigned int scaling_mode;
      unsigned int frame_limit;
      unsigned int resizable_window;
      unsigned int colormap_fix;
      unsigned int screen_width;
      unsigned int screen_height;
      unsigned int gl_threadfix;
      unsigned int texture_seam_fix;
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
static unsigned char glx_sgi_swap_supported = 0;
static unsigned char glx_ext_swap_supported = 0;
static unsigned char glx_mesa_swap_supported = 0;
static unsigned char glx_oml_swap_supported = 1;

// -- HELPERS --

// Typical sleep until elapsed exit time
static void wait_frame_limit(unsigned int fps) {
    static struct timespec last_swap_time = {0, 0};

    if (fps == 0) {
        return;
    }

    unsigned int swap_interval_nsec = 1000000000 / fps;
    struct timespec current_time;
    struct timespec wait_time;

    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Calculate the time elapsed since the last swap
    unsigned int elapsed_nsec = (current_time.tv_sec - last_swap_time.tv_sec) * 1000000000
                                 + (current_time.tv_nsec - last_swap_time.tv_nsec);

    if (elapsed_nsec < swap_interval_nsec) {
        wait_time.tv_sec = (swap_interval_nsec - elapsed_nsec) / 1000000000;
        wait_time.tv_nsec = (swap_interval_nsec - elapsed_nsec) % 1000000000;

        clock_nanosleep(CLOCK_MONOTONIC, 0, &wait_time, NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &last_swap_time);
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

int is_extension_supported(const char *extensions, const char *extension) {
    const char *start = extensions;
    const char *where, *terminator;

    while (1) {
        where = strstr(start, extension);
        if (!where) {
            break;
        }
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ') {
            if (*terminator == ' ' || *terminator == '\0') {
                return 1;
            }
        }
        start = terminator;
    }
    return 0;
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


void *handle_window_events(void *arg) {
    Display *dpy = (Display *)arg;
    XEvent event;

    while (1) {
      while (XCheckMaskEvent(dpy, StructureNotifyMask, &event)) {
         if (event.type == ConfigureNotify) {
          XConfigureEvent xce = event.xconfigure;
           // Check if the window size changed.
            if (xce.width != target_display_width || xce.height != target_display_height) {
                target_display_width = xce.width;
                target_display_height = xce.height;


                // Recalculate Zoom Factor
                calculate_zoom_factors(initial_display_width,initial_display_height,target_display_width,target_display_height,&zoom_factor_x,&zoom_factor_y);
                // Update your zoom level or perform other actions based on the new dimensions.
                
            }    
    }
    }
    }      
    return NULL;
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

    Window res = next_XCreateWindow(display,parent,x,y,width,height,border_width,depth,_class,visual,valuemask,attributes);

  // We're going to check if glx swap is available and fix invalid calls as a result.
    const char *extensions = glXQueryExtensionsString(display, DefaultScreen(display));
    glx_sgi_swap_supported = is_extension_supported(extensions, "GLX_SGI_swap_control");
    glx_mesa_swap_supported = is_extension_supported(extensions,"GLX_MESA_swap_control");
    glx_ext_swap_supported = is_extension_supported(extensions,"GLX_EXT_swap_control");
    glx_oml_swap_supported = is_extension_supported(extensions,"GLX_OML_swap_method");
    printf("[%s] Swap Control Support: SGI:%d MESA:%d EXT:%d OML:%d\n",__FILE__,glx_sgi_swap_supported,glx_mesa_swap_supported,glx_ext_swap_supported,glx_oml_swap_supported);

    // Create a new thread to handle window events
   // pthread_t thread;
   // pthread_create(&thread, NULL, handle_window_events, display);
    return res;
}



Display* s3d_XOpenDisplay(char *display_name){
  //XInitThreads();
  Display* dpy = next_XOpenDisplay(display_name);
  if(options_gfx_s3d.screen_height == 0){
    return dpy;
  }
  int screen = DefaultScreen(dpy);
  dpy->screens[screen].height = options_gfx_s3d.screen_height;
  return dpy;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Call the original glTexImage2D function
    next_glTexImage2D(target, level, internalformat, width, height,
                          border, format, type, pixels);
}

typedef int (*PFNGLXSWAPINTERVALSGIPROC)(int interval);
PFNGLXSWAPINTERVALSGIPROC next_glXSwapIntervalSGI;
static int s3d_glXSwapIntervalSGI(int interval){
  if(glx_sgi_swap_supported == 0){return 0;}
  return next_glXSwapIntervalSGI(interval);
}

/*
  We may need this patch in the future for NXA 1.08, for now it seems fixed, but who knows:

  static const float knownGood[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, -240.0f, 1.0f
};

   Pump periodically takes a shit on the modelview matrix if you don't
   run it from its specialised OS. I have no idea why. However, the
   quasi-identity matrix given above (it's transposed, btw) seems to
   be the matrix it intends to construct when calling this function.

void gluLookAt(double x0, double y0, double z0, double x1, double y1,
    double z1, double x2, double y2, double z2)
{
    float m[16];
    int i;

    realLookAt(x0, y0, z0, x1, y1, z1, x2, y2, z2);
    realGetFloatv(0x0BA6, m);

    for (i = 0 ; i < 16 ; i++) {
         if (isnan(m[i])) {
            realLoadMatrixf(knownGood);
            return;
        }
    }
}

*/



static HookEntry entries[] = {   
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6","XCreateWindow", s3d_XCreateWindow, &next_XCreateWindow, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glDrawPixels", s3d_glDrawPixels, &next_glDrawPixels, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glXSwapBuffers", s3d_glXSwapBuffers, &next_glxSwapBuffers, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glTexImage2D", s3d_glTexImage2D, &next_glTexImage2D, 0),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glXSwapIntervalSGI", s3d_glXSwapIntervalSGI, &next_glXSwapIntervalSGI, 1),      
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6","XOpenDisplay", s3d_XOpenDisplay, &next_XOpenDisplay, 1),    
    {}       
};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("S3D_OPENGL","frame_limit",CONFIG_TYPE_INT,&options_gfx_s3d.frame_limit ,sizeof(options_gfx_s3d.frame_limit)),
  CONFIG_ENTRY("S3D_OPENGL","resizable_window",CONFIG_TYPE_BOOL, &options_gfx_s3d.resizable_window,sizeof(options_gfx_s3d.resizable_window)),
  CONFIG_ENTRY("S3D_OPENGL","colormap_fix",CONFIG_TYPE_BOOL, &options_gfx_s3d.colormap_fix,sizeof(options_gfx_s3d.colormap_fix)),
  CONFIG_ENTRY("S3D_OPENGL","screen_width",CONFIG_TYPE_INT, &options_gfx_s3d.screen_width,sizeof(options_gfx_s3d.screen_width)),
  CONFIG_ENTRY("S3D_OPENGL","screen_height",CONFIG_TYPE_INT, &options_gfx_s3d.screen_height,sizeof(options_gfx_s3d.screen_height)),
  CONFIG_ENTRY("S3D_OPENGL","scaling_mode",CONFIG_TYPE_INT, &options_gfx_s3d.scaling_mode,sizeof(options_gfx_s3d.scaling_mode)),
  CONFIG_ENTRY("S3D_OPENGL","texture_seam_fix",CONFIG_TYPE_BOOL, &options_gfx_s3d.texture_seam_fix,sizeof(options_gfx_s3d.texture_seam_fix)),
  CONFIG_ENTRY("S3D_OPENGL","gl_single_threadfix",CONFIG_TYPE_BOOL, &options_gfx_s3d.gl_threadfix,sizeof(options_gfx_s3d.gl_threadfix)),
  {}
};

const PHookEntry plugin_init(void) {
  PIUTools_Config_Read(plugin_config);

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
      setenv("__GL_SINGLETHREADED","1",1);  
  }

  if(options_gfx_s3d.texture_seam_fix == 1){
      DBG_printf("[%s] Texture Seam Clamp Fix",__FILE__);     
      entries[3].hook_enabled=1; 
  }      
  return entries;
}