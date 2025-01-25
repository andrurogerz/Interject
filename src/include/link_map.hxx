#include <link.h>

class LinkMap {
public:
  static void forEach(void(*)(const struct dl_phdr_info*));

private:
  static int callback(struct dl_phdr_info* info, size_t size, void* data);
};
