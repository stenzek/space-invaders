#pragma once
#include "common/types.h"
#include "i8080/cpu.h"
#include "i8080/bus.h"
#include <memory>

class SimpleDisplay;

namespace Invaders {

struct Inputs
{
  union // INP0
  {
    BitField<u8, bool, 0, 1> dip4;
    BitField<u8, bool, 4, 1> fire;
    BitField<u8, bool, 5, 1> left;
    BitField<u8, bool, 6, 1> right;
    u8 INP0_bits;
  };

  union // INP1
  {
    BitField<u8, bool, 0, 1> credit;
    BitField<u8, bool, 1, 1> start_2p;
    BitField<u8, bool, 2, 1> start_1p;
    BitField<u8, bool, 4, 1> fire_1p;
    BitField<u8, bool, 5, 1> left_1p;
    BitField<u8, bool, 6, 1> right_1p;
    u8 INP1_bits;
  };

  union // INP2
  {
    BitField<u8, bool, 0, 1> dip3;
    BitField<u8, bool, 1, 1> dip5;
    BitField<u8, bool, 2, 1> tilt;
    BitField<u8, bool, 3, 1> dip6;
    BitField<u8, bool, 1, 1> start_2p;
    BitField<u8, bool, 2, 1> start_1p;
    BitField<u8, bool, 4, 1> fire_2p;
    BitField<u8, bool, 5, 1> left_2p;
    BitField<u8, bool, 6, 1> right_2p;
    BitField<u8, bool, 6, 1> dip7;
    u8 INP2_bits;
  };
};

class System : public i8080::Bus
{
public:
  System();
  ~System();

  const Inputs& GetInputs() const { return m_inputs; }
  Inputs& GetInputs() { return m_inputs; }

  bool LoadROMs(const char* base_directory);

  bool Initialize(SimpleDisplay* display);
  void Reset();

  // Executes a frame
  void ExecuteFrame();

  // Inherited via Bus
  void AddCycles(CycleCount cycles) override;
  u8 ReadMemory(i8080::MemoryAddress address) override;
  void WriteMemory(i8080::MemoryAddress address, u8 value) override;
  u8 ReadIO(i8080::MemoryAddress address) override;
  void WriteIO(i8080::MemoryAddress address, u8 value) override;

private:
  static constexpr CycleCount INTERRUPT_CYCLE_INTERVAL = 17066;
  static constexpr u32 DISPLAY_WIDTH = 256;
  static constexpr u32 DISPLAY_HEIGHT = 224;

  bool ReadROMToBuffer(const char* filename, void* buffer, u32 buffer_size);
  void InitColorMask();
  void RenderDisplay();

  u8 Read_SHFT_IN();
  void Write_SHFT_AMNT(u8 val);
  void Write_SHFT_DATA(u8 val);
  void Write_SOUND1(u8 val);
  void Write_SOUND2(u8 val);
  void Write_WATCHDOG(u8 val);

  SimpleDisplay* m_display = nullptr;

  i8080::CPU m_cpu;

  u8 m_rom[0x2000] = {};      // h - 0000-07FF, g - 0800-0FFF, f - 1000-17FF, e - 1800-1FFF
  u8 m_ram[0x2000] = {};      // 2000-23FF RAM, 2400-3FFF VRAM

  Inputs m_inputs = {};

  u16 m_shift_register_value = 0;
  u8 m_shift_register_read_offset = 0;

  CycleCount m_cycles_to_next_interrupt = INTERRUPT_CYCLE_INTERVAL;
  bool m_last_interrupt_was_vblank = true;

  std::vector<u32> m_color_mask;
};
} // namespace Invaders