#include <link.h>
#include <stdio.h>

#include <link_map.hxx>

int LinkMap::callback(struct dl_phdr_info* info, size_t size, void *context) {
  auto func = reinterpret_cast<void(*)(const struct dl_phdr_info*)>(context);
  func(info);
  return 0;
}

void LinkMap::forEach(void(*callback)(const struct dl_phdr_info*)) {
  dl_iterate_phdr(LinkMap::callback, reinterpret_cast<void*>(callback));
}
