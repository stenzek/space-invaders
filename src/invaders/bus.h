#pragma once
#include "common/types.h"
#include "i8080/bus.h"

namespace Invaders {
class System : public i8080::Bus
{
public:
  System(DisplayRenderer* display_renderer);
  ~System();

  bool LoadROMs(const char* base_directory);

  // Inherited via Bus
  u8 ReadMemory(i8080::MemoryAddress address) override;
  void WriteMemory(i8080::MemoryAddress address, u8 value) override;
  u8 ReadIO(i8080::MemoryAddress address) override;
  void WriteIO(i8080::MemoryAddress address, u8 value) override;

private:
  bool ReadROMToBuffer(const char* filename, void* buffer, u32 buffer_size);

  u8 m_rom[0x2000] = {};      // h - 0000-07FF, g - 0800-0FFF, f - 1000-17FF, e - 1800-1FFF
  u8 m_ram[0x2000] = {};      // 2000-23FF RAM, 2400-3FFF VRAM
};
} // namespace Invaders