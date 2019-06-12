#include "system.h"
#include "YBaseLib/Log.h"
#include "common/simple_display.h"
#include "common/util.h"
#include "i8080/cpu.h"
#include <cstdio>
Log_SetChannel(Bus);

namespace Invaders {
System::System() : m_cpu(this) {}

System::~System() = default;

bool System::LoadROMs(const char* base_directory)
{
  return (ReadROMToBuffer(Util::StringFromFormat("%s/invaders.h", base_directory).c_str(), &m_rom[0x0000], 0x800) &&
          ReadROMToBuffer(Util::StringFromFormat("%s/invaders.g", base_directory).c_str(), &m_rom[0x0800], 0x800) &&
          ReadROMToBuffer(Util::StringFromFormat("%s/invaders.f", base_directory).c_str(), &m_rom[0x1000], 0x800) &&
          ReadROMToBuffer(Util::StringFromFormat("%s/invaders.e", base_directory).c_str(), &m_rom[0x1800], 0x800));
}

bool System::Initialize(SimpleDisplay* display)
{
  m_display = display;
  m_display->ResizeFramebuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  m_display->SetRotation(90.0f);
  m_display->SetDisplayAspectRatio(1, 1);
  m_display->SetDisplayScale(2);
  m_display->ResizeDisplay();
  InitColorMask();
  return true;
}

void System::Reset()
{
  m_cpu.Reset();
  m_cycles_to_next_interrupt = INTERRUPT_CYCLE_INTERVAL;
  m_last_interrupt_was_vblank = true;
  m_shift_register_value = 0;
  m_shift_register_read_offset = 0;

  m_display->ResetFramesRendered();
  m_display->ClearFramebuffer();
}

void System::ExecuteFrame()
{
  if (m_last_interrupt_was_vblank)
    m_cpu.ExecuteCycles(m_cycles_to_next_interrupt);

  m_cpu.ExecuteCycles(m_cycles_to_next_interrupt);
}

void System::AddCycles(CycleCount cycles)
{
  m_cycles_to_next_interrupt -= cycles;
  if (m_cycles_to_next_interrupt > 0)
    return;

  m_last_interrupt_was_vblank = !m_last_interrupt_was_vblank;
  m_cycles_to_next_interrupt += INTERRUPT_CYCLE_INTERVAL;
  m_cpu.InterruptRequest(true, m_last_interrupt_was_vblank ? 2 : 1);
  if (m_last_interrupt_was_vblank)
    RenderDisplay();
}

u8 System::ReadMemory(i8080::MemoryAddress address)
{
  switch ((address >> 12) & 0xF)
  {
    case 0x0:
    case 0x1:
      return m_rom[address & static_cast<i8080::MemoryAddress>(0x1FFF)];

    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
      return m_ram[address & static_cast<i8080::MemoryAddress>(0x1FFF)];

    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    default:
      Log_ErrorPrintf("Unhandled read: 0x%04X", address);
      return 0xFF;
  }
}

void System::WriteMemory(i8080::MemoryAddress address, u8 value)
{
  switch ((address >> 12) & 0xF)
  {
    case 0x0:
    case 0x1:
      return;

    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
      m_ram[address & static_cast<i8080::MemoryAddress>(0x1FFF)] = value;
      return;

    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
    default:
      Log_ErrorPrintf("Unhandled write: 0x%04X <- 0x%02X", address, value);
      return;
  }
}

u8 System::ReadIO(i8080::MemoryAddress address)
{
  switch (address)
  {
    case 0x00:
      return m_inputs.INP0_bits | u8(0b00001110);
    case 0x01:
      return m_inputs.INP1_bits | u8(0b00001000);
    case 0x02:
      return m_inputs.INP2_bits;
    case 0x03:
      return Read_SHFT_IN();
    default:
      Log_WarningPrintf("Unhandled I/O port read: 0x%04X", address);
      return 0xFF;
  }
}

void System::WriteIO(i8080::MemoryAddress address, u8 value)
{
  switch (address)
  {
    case 0x02:
      Write_SHFT_AMNT(value);
      return;
    case 0x03:
      Write_SOUND1(value);
      return;
    case 0x04:
      Write_SHFT_DATA(value);
      return;
    case 0x05:
      Write_SOUND2(value);
      return;
    case 0x06:
      Write_WATCHDOG(value);
      return;
    default:
      Log_ErrorPrintf("Unhandled I/O port write 0x%04X <- 0x%02X", address, value);
      return;
  }
}

bool System::ReadROMToBuffer(const char* filename, void* buffer, u32 buffer_size)
{
  std::FILE* fp = std::fopen(filename, "rb");
  if (!fp)
  {
    Log_ErrorPrintf("Failed to open '%s'", filename);
    return false;
  }

  std::fseek(fp, 0, SEEK_END);
  u32 size = static_cast<u32>(std::ftell(fp));
  std::fseek(fp, 0, SEEK_SET);
  if (size != buffer_size)
  {
    Log_ErrorPrintf("Mismatched size for %s (got %u bytes, expected %u bytes)", filename, size, buffer_size);
    std::fclose(fp);
    return false;
  }

  if (std::fread(buffer, size, 1, fp) != 1)
  {
    Log_ErrorPrintf("Failed to read %u bytes from %s", size, filename);
    std::fclose(fp);
    return false;
  }

  std::fclose(fp);
  return true;
}

void System::InitColorMask()
{
  m_color_mask.resize(DISPLAY_WIDTH * DISPLAY_HEIGHT);

  u32* color_mask_ptr = m_color_mask.data();
  for (u32 row = 0; row < DISPLAY_HEIGHT; row++)
  {
    for (u32 col = 0; col < DISPLAY_WIDTH; col++)
    {
      u32 mask;

      if (col < 16)
      {
        if (row < 16)
          mask = m_display->PackRGB(255, 255, 255);
        else if (row < 118)
          mask = m_display->PackRGB(0, 255, 0);
        else
          mask = m_display->PackRGB(255, 255, 255);
      }
      else if (col < 72)
      {
        mask = m_display->PackRGB(0, 255, 0);
      }
      else if (col < 192)
      {
        mask = m_display->PackRGB(255, 0, 0);
      }
      else if (col < 224)
      {
        mask = m_display->PackRGB(0, 255, 0);
      }
      else
      {
        mask = m_display->PackRGB(255, 255, 255);
      }

      *color_mask_ptr++ = mask;
    }
  }
}

void System::RenderDisplay()
{
  constexpr u32 BYTE_COUNT = DISPLAY_WIDTH * DISPLAY_HEIGHT / 8;
  const u8* vram_ptr = &m_ram[0x400];
  const u32* color_mask_ptr = m_color_mask.data();
  u8* fb_ptr = m_display->GetFramebufferPointer();
  for (u32 row = 0; row < DISPLAY_HEIGHT; row++)
  {
    u8* fb_row_ptr = fb_ptr;
    for (u32 col = 0; col < (DISPLAY_WIDTH / 8); col++)
    {
      u8 in_byte = *vram_ptr++;

      for (u32 i = 0; i < 8; i++)
      {
        u32 rgb = (in_byte & u8(1)) ? UINT32_C(0xFFFFFFFF) : UINT32_C(0x00000000);
        rgb &= *color_mask_ptr++;
        std::memcpy(fb_row_ptr, &rgb, sizeof(rgb));
        fb_row_ptr += sizeof(rgb);
        in_byte >>= 1;
      }
    }

    fb_ptr += m_display->GetFramebufferPitch();
  }

  m_display->DisplayFramebuffer();
}

u8 System::Read_SHFT_IN()
{
  return Truncate8(m_shift_register_value >> (8 - m_shift_register_read_offset));
}

void System::Write_SHFT_AMNT(u8 val)
{
  m_shift_register_read_offset = (val & u8(0x07));
}

void System::Write_SHFT_DATA(u8 val)
{
  m_shift_register_value = (ZeroExtend16(val) << 8) | (m_shift_register_value >> 8);
}

void System::Write_SOUND1(u8 val) {}

void System::Write_SOUND2(u8 val) {}

void System::Write_WATCHDOG(u8 val) {}

} // namespace Invaders