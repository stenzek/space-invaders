#pragma once
#include "common/bitfield.h"
#include "common/types.h"

namespace i8080 {
using MemoryAddress = u16;

union Flags
{
  BitField<u8, bool, 7, 1> s; // sign flag
  BitField<u8, bool, 6, 1> z; // zero flag
  BitField<u8, bool, 4, 1> h; // auxiliary carry flag
  BitField<u8, bool, 2, 1> p; // parity flag
  BitField<u8, bool, 0, 1> c; // carry flag

  // only set mutable flags
  void Fixup() { bits = (bits & 0b11010101) | 0b00000010; }

  u8 bits;
};

// indices in instruction encoding
enum Reg8
{
  Reg8_A = 0b111,
  Reg8_B = 0b000,
  Reg8_C = 0b001,
  Reg8_D = 0b010,
  Reg8_E = 0b011,
  Reg8_H = 0b100,
  Reg8_L = 0b101,
  Reg8_Count,

  // Out-of-range value to indicate memory.
  Reg8_M,
  Reg8_Immediate
};

// indices in instruction encoding
enum Reg16
{
  Reg16_BC = 0b00,
  Reg16_DE = 0b01,
  Reg16_HL = 0b10,
  Reg16_SP = 0b11,
  Reg16_Count
};

union Registers
{
  struct
  {
    u8 c;
    u8 b;
    u8 e;
    u8 d;
    u8 l;
    u8 h;
    Flags f;
    u8 a;
  };

  struct
  {
    u16 bc;
    u16 de;
    u16 hl;
    u16 af;
    u16 sp;
    u16 pc;
  };

  u8 reg8_index[Reg8_Count];
  u16 reg16_index[Reg16_Count];

  inline u8 ReadReg8(Reg8 reg) const { return reg8_index[reg]; }
  inline void WriteReg8(Reg8 reg, u8 val) { reg8_index[reg] = val; }

  // reversed because little endian, remove swap for big endian
  inline u16 ReadReg16(Reg16 reg) const { return ((reg16_index[reg] << 8) | (reg16_index[reg] >> 8)); }
  inline void WriteReg16(Reg16 reg, u16 val) { reg16_index[reg] = ((val << 8) | (val >> 8)); }
};

} // namespace i8080