extern void (*redraw_callback)();
extern void (*window_configuration_callback)(
    int x, int y, int width, int heignt);

extern void window_init();
extern void window_event_loop();
