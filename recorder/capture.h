#ifndef CAPTURE_H
#define CAPTURE_H

void capture_init(const char *filename, int fps);
void capture_put_frame(void);
void capture_quit(void);

#endif
