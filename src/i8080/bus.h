#include "types.h"

namespace i8080 {

class Bus
{
public:
  virtual void AddCycles(CycleCount cycles) = 0;

  virtual u8 ReadMemory(MemoryAddress address) = 0;
  virtual void WriteMemory(MemoryAddress address, u8 value) = 0;

  virtual u8 ReadIO(MemoryAddress address) = 0;
  virtual void WriteIO(MemoryAddress address, u8 value) = 0;
};

} // namespace i8080
