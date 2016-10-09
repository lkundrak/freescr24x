ssize_t fake_write (int fd, const void *buf, size_t count);
ssize_t fake_read (int fd, void *buf, size_t count);
int fake_ioctl (int fd, unsigned long request, char *argp);
ssize_t real_write (int fd, const void *buf, size_t count);
ssize_t real_read (int fd, void *buf, size_t count);
int real_ioctl (int fd, unsigned long request, char *argp);
extern void *pending;
