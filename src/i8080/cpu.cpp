#include "cpu.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "bus.h"
#include <array>
#include <cstdio>

namespace i8080 {

bool TRACE_EXECUTION = false;

CPU::CPU(Bus* bus) : m_bus(bus)
{
  Reset();
}

CPU::~CPU() = default;

void CPU::DisassembleInstruction(MemoryAddress address, String* dest) const
{
  static constexpr char nibbles[] = "0123456789ABCDEF";
  static constexpr std::array<const char*, 256> instruction_names = {
    {"nop",      "lxi b, ##", "stax b",   "inx b",    "inr b",      "dcr b",    "mvi b, #",  "rlc",        "nop",
     "dad b",    "ldax b",    "dcx b",    "inr c",    "dcr c",      "mvi c, #", "rrc",       "nop",        "lxi d, ##",
     "stax d",   "inx d",     "inr d",    "dcr d",    "mvi d, #",   "ral",      "nop",       "dad d",      "ldax d",
     "dcx d",    "inr e",     "dcr e",    "mvi e, #", "rar",        "nop",      "lxi h, ##", "shld      ", "inx h",
     "inr h",    "dcr h",     "mvi h, #", "daa",      "nop",        "dad h",    "lhld",      "dcx h",      "inr l",
     "dcr l",    "mvi l, #",  "cma",      "nop",      "lxi sp, ##", "sta $",    "inx sp",    "inr m",      "dcr m",
     "mvi m, #", "stc",       "nop",      "dad sp",   "lda $",      "dcx sp",   "inr a",     "dcr a",      "mvi a, #",
     "cmc",      "mov b, b",  "mov b, c", "mov b, d", "mov b, e",   "mov b, h", "mov b, l",  "mov b, m",   "mov b, a",
     "mov c, b", "mov c, c",  "mov c, d", "mov c, e", "mov c, h",   "mov c, l", "mov c, m",  "mov c, a",   "mov d, b",
     "mov d, c", "mov d, d",  "mov d, e", "mov d, h", "mov d, l",   "mov d, m", "mov d, a",  "mov e, b",   "mov e, c",
     "mov e, d", "mov e, e",  "mov e, h", "mov e, l", "mov e, m",   "mov e, a", "mov h, b",  "mov h, c",   "mov h, d",
     "mov h, e", "mov h, h",  "mov h, l", "mov h, m", "mov h, a",   "mov l, b", "mov l, c",  "mov l, d",   "mov l, e",
     "mov l, h", "mov l, l",  "mov l, m", "mov l, a", "mov m, b",   "mov m, c", "mov m, d",  "mov m, e",   "mov m, h",
     "mov m, l", "hlt",       "mov m, a", "mov a, b", "mov a, c",   "mov a, d", "mov a, e",  "mov a, h",   "mov a, l",
     "mov a, m", "mov a, a",  "add b",    "add c",    "add d",      "add e",    "add h",     "add l",      "add m",
     "add a",    "adc b",     "adc c",    "adc d",    "adc e",      "adc h",    "adc l",     "adc m",      "adc a",
     "sub b",    "sub c",     "sub d",    "sub e",    "sub h",      "sub l",    "sub m",     "sub a",      "sbc b",
     "sbc c",    "sbc d",     "sbc e",    "sbc h",    "sbc l",      "sbc m",    "sbc a",     "ana b",      "ana c",
     "ana d",    "ana e",     "ana h",    "ana l",    "ana m",      "ana a",    "xra b",     "xra c",      "xra d",
     "xra e",    "xra h",     "xra l",    "xra m",    "xra a",      "ora b",    "ora c",     "ora d",      "ora e",
     "ora h",    "ora l",     "ora m",    "ora a",    "cmp b",      "cmp c",    "cmp d",     "cmp e",      "cmp h",
     "cmp l",    "cmp m",     "cmp a",    "rnz",      "pop b",      "jnz $",    "jmp $",     "cnz $",      "push b",
     "adi #",    "rst 0",     "rz",       "ret",      "jz $",       "jmp $",    "cz $",      "call $",     "aci #",
     "rst 1",    "rnc",       "pop d",    "jnc $",    "out #",      "cnc $",    "push d",    "sui #",      "rst 2",
     "rc",       "ret",       "jc $",     "in #",     "cc $",       "call $",   "sbi #",     "rst 3",      "rpo",
     "pop h",    "jpo $",     "xthl",     "cpo $",    "push h",     "ani #",    "rst 4",     "rpe",        "pchl",
     "jo $",     "xchg",      "cpe $",    "call $",   "xri #",      "rst 5",    "rp",        "pop psw",    "jp $",
     "di",       "cp $",      "push psw", "ori #",    "rst 6",      "rm",       "sphl",      "jm $",       "ei",
     "cm $",     "call $",    "cpi #",    "rst 7"}};

  u16 current_address = address;
  const u8 opcode = m_bus->ReadMemory(current_address++);
  const char* instruction_template = instruction_names[opcode];
  const size_t instruction_template_length = std::strlen(instruction_template);

  TinyString hex;
  TinyString instruction;

  hex.AppendFormattedString("%02X", opcode);

  for (size_t i = 0; i < instruction_template_length;)
  {
    if (instruction_template[i] == '$' || (instruction_template[i] == '#' && instruction_template[i + 1] == '#'))
    {
      const u8 low = m_bus->ReadMemory(current_address++);
      const u8 high = m_bus->ReadMemory(current_address++);
      const u16 value = ZeroExtend16(low) | (ZeroExtend16(high) << 8);
      hex.AppendFormattedString(" %02X %02X", low, high);
      instruction.AppendFormattedString("%04xh", value);
      i += 2;
    }
    else if (instruction_template[i] == '#')
    {
      const u8 value = m_bus->ReadMemory(current_address++);
      hex.AppendFormattedString(" %02X", value);
      instruction.AppendFormattedString("%02xh", value);
      i++;
    }
    else
    {
      instruction.AppendCharacter(instruction_template[i]);
      i++;
    }
  }

  dest->Format("%04X: %-16s %s", address, hex.GetCharArray(), instruction.GetCharArray());
}

void CPU::GetStateString(String* dest) const
{
  SmallString disasm;
  DisassembleInstruction(m_regs.pc, &disasm);

  dest->Format("A: %02X F: %02X_%c%c%c%c%c B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X %s", m_regs.a,
               m_regs.f.bits, m_regs.f.s ? 'S' : 's', m_regs.f.z ? 'Z' : 'z', m_regs.f.h ? 'H' : 'h',
               m_regs.f.p ? 'P' : 'p', m_regs.f.c ? 'C' : 'c', m_regs.b, m_regs.c, m_regs.d, m_regs.e, m_regs.h,
               m_regs.l, m_regs.sp, disasm.GetCharArray());
}

void CPU::Reset()
{
  m_regs = {};
  m_cycles_left = 0;
  m_pending_cycles = 0;
  m_interrupt_enabled = true;
  m_interrupt_request = false;
  m_interrupt_request_vector = 0;
}

void CPU::SingleStep()
{
  DispatchInterrupt();
  if (m_halted)
    return;

  ExecuteInstruction();

  m_bus->AddCycles(m_pending_cycles);
  m_pending_cycles = 0;
}

void CPU::ExecuteCycles(CycleCount cycles)
{
  m_cycles_left += cycles;

  while (m_cycles_left > 0)
  {
    DispatchInterrupt();
    if (m_halted)
    {
      m_cycles_left = 0;
      break;
    }

    ExecuteInstruction();
  }

  m_bus->AddCycles(m_pending_cycles);
  m_pending_cycles = 0;
}

void CPU::InterruptRequest(bool enable, u8 vector)
{
  m_interrupt_request = enable;
  m_interrupt_request_vector = vector;
}

u8 CPU::ReadMemoryByte(MemoryAddress address)
{
  return m_bus->ReadMemory(address);
}

u16 CPU::ReadMemoryWord(MemoryAddress address)
{
  const u8 low = m_bus->ReadMemory(address);
  const u8 high = m_bus->ReadMemory(address + 1);
  return ZeroExtend16(low) | (ZeroExtend16(high) << 8);
}

void CPU::WriteMemoryByte(MemoryAddress address, u8 value)
{
  m_bus->WriteMemory(address, value);
}

void CPU::WriteMemoryWord(MemoryAddress address, u16 value)
{
  m_bus->WriteMemory(address, Truncate8(value));
  m_bus->WriteMemory(address + 1, Truncate8(value >> 8));
}

void CPU::PushWord(u16 value)
{
  m_bus->WriteMemory(--m_regs.sp, Truncate8(value >> 8));
  m_bus->WriteMemory(--m_regs.sp, Truncate8(value));
}

u16 CPU::PopWord()
{
  const u8 low = m_bus->ReadMemory(m_regs.sp++);
  const u8 high = m_bus->ReadMemory(m_regs.sp++);
  return ZeroExtend16(low) | (ZeroExtend16(high) << 8);
}

u8 CPU::ReadIOByte(u8 port)
{
  return m_bus->ReadIO(ZeroExtend16(port));
}

void CPU::WriteIOByte(u8 port, u8 value)
{
  return m_bus->WriteIO(ZeroExtend16(port), value);
}

void CPU::DispatchInterrupt()
{
  if (!(m_interrupt_request & m_interrupt_enabled))
    return;

  PushWord(m_regs.pc);
  m_regs.pc = ZeroExtend16(m_interrupt_request_vector) * u16(8);
  m_interrupt_enabled = false;
  m_interrupt_request = false;
  m_interrupt_request_vector = 0;
  m_halted = false;
}

void CPU::Halt()
{
  m_halted = true;
  if (m_cycles_left > 0)
  {
    m_pending_cycles += m_cycles_left;
    m_cycles_left = 0;
  }
}

void CPU::ExecuteInstruction()
{
  if (TRACE_EXECUTION)
  {
    SmallString str;
    GetStateString(&str);
    std::puts(str);
  }

#define CYCLES(n) do { m_cycles_left -= (n); m_pending_cycles += (n); } while (0)
  u16 tgt;

  const u8 opcode = ReadImmediateByte();
  switch (opcode & static_cast<u8>(0xFF))
  {
      // clang-format off
    case 0x00: CYCLES(4); break;                                                                                      // nop
    case 0x08: CYCLES(4); break;                                                                                      // nop
    case 0x10: CYCLES(4); break;                                                                                      // nop
    case 0x18: CYCLES(4); break;                                                                                      // nop
    case 0x20: CYCLES(4); break;                                                                                      // nop
    case 0x28: CYCLES(4); break;                                                                                      // nop
    case 0x30: CYCLES(4); break;                                                                                      // nop
    case 0x38: CYCLES(4); break;                                                                                      // nop
    case 0x01: CYCLES(10); m_regs.bc = ReadImmediateWord(); break;                                                    // lxi b, d16
    case 0x11: CYCLES(10); m_regs.de = ReadImmediateWord(); break;                                                    // lxi d, d16
    case 0x21: CYCLES(10); m_regs.hl = ReadImmediateWord(); break;                                                    // lxi h, d16
    case 0x31: CYCLES(10); m_regs.sp = ReadImmediateWord(); break;                                                    // lxi sp, d16
    case 0x0A: CYCLES(7); m_regs.a = ReadMemoryByte(m_regs.bc); break;                                                // ldax b
    case 0x1A: CYCLES(7); m_regs.a = ReadMemoryByte(m_regs.de); break;                                                // ldax d
    case 0x02: CYCLES(7); WriteMemoryByte(m_regs.bc, m_regs.a); break;                                                // stax b
    case 0x12: CYCLES(7); WriteMemoryByte(m_regs.de, m_regs.a); break;                                                // stax d
    case 0x3A: CYCLES(13); m_regs.a = ReadMemoryByte(ReadImmediateWord()); break;                                     // lda a16
    case 0x32: CYCLES(13); WriteMemoryByte(ReadImmediateWord(), m_regs.a); break;                                     // sta a16
    case 0x2A: CYCLES(16); m_regs.hl = ReadMemoryWord(ReadImmediateWord()); break;                                    // lhld
    case 0x22: CYCLES(16); WriteMemoryWord(ReadImmediateWord(), m_regs.hl); break;                                    // shld      
    case 0x03: CYCLES(5); m_regs.bc++; break;                                                                         // inx b
    case 0x13: CYCLES(5); m_regs.de++; break;                                                                         // inx d
    case 0x23: CYCLES(5); m_regs.hl++; break;                                                                         // inx h
    case 0x33: CYCLES(5); m_regs.sp++; break;                                                                         // inx sp
    case 0x0B: CYCLES(5); m_regs.bc--; break;                                                                         // dcx b
    case 0x1B: CYCLES(5); m_regs.de--; break;                                                                         // dcx d
    case 0x2B: CYCLES(5); m_regs.hl--; break;                                                                         // dcx h
    case 0x3B: CYCLES(5); m_regs.sp--; break;                                                                         // dcx sp
    case 0x09: CYCLES(10); m_regs.hl = op_dad(m_regs.hl, m_regs.bc); break;                                           // dad b
    case 0x19: CYCLES(10); m_regs.hl = op_dad(m_regs.hl, m_regs.de); break;                                           // dad d
    case 0x29: CYCLES(10); m_regs.hl = op_dad(m_regs.hl, m_regs.hl); break;                                           // dad h
    case 0x39: CYCLES(10); m_regs.hl = op_dad(m_regs.hl, m_regs.sp); break;                                           // dad sp
    case 0x3C: CYCLES(5); m_regs.a = op_inr(m_regs.a); break;                                                         // inr a
    case 0x04: CYCLES(5); m_regs.b = op_inr(m_regs.b); break;                                                         // inr b
    case 0x0C: CYCLES(5); m_regs.c = op_inr(m_regs.c); break;                                                         // inr c
    case 0x14: CYCLES(5); m_regs.d = op_inr(m_regs.d); break;                                                         // inr d
    case 0x1C: CYCLES(5); m_regs.e = op_inr(m_regs.e); break;                                                         // inr e
    case 0x24: CYCLES(5); m_regs.h = op_inr(m_regs.h); break;                                                         // inr h
    case 0x2C: CYCLES(5); m_regs.l = op_inr(m_regs.l); break;                                                         // inr l
    case 0x34: CYCLES(10); WriteMemoryByte(m_regs.hl, op_inr(ReadMemoryByte(m_regs.hl))); break;                      // inr m
    case 0x3D: CYCLES(5); m_regs.a = op_dcr(m_regs.a); break;                                                         // dcr a
    case 0x05: CYCLES(5); m_regs.b = op_dcr(m_regs.b); break;                                                         // dcr b
    case 0x0D: CYCLES(5); m_regs.c = op_dcr(m_regs.c); break;                                                         // dcr c
    case 0x15: CYCLES(5); m_regs.d = op_dcr(m_regs.d); break;                                                         // dcr d
    case 0x1D: CYCLES(5); m_regs.e = op_dcr(m_regs.e); break;                                                         // dcr e
    case 0x25: CYCLES(5); m_regs.h = op_dcr(m_regs.h); break;                                                         // dcr h
    case 0x2D: CYCLES(5); m_regs.l = op_dcr(m_regs.l); break;                                                         // dcr l
    case 0x35: CYCLES(10); WriteMemoryByte(m_regs.hl, op_dcr(ReadMemoryByte(m_regs.hl))); break;                      // dcr m
    case 0x3E: CYCLES(7); m_regs.a = ReadImmediateByte(); break;                                                      // mvi a, d8
    case 0x06: CYCLES(7); m_regs.b = ReadImmediateByte(); break;                                                      // mvi b, d8
    case 0x0E: CYCLES(7); m_regs.c = ReadImmediateByte(); break;                                                      // mvi c, d8
    case 0x16: CYCLES(7); m_regs.d = ReadImmediateByte(); break;                                                      // mvi d, d8
    case 0x1E: CYCLES(7); m_regs.e = ReadImmediateByte(); break;                                                      // mvi e, d8
    case 0x26: CYCLES(7); m_regs.h = ReadImmediateByte(); break;                                                      // mvi h, d8
    case 0x2E: CYCLES(7); m_regs.l = ReadImmediateByte(); break;                                                      // mvi l, d8
    case 0x36: CYCLES(10); WriteMemoryByte(m_regs.hl, ReadImmediateByte()); break;                                    // mvi m, d8
    case 0x07: CYCLES(4); m_regs.a = op_rlc(m_regs.a); break;                                                         // rlc
    case 0x17: CYCLES(4); m_regs.a = op_ral(m_regs.a); break;                                                         // ral
    case 0x0F: CYCLES(4); m_regs.a = op_rrc(m_regs.a); break;                                                         // rrc
    case 0x1F: CYCLES(4); m_regs.a = op_rar(m_regs.a); break;                                                         // rar
    case 0x27: CYCLES(4); m_regs.a = op_daa(m_regs.a); break;                                                         // daa
    case 0x2F: CYCLES(4); m_regs.a = ~m_regs.a; break;                                                                // cma
    case 0x37: CYCLES(4); m_regs.f.c = true; break;                                                                   // stc
    case 0x3F: CYCLES(4); m_regs.f.c = !m_regs.f.c; break;                                                            // cmc
    case 0x76: CYCLES(7); Halt();                                                                                     // hlt
    case 0x47: CYCLES(5); m_regs.b = m_regs.a; break;                                                                 // mov b, a
    case 0x40: CYCLES(5); m_regs.b = m_regs.b; break;                                                                 // mov b, b
    case 0x41: CYCLES(5); m_regs.b = m_regs.c; break;                                                                 // mov b, c
    case 0x42: CYCLES(5); m_regs.b = m_regs.d; break;                                                                 // mov b, d
    case 0x43: CYCLES(5); m_regs.b = m_regs.e; break;                                                                 // mov b, e
    case 0x44: CYCLES(5); m_regs.b = m_regs.h; break;                                                                 // mov b, h
    case 0x45: CYCLES(5); m_regs.b = m_regs.l; break;                                                                 // mov b, l
    case 0x46: CYCLES(7); m_regs.b = ReadMemoryByte(m_regs.hl); break;                                                // mov b, m
    case 0x4F: CYCLES(5); m_regs.c = m_regs.a; break;                                                                 // mov c, a
    case 0x48: CYCLES(5); m_regs.c = m_regs.b; break;                                                                 // mov c, b
    case 0x49: CYCLES(5); m_regs.c = m_regs.c; break;                                                                 // mov c, c
    case 0x4A: CYCLES(5); m_regs.c = m_regs.d; break;                                                                 // mov c, d
    case 0x4B: CYCLES(5); m_regs.c = m_regs.e; break;                                                                 // mov c, e
    case 0x4C: CYCLES(5); m_regs.c = m_regs.h; break;                                                                 // mov c, h
    case 0x4D: CYCLES(5); m_regs.c = m_regs.l; break;                                                                 // mov c, l
    case 0x4E: CYCLES(7); m_regs.c = ReadMemoryByte(m_regs.hl); break;                                                // mov c, m
    case 0x57: CYCLES(5); m_regs.d = m_regs.a; break;                                                                 // mov d, a
    case 0x50: CYCLES(5); m_regs.d = m_regs.b; break;                                                                 // mov d, b
    case 0x51: CYCLES(5); m_regs.d = m_regs.c; break;                                                                 // mov d, c
    case 0x52: CYCLES(5); m_regs.d = m_regs.d; break;                                                                 // mov d, d
    case 0x53: CYCLES(5); m_regs.d = m_regs.e; break;                                                                 // mov d, e
    case 0x54: CYCLES(5); m_regs.d = m_regs.h; break;                                                                 // mov d, h
    case 0x55: CYCLES(5); m_regs.d = m_regs.l; break;                                                                 // mov d, l
    case 0x56: CYCLES(7); m_regs.d = ReadMemoryByte(m_regs.hl); break;                                                // mov d, m
    case 0x5F: CYCLES(5); m_regs.e = m_regs.a; break;                                                                 // mov e, a
    case 0x58: CYCLES(5); m_regs.e = m_regs.b; break;                                                                 // mov e, b
    case 0x59: CYCLES(5); m_regs.e = m_regs.c; break;                                                                 // mov e, c
    case 0x5A: CYCLES(5); m_regs.e = m_regs.d; break;                                                                 // mov e, d
    case 0x5B: CYCLES(5); m_regs.e = m_regs.e; break;                                                                 // mov e, e
    case 0x5C: CYCLES(5); m_regs.e = m_regs.h; break;                                                                 // mov e, h
    case 0x5D: CYCLES(5); m_regs.e = m_regs.l; break;                                                                 // mov e, l
    case 0x5E: CYCLES(7); m_regs.e = ReadMemoryByte(m_regs.hl); break;                                                // mov e, m
    case 0x67: CYCLES(5); m_regs.h = m_regs.a; break;                                                                 // mov h, a
    case 0x60: CYCLES(5); m_regs.h = m_regs.b; break;                                                                 // mov h, b
    case 0x61: CYCLES(5); m_regs.h = m_regs.c; break;                                                                 // mov h, c
    case 0x62: CYCLES(5); m_regs.h = m_regs.d; break;                                                                 // mov h, d
    case 0x63: CYCLES(5); m_regs.h = m_regs.e; break;                                                                 // mov h, e
    case 0x64: CYCLES(5); m_regs.h = m_regs.h; break;                                                                 // mov h, h
    case 0x65: CYCLES(5); m_regs.h = m_regs.l; break;                                                                 // mov h, l
    case 0x66: CYCLES(7); m_regs.h = ReadMemoryByte(m_regs.hl); break;                                                // mov h, m
    case 0x6F: CYCLES(5); m_regs.l = m_regs.a; break;                                                                 // mov l, a
    case 0x68: CYCLES(5); m_regs.l = m_regs.b; break;                                                                 // mov l, b
    case 0x69: CYCLES(5); m_regs.l = m_regs.c; break;                                                                 // mov l, c
    case 0x6A: CYCLES(5); m_regs.l = m_regs.d; break;                                                                 // mov l, d
    case 0x6B: CYCLES(5); m_regs.l = m_regs.e; break;                                                                 // mov l, e
    case 0x6C: CYCLES(5); m_regs.l = m_regs.h; break;                                                                 // mov l, h
    case 0x6D: CYCLES(5); m_regs.l = m_regs.l; break;                                                                 // mov l, l
    case 0x6E: CYCLES(7); m_regs.l = ReadMemoryByte(m_regs.hl); break;                                                // mov l, m
    case 0x7F: CYCLES(5); m_regs.a = m_regs.a; break;                                                                 // mov a, a
    case 0x78: CYCLES(5); m_regs.a = m_regs.b; break;                                                                 // mov a, b
    case 0x79: CYCLES(5); m_regs.a = m_regs.c; break;                                                                 // mov a, c
    case 0x7A: CYCLES(5); m_regs.a = m_regs.d; break;                                                                 // mov a, d
    case 0x7B: CYCLES(5); m_regs.a = m_regs.e; break;                                                                 // mov a, e
    case 0x7C: CYCLES(5); m_regs.a = m_regs.h; break;                                                                 // mov a, h
    case 0x7D: CYCLES(5); m_regs.a = m_regs.l; break;                                                                 // mov a, l
    case 0x7E: CYCLES(7); m_regs.a = ReadMemoryByte(m_regs.hl); break;                                                // mov a, m
    case 0x77: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.a); break;                                                // mov m, a
    case 0x70: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.b); break;                                                // mov m, b
    case 0x71: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.c); break;                                                // mov m, c
    case 0x72: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.d); break;                                                // mov m, d
    case 0x73: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.e); break;                                                // mov m, e
    case 0x74: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.h); break;                                                // mov m, h
    case 0x75: CYCLES(7); WriteMemoryByte(m_regs.hl, m_regs.l); break;                                                // mov m, l
    case 0x87: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.a); break;                                               // add a
    case 0x80: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.b); break;                                               // add b
    case 0x81: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.c); break;                                               // add c
    case 0x82: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.d); break;                                               // add d
    case 0x83: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.e); break;                                               // add e
    case 0x84: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.h); break;                                               // add h
    case 0x85: CYCLES(4); m_regs.a = op_add(m_regs.a, m_regs.l); break;                                               // add l
    case 0x86: CYCLES(4); m_regs.a = op_add(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // add m
    case 0xC6: CYCLES(7); m_regs.a = op_add(m_regs.a, ReadImmediateByte()); break;                                    // adi d8
    case 0x8F: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.a); break;                                               // adc a
    case 0x88: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.b); break;                                               // adc b
    case 0x89: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.c); break;                                               // adc c
    case 0x8A: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.d); break;                                               // adc d
    case 0x8B: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.e); break;                                               // adc e
    case 0x8C: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.h); break;                                               // adc h
    case 0x8D: CYCLES(4); m_regs.a = op_adc(m_regs.a, m_regs.l); break;                                               // adc l
    case 0x8E: CYCLES(4); m_regs.a = op_adc(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // adc m
    case 0xCE: CYCLES(7); m_regs.a = op_adc(m_regs.a, ReadImmediateByte()); break;                                    // aci d8
    case 0x97: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.a); break;                                               // sub a
    case 0x90: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.b); break;                                               // sub b
    case 0x91: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.c); break;                                               // sub c
    case 0x92: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.d); break;                                               // sub d
    case 0x93: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.e); break;                                               // sub e
    case 0x94: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.h); break;                                               // sub h
    case 0x95: CYCLES(4); m_regs.a = op_sub(m_regs.a, m_regs.l); break;                                               // sub l
    case 0x96: CYCLES(4); m_regs.a = op_sub(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // sub m
    case 0xD6: CYCLES(7); m_regs.a = op_sub(m_regs.a, ReadImmediateByte()); break;                                    // sui d8
    case 0x9F: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.a); break;                                               // sbc a
    case 0x98: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.b); break;                                               // sbc b
    case 0x99: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.c); break;                                               // sbc c
    case 0x9A: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.d); break;                                               // sbc d
    case 0x9B: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.e); break;                                               // sbc e
    case 0x9C: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.h); break;                                               // sbc h
    case 0x9D: CYCLES(4); m_regs.a = op_sbb(m_regs.a, m_regs.l); break;                                               // sbc l
    case 0x9E: CYCLES(4); m_regs.a = op_sbb(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // sbc m
    case 0xDE: CYCLES(7); m_regs.a = op_sbb(m_regs.a, ReadImmediateByte()); break;                                    // sbi d8
    case 0xA7: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.a); break;                                               // ana a
    case 0xA0: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.b); break;                                               // ana b
    case 0xA1: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.c); break;                                               // ana c
    case 0xA2: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.d); break;                                               // ana d
    case 0xA3: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.e); break;                                               // ana e
    case 0xA4: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.h); break;                                               // ana h
    case 0xA5: CYCLES(4); m_regs.a = op_and(m_regs.a, m_regs.l); break;                                               // ana l
    case 0xA6: CYCLES(4); m_regs.a = op_and(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // ana m
    case 0xE6: CYCLES(7); m_regs.a = op_and(m_regs.a, ReadImmediateByte()); break;                                    // ani d8
    case 0xAF: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.a); break;                                               // xra a
    case 0xA8: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.b); break;                                               // xra b
    case 0xA9: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.c); break;                                               // xra c
    case 0xAA: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.d); break;                                               // xra d
    case 0xAB: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.e); break;                                               // xra e
    case 0xAC: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.h); break;                                               // xra h
    case 0xAD: CYCLES(4); m_regs.a = op_xor(m_regs.a, m_regs.l); break;                                               // xra l
    case 0xAE: CYCLES(4); m_regs.a = op_xor(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                              // xra m
    case 0xEE: CYCLES(7); m_regs.a = op_xor(m_regs.a, ReadImmediateByte()); break;                                    // xri d8
    case 0xB7: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.a); break;                                                // ora a
    case 0xB0: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.b); break;                                                // ora b
    case 0xB1: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.c); break;                                                // ora c
    case 0xB2: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.d); break;                                                // ora d
    case 0xB3: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.e); break;                                                // ora e
    case 0xB4: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.h); break;                                                // ora h
    case 0xB5: CYCLES(4); m_regs.a = op_or(m_regs.a, m_regs.l); break;                                                // ora l
    case 0xB6: CYCLES(4); m_regs.a = op_or(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                               // ora m
    case 0xF6: CYCLES(7); m_regs.a = op_or(m_regs.a, ReadImmediateByte()); break;                                     // ori d8
    case 0xBF: CYCLES(4); op_sub(m_regs.a, m_regs.a); break;                                                          // cmp a
    case 0xB8: CYCLES(4); op_sub(m_regs.a, m_regs.b); break;                                                          // cmp b
    case 0xB9: CYCLES(4); op_sub(m_regs.a, m_regs.c); break;                                                          // cmp c
    case 0xBA: CYCLES(4); op_sub(m_regs.a, m_regs.d); break;                                                          // cmp d
    case 0xBB: CYCLES(4); op_sub(m_regs.a, m_regs.e); break;                                                          // cmp e
    case 0xBC: CYCLES(4); op_sub(m_regs.a, m_regs.h); break;                                                          // cmp h
    case 0xBD: CYCLES(4); op_sub(m_regs.a, m_regs.l); break;                                                          // cmp l
    case 0xBE: CYCLES(4); op_sub(m_regs.a, ReadMemoryByte(m_regs.hl)); break;                                         // cmp m
    case 0xFE: CYCLES(7); op_sub(m_regs.a, ReadImmediateByte()); break;                                               // cpi d8
    case 0xC3: CYCLES(10); tgt = ReadImmediateWord(); op_jmp(tgt); break;                                             // jmp a16
    case 0xCB: CYCLES(10); tgt = ReadImmediateWord(); op_jmp(tgt); break;                                             // jmp a16
    case 0xC2: CYCLES(10); tgt = ReadImmediateWord(); if (!m_regs.f.z) { op_jmp(tgt); } break;                        // jnz a16
    case 0xD2: CYCLES(10); tgt = ReadImmediateWord(); if (!m_regs.f.c) { op_jmp(tgt); } break;                        // jnc a16
    case 0xE2: CYCLES(10); tgt = ReadImmediateWord(); if (!m_regs.f.p) { op_jmp(tgt); } break;                        // jpo a16
    case 0xF2: CYCLES(10); tgt = ReadImmediateWord(); if (!m_regs.f.s) { op_jmp(tgt); } break;                        // jp a16
    case 0xCA: CYCLES(10); tgt = ReadImmediateWord(); if (m_regs.f.z) { op_jmp(tgt); } break;                         // jz a16
    case 0xDA: CYCLES(10); tgt = ReadImmediateWord(); if (m_regs.f.c) { op_jmp(tgt); } break;                         // jc a16
    case 0xEA: CYCLES(10); tgt = ReadImmediateWord(); if (m_regs.f.p) { op_jmp(tgt); } break;                         // jo a16
    case 0xFA: CYCLES(10); tgt = ReadImmediateWord(); if (m_regs.f.s) { op_jmp(tgt); } break;                         // jm a16
    case 0xCD: CYCLES(17); tgt = ReadImmediateWord(); op_call(tgt); break;                                            // call a16
    case 0xDD: CYCLES(17); tgt = ReadImmediateWord(); op_call(tgt); break;                                            // call a16
    case 0xED: CYCLES(17); tgt = ReadImmediateWord(); op_call(tgt); break;                                            // call a16
    case 0xFD: CYCLES(17); tgt = ReadImmediateWord(); op_call(tgt); break;                                            // call a16
    case 0xC4: tgt = ReadImmediateWord(); if (!m_regs.f.z) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;  // cnz a16
    case 0xD4: tgt = ReadImmediateWord(); if (!m_regs.f.c) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;  // cnc a16
    case 0xE4: tgt = ReadImmediateWord(); if (!m_regs.f.p) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;  // cpo a16
    case 0xF4: tgt = ReadImmediateWord(); if (!m_regs.f.s) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;  // cp a16
    case 0xCC: tgt = ReadImmediateWord(); if (m_regs.f.z) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;   // cz a16
    case 0xDC: tgt = ReadImmediateWord(); if (m_regs.f.c) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;   // cc a16
    case 0xEC: tgt = ReadImmediateWord(); if (m_regs.f.p) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;   // cpe a16
    case 0xFC: tgt = ReadImmediateWord(); if (m_regs.f.s) { CYCLES(17); op_call(tgt); } else { CYCLES(11); } break;   // cm a16
    case 0xC9: CYCLES(10); op_ret(); break;                                                                           // ret
    case 0xD9: CYCLES(10); op_ret(); break;                                                                           // ret
    case 0xC0: if (!m_regs.f.z) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                  // rnz
    case 0xD0: if (!m_regs.f.c) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                  // rnc
    case 0xE0: if (!m_regs.f.p) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                  // rpo
    case 0xF0: if (!m_regs.f.s) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                  // rp
    case 0xC8: if (m_regs.f.z) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                   // rz
    case 0xD8: if (m_regs.f.c) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                   // rc
    case 0xE8: if (m_regs.f.p) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                   // rpe
    case 0xF8: if (m_regs.f.s) { CYCLES(11); op_ret(); } else { CYCLES(5); } break;                                   // rm
    case 0xC7: CYCLES(11); op_call(0x0000); break;                                                                    // rst 0
    case 0xCF: CYCLES(11); op_call(0x0008); break;                                                                    // rst 1
    case 0xD7: CYCLES(11); op_call(0x0010); break;                                                                    // rst 2
    case 0xDF: CYCLES(11); op_call(0x0018); break;                                                                    // rst 3
    case 0xE7: CYCLES(11); op_call(0x0020); break;                                                                    // rst 4
    case 0xEF: CYCLES(11); op_call(0x0028); break;                                                                    // rst 5
    case 0xF7: CYCLES(11); op_call(0x0030); break;                                                                    // rst 6
    case 0xFF: CYCLES(11); op_call(0x0038); break;                                                                    // rst 7
    case 0xF5: CYCLES(11); PushWord(m_regs.af); break;                                                                // push psw
    case 0xC5: CYCLES(11); PushWord(m_regs.bc); break;                                                                // push b
    case 0xD5: CYCLES(11); PushWord(m_regs.de); break;                                                                // push d
    case 0xE5: CYCLES(11); PushWord(m_regs.hl); break;                                                                // push h
    case 0xC1: CYCLES(10); m_regs.bc = PopWord(); break;                                                              // pop b
    case 0xD1: CYCLES(10); m_regs.de = PopWord(); break;                                                              // pop d
    case 0xE1: CYCLES(10); m_regs.hl = PopWord(); break;                                                              // pop h
    case 0xF1: CYCLES(10); m_regs.af = PopWord(); m_regs.f.Fixup(); break;                                            // pop psw
    case 0xEB: CYCLES(5); std::swap(m_regs.de, m_regs.hl); break;                                                     // xchg
    case 0xE3: CYCLES(18); op_xthl(); break;                                                                          // xthl
    case 0xE9: CYCLES(5); m_regs.pc = m_regs.hl; break;                                                               // pchl
    case 0xF9: CYCLES(5); m_regs.sp = m_regs.hl; break;                                                               // sphl
    case 0xF3: CYCLES(4); m_interrupt_enabled = false; break;                                                         // di
    case 0xFB: CYCLES(4); m_interrupt_enabled = true; break;                                                          // ei
    case 0xDB: CYCLES(10); m_regs.a = ReadIOByte(ReadImmediateByte()); break;                                         // in d8
    case 0xD3: CYCLES(10); WriteIOByte(ReadImmediateByte(), m_regs.a); break;                                         // out d8
      // clang-format on
  }
}

u8 CPU::ReadImmediateByte()
{
  return ReadMemoryByte(m_regs.pc++);
}

u16 CPU::ReadImmediateWord()
{
  const u16 ret = ReadMemoryWord(m_regs.pc);
  m_regs.pc += 2;
  return ret;
}

static bool ParityFlag(u8 val)
{
  return ConvertToBoolUnchecked((Y_popcnt(val) & u8(1)) ^ u8(1));
}

u8 CPU::op_inr(u8 rhs)
{
  const u8 res = rhs + 1;
  m_regs.f.z = res == 0;
  m_regs.f.s = ConvertToBoolUnchecked(res >> 7);
  m_regs.f.p = ParityFlag(res);
  m_regs.f.h = (res & u8(0xF)) == 0;
  return res;
}

u8 CPU::op_dcr(u8 rhs)
{
  const u8 res = rhs - 1;
  m_regs.f.s = ConvertToBoolUnchecked(res >> 7);
  m_regs.f.h = !((res & u8(0xF)) == u8(0xF));
  m_regs.f.p = ParityFlag(res);
  m_regs.f.z = res == 0;
  return res;
}

u8 CPU::op_add(u8 lhs, u8 rhs)
{
  const u16 res16 = ZeroExtend16(lhs) + ZeroExtend16(rhs);
  const u8 res8 = Truncate8(res16);

  m_regs.f.s = ConvertToBoolUnchecked(res8 >> 7);
  m_regs.f.h = ConvertToBoolUnchecked(((lhs ^ rhs ^ res8) >> 4) & u8(1));
  m_regs.f.c = ConvertToBoolUnchecked(res16 >> 8);
  m_regs.f.p = ParityFlag(res8);
  m_regs.f.z = res8 == 0;

  return res8;
}

u8 CPU::op_adc(u8 lhs, u8 rhs)
{
  const u16 res16 = ZeroExtend16(lhs) + ZeroExtend16(rhs) + BoolToUInt16(m_regs.f.c);
  const u8 res8 = Truncate8(res16);

  m_regs.f.s = ConvertToBoolUnchecked(res8 >> 7);
  m_regs.f.h = ConvertToBoolUnchecked(((lhs ^ rhs ^ res8) >> 4) & u8(1));
  m_regs.f.c = ConvertToBoolUnchecked(res16 >> 8);
  m_regs.f.p = ParityFlag(res8);
  m_regs.f.z = res8 == 0;

  return res8;
}

u8 CPU::op_sub(u8 lhs, u8 rhs)
{
  const u16 res16 = ZeroExtend16(lhs) - ZeroExtend16(rhs);
  const u8 res8 = Truncate8(res16);

  m_regs.f.s = ConvertToBoolUnchecked(res8 >> 7);
  m_regs.f.h = ConvertToBoolUnchecked((~(lhs ^ rhs ^ res8) >> 4) & u8(1));
  m_regs.f.c = ConvertToBoolUnchecked(res16 >> 8);
  m_regs.f.p = ParityFlag(res8);
  m_regs.f.z = res8 == 0;

  return res8;
}

u8 CPU::op_sbb(u8 lhs, u8 rhs)
{
  const u16 res16 = ZeroExtend16(lhs) - ZeroExtend16(rhs) - BoolToUInt16(m_regs.f.c);
  const u8 res8 = Truncate8(res16);

  m_regs.f.s = ConvertToBoolUnchecked(res8 >> 7);
  m_regs.f.h = ConvertToBoolUnchecked((~(lhs ^ rhs ^ res8) >> 4) & u8(1));
  m_regs.f.c = ConvertToBoolUnchecked(res16 >> 8);
  m_regs.f.p = ParityFlag(res8);
  m_regs.f.z = res8 == 0;

  return res8;
}

u8 CPU::op_and(u8 lhs, u8 rhs)
{
  const u8 res = lhs & rhs;

  m_regs.f.s = ConvertToBoolUnchecked(res >> 7);
  m_regs.f.p = ParityFlag(res);
  m_regs.f.z = res == 0;
  m_regs.f.c = false;
  m_regs.f.h = ConvertToBoolUnchecked(((lhs | rhs) >> 3) & u8(1));

  return res;
}

u8 CPU::op_xor(u8 lhs, u8 rhs)
{
  const u8 res = lhs ^ rhs;

  m_regs.f.s = ConvertToBoolUnchecked(res >> 7);
  m_regs.f.p = ParityFlag(res);
  m_regs.f.z = res == 0;
  m_regs.f.c = 0;
  m_regs.f.h = 0;

  return res;
}

u8 CPU::op_or(u8 lhs, u8 rhs)
{
  const u8 res = lhs | rhs;

  m_regs.f.s = ConvertToBoolUnchecked(res >> 7);
  m_regs.f.p = ParityFlag(res);
  m_regs.f.z = res == 0;
  m_regs.f.c = 0;
  m_regs.f.h = 0;

  return res;
}

u8 CPU::op_rlc(u8 rhs)
{
  const u8 res = (rhs << 1) | (rhs >> 7);
  m_regs.f.c = ConvertToBoolUnchecked(rhs >> 7);
  return res;
}

u8 CPU::op_rrc(u8 rhs)
{
  const u8 res = (rhs >> 1) | (rhs << 7);
  m_regs.f.c = ConvertToBoolUnchecked(rhs & u8(1));
  return res;
}

u8 CPU::op_ral(u8 rhs)
{
  const u8 res = (rhs << 1) | BoolToUInt8(m_regs.f.c);
  m_regs.f.c = ConvertToBoolUnchecked(rhs >> 7);
  return res;
}

u8 CPU::op_rar(u8 rhs)
{
  const u8 res = (rhs >> 1) | (BoolToUInt8(m_regs.f.c) << 7);
  m_regs.f.c = ConvertToBoolUnchecked(rhs & u8(1));
  return res;
}

u8 CPU::op_daa(u8 rhs)
{
  u8 add = 0;
  if ((rhs & u8(0xF)) > 0x9 || m_regs.f.h)
    add = 0x06;

  if (rhs > 0x99 || m_regs.f.c)
  {
    add += 0x60;
    m_regs.f.c = true;
  }

  const u16 res16 = ZeroExtend16(rhs) + ZeroExtend16(add);
  const u8 res8 = Truncate8(res16);
  m_regs.f.s = ConvertToBoolUnchecked(res8 >> 7);
  m_regs.f.h = ConvertToBoolUnchecked(((rhs ^ add ^ res8) >> 4) & u8(1));
  m_regs.f.p = ParityFlag(res8);
  m_regs.f.z = res8 == 0;

  return Truncate8(res8);
}

u16 CPU::op_dad(u16 lhs, u16 rhs)
{
  const u32 res32 = ZeroExtend32(lhs) + ZeroExtend32(rhs);
  m_regs.f.c = ConvertToBoolUnchecked(res32 >> 16);
  return Truncate16(res32);
}

void CPU::op_jmp(u16 rhs)
{
  m_regs.pc = rhs;
}

void CPU::op_call(u16 rhs)
{
  PushWord(m_regs.pc);
  op_jmp(rhs);
}

void CPU::op_ret()
{
  m_regs.pc = PopWord();
}

void CPU::op_xthl()
{
  const u16 hl = m_regs.hl;
  m_regs.hl = ReadMemoryWord(m_regs.sp);
  WriteMemoryWord(m_regs.sp, hl);
}

} // namespace i8080