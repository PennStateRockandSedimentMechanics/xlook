struct rs_fric_window;

struct rs_fric_window *create_rs_fric_window(void);
void update_parameters(struct rs_fric_window *window); // was update_params

void bring_rs_fric_window_to_front(struct rs_fric_window *window);
