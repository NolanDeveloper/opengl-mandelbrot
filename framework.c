#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*glXCreateContextAttribsARBProc) (
        Display *, GLXFBConfig, GLXContext, Bool, const int *);

void (*redraw_callback)();
void (*window_configuration_callback)(int x, int y, int width, int heignt);

static int
is_extension_supported(const char * extList, const char * extension) {
    const char * start;
    const char * where;
    const char * terminator;
    where = strchr(extension, ' ');
    if (where || *extension == '\0') return 0;
    start = extList;
    while (1) {
        where = strstr(start, extension);
        if (!where) break;
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

static int
context_error_handler(Display * display, XErrorEvent * error_event) {
    char buf[256];
    XGetErrorText(display, error_event->error_code, buf, 256);
    fputs(buf, stderr);
    fputs("\n", stderr);
    exit(-1);
}

static XEvent event;
static Display * display;
static int screen;
static Window window;
static GLXContext context;
static Colormap cmap;

static void
init_glew() {
    // Obtain opengl function addresses
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        printf("Error: %s\n", glewGetErrorString(err));
        exit(-1);
    }
}

static void
init_opengl_window() {
    display = XOpenDisplay(NULL);
    if (!display) {
        printf("Failed to open X display\n");
        exit(1);
    }
    screen = DefaultScreen(display);
    // Get a matching FB config
    static int visual_attribs[] = {
        GLX_X_RENDERABLE,   True,
        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,    GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_ALPHA_SIZE,     8,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   8,
        GLX_DOUBLEBUFFER,   True,
#if 0
        GLX_SAMPLE_BUFFERS, 1, // <-- MSAA
        GLX_SAMPLES,        4, // <-- MSAA
#endif
        None
    };
    int glx_major, glx_minor;
    // FBConfigs were added in GLX version 1.3.
    if (!glXQueryVersion(display, &glx_major, &glx_minor) ||
            ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1)) {
        printf("Invalid GLX version");
        exit(1);
    }
    printf("Getting matching framebuffer configs\n");
    int fbcount;
    GLXFBConfig * fbc =
        glXChooseFBConfig(display, screen, visual_attribs, &fbcount);
    if (!fbc) {
        printf("Failed to retrieve a framebuffer config\n");
        exit(1);
    }
    printf("Found %d matching FB configs.\n", fbcount);
    // Pick the FB config/visual with the most samples per pixel
    printf("Getting XVisualInfos\n");
    int best_fbc = -1;
    int best_num_samp = -1;
    int i;
    for (i = 0; i < fbcount; ++i) {
        XVisualInfo * vi = glXGetVisualFromFBConfig(display, fbc[i]);
        if (vi) {
            int samp_buf;
            int samples;
            glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);
            printf("    Matching fbconfig %d, visual ID 0x%2lx:"
                   " SAMPLE_BUFFERS = %d, SAMPLES = %d\n",
                   i, vi->visualid, samp_buf, samples);
            if (best_fbc < 0 || samp_buf && samples > best_num_samp) {
                best_fbc = i;
                best_num_samp = samples;
            }
        }
        XFree(vi);
    }
    GLXFBConfig bestFbc = fbc[best_fbc];
    // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
    XFree(fbc);
    // Get a visual
    XVisualInfo * vi = glXGetVisualFromFBConfig(display, bestFbc);
    printf("Chosen visual ID = 0x%lx\n", vi->visualid);
    printf("Creating colormap\n" );
    XSetWindowAttributes swa;
    swa.colormap = cmap = XCreateColormap(display,
        RootWindow(display, vi->screen), vi->visual, AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = /*ExposureMask | */StructureNotifyMask;
    printf("Creating window\n");
    window = XCreateWindow(display, RootWindow(display, vi->screen),
        0, 0, 640, 480, 0, vi->depth, InputOutput, vi->visual,
        CWBorderPixel | CWColormap | CWEventMask, &swa);
    if (!window) {
        printf("Failed to create window.\n");
        exit(1);
    }
    // Done with the visual info data
    XFree(vi);
    XStoreName(display, window, "GL 3.0 Window");
    printf("Mapping window\n");
    XMapWindow(display, window);
    // Get the default screen's GLX extension list
    const char * glxExts = glXQueryExtensionsString(display, screen);
    // NOTE: It is not necessary to create or make current to a context before
    // calling glXGetProcAddressARB
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
        glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");
    XSetErrorHandler(&context_error_handler);
    // Check for the GLX_ARB_create_context extension string and the function.
    if (!is_extension_supported(glxExts, "GLX_ARB_create_context") ||
            !glXCreateContextAttribsARB) {
        printf("glXCreateContextAttribsARB() not found\n");
        exit(-1);
    }
    int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        None
    };
    printf("Creating context\n");
    context = glXCreateContextAttribsARB(display, bestFbc, 0,
        True, context_attribs);
    // Sync to ensure any errors generated are processed.
    XSync(display, False);
    printf("Created GL 3.0 context\n");
    // Verifying that context is a direct context
    if (!glXIsDirect(display, context)) {
        printf("Indirect GLX rendering context obtained\n");
    } else {
        printf("Direct GLX rendering context obtained\n");
    }
    printf("Making context current\n");
    glXMakeCurrent(display, window, context);
    Atom delete_atom = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &delete_atom, 1);
}

static void
free_opengl_window() {
    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, context);
    XDestroyWindow(display, window);
    XFreeColormap(display, cmap);
    XCloseDisplay(display);
}

static void redraw_callback_stub() { }
static void window_configuration_callback_stub(
    int x, int y, int width, int heignt) { }

extern void
window_init() {
    redraw_callback = redraw_callback_stub;
    window_configuration_callback = window_configuration_callback_stub;
    init_opengl_window();
    init_glew();
}

extern void
window_event_loop() {
    struct timespec last_screen_update_time;
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &last_screen_update_time);
    while (1) {
        while (XPending(display)) {
            XNextEvent(display, &event);
            switch (event.type) {
            case ClientMessage:
                printf("ClientMessage\n");
                goto finish;
            case ConfigureNotify: {
                int x = event.xconfigure.x;
                int y = event.xconfigure.y;
                int width = event.xconfigure.width;
                int height = event.xconfigure.height;
                window_configuration_callback(x, y, width, height);
                break;
            }
            case Expose:
                redraw_callback();
                glXSwapBuffers(display, window);
                clock_gettime(CLOCK_MONOTONIC, &last_screen_update_time);
                break;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double dt = (double) (current_time.tv_sec
            - last_screen_update_time.tv_sec) * 1000
            + (current_time.tv_nsec - last_screen_update_time.tv_nsec)
            / 1000000;
        if (1000. / 60 < dt) {
            redraw_callback();
            glXSwapBuffers(display, window);
            last_screen_update_time = current_time;
        }
    }
finish:
    free_opengl_window();
}
