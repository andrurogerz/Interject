#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <elfio/elfio.hpp>

#include <linux/elf.h>

#include <link_map.hxx>

static void loadElf(const char* file_path) {
  ELFIO::elfio reader;
  if (!reader.load(file_path)) {
    fprintf(stderr, "failed to load %s as an ELF file\n", file_path);
    return;
  }

  printf("Loaded ELF file \"%s\" class:%s\n", file_path,
      reader.get_class() == ELFCLASS32 ? "ELF32" : "ELF64");

  const ELFIO::Elf_Half section_count = reader.sections.size();
  for (ELFIO::Elf_Half i = 0; i < section_count; i++) {
    ELFIO::section *section = reader.sections[i];
    printf("section %u: \%s\n", i, section->get_name().c_str());
    if (section->get_type() == SHT_SYMTAB ||
        section->get_type() == SHT_DYNSYM) {
      const ELFIO::symbol_section_accessor symbols(reader, section);
      const ELFIO::Elf_Xword symbol_count = symbols.get_symbols_num();
      printf("%u symbols\n", symbol_count);
      /*
      for (ELFIO::Elf_Xword j = 0; j < symbol_count; j++) {
        std::string name;
        Elf64_Addr value;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half section_index;
        unsigned char other;
        if (symbols.get_symbol(j, name, value, size, bind, type, section_index, other)) {
          std::cout << j << " " << name << std::endl;
        }
      }
      */
    }
  }
}

int main(int argc, char *argv[]) {
  LinkMap::forEach([](const dl_phdr_info *info) {
    loadElf(info->dlpi_name);
    });
  return 0;
}
