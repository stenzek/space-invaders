#pragma once
#include "types.h"
#include "YBaseLib/String.h"

namespace i8080 {

class Bus;

class CPU
{
public:
  CPU(Bus* bus);
  ~CPU();

  Bus* GetBus() const { return m_bus; }

  const Registers& GetRegs() const { return m_regs; }
  Registers& GetRegs() { return m_regs; }

  void DisassembleInstruction(MemoryAddress address, String* dest) const;
  void GetStateString(String* dest) const;

  void Reset();
  void SingleStep();

  void ExecuteCycles(CycleCount cycles);

  void InterruptRequest(bool enable, u8 vector = 0);

private:
  u8 ReadMemoryByte(MemoryAddress address);
  u16 ReadMemoryWord(MemoryAddress address);
  void WriteMemoryByte(MemoryAddress address, u8 value);
  void WriteMemoryWord(MemoryAddress address, u16 value);
  void PushWord(u16 value);
  u16 PopWord();
  u8 ReadIOByte(u8 port);
  void WriteIOByte(u8 port, u8 value);
  void DispatchInterrupt();
  void Halt();

  void ExecuteInstruction();

  u8 ReadImmediateByte();
  u16 ReadImmediateWord();

  u8 op_inr(u8 rhs);
  u8 op_dcr(u8 rhs);
  u8 op_add(u8 lhs, u8 rhs);
  u8 op_adc(u8 lhs, u8 rhs);
  u8 op_sub(u8 lhs, u8 rhs);
  u8 op_sbb(u8 lhs, u8 rhs);
  u8 op_and(u8 lhs, u8 rhs);
  u8 op_xor(u8 lhs, u8 rhs);
  u8 op_or(u8 lhs, u8 rhs);
  u8 op_rlc(u8 rhs);
  u8 op_rrc(u8 rhs);
  u8 op_ral(u8 rhs);
  u8 op_rar(u8 rhs);
  u8 op_daa(u8 rhs);
  u16 op_dad(u16 lhs, u16 rhs);
  void op_jmp(u16 rhs);
  void op_call(u16 rhs);
  void op_ret();
  void op_xthl();

  Bus* m_bus;
  CycleCount m_cycles_left = 0;
  CycleCount m_pending_cycles = 0;
  Registers m_regs = {};
  bool m_halted = false;
  bool m_interrupt_enabled = true;
  bool m_interrupt_request = false;
  u8 m_interrupt_request_vector = 0;
};

extern bool TRACE_EXECUTION;

} // namespace i8080