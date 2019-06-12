#include "YBaseLib/Log.h"
#include "i8080/bus.h"
#include "i8080/cpu.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <vector>
Log_SetChannel(Test);

static std::optional<std::vector<u8>> ReadFile(const char* filename)
{
  std::FILE* fp = std::fopen(filename, "rb");
  if (!fp)
    return std::nullopt;

  std::fseek(fp, 0, SEEK_END);
  size_t len = std::ftell(fp);
  std::fseek(fp, 0, SEEK_SET);

  std::vector<u8> ret(len);
  if (std::fread(ret.data(), len, 1, fp) != 1)
  {
    std::fclose(fp);
    return std::nullopt;
  }

  return ret;
}

class TestBus : public i8080::Bus
{
public:
  TestBus() = default;
  ~TestBus() = default;

  const u8* GetRAM() const { return m_ram; }
  u8* GetRAM() { return m_ram; }

  void AddCycles(CycleCount cycles) override {}

  u8 ReadMemory(i8080::MemoryAddress address) override { return m_ram[address & 0xFFFF]; }
  void WriteMemory(i8080::MemoryAddress address, u8 value) override { m_ram[address & 0xFFFF] = value; }

  u8 ReadIO(i8080::MemoryAddress address) override { return 0xFF; }
  void WriteIO(i8080::MemoryAddress address, u8 value) override {}

  bool LoadFileToAddress(const char* filename, i8080::MemoryAddress base)
  {
    auto data = ReadFile(filename);
    if (!data)
      return false;

    const size_t copy_size = std::min(static_cast<size_t>(base) + data->size(), sizeof(m_ram)) - base;
    if (copy_size == 0)
      return false;

    Log_DevPrintf("Loading %zu bytes at %04X from %s", copy_size, base, filename);
    std::memcpy(m_ram + base, data->data(), copy_size);
    return true;
  }

private:
  u8 m_ram[0x10000] = {};
};

static String s_line_buffer;

static void AddLineCharacter(u8 ch)
{
  if (ch == '\r')
    return;
  else if (ch != '\n' && !std::isprint(ch))
    ch = '?';

  s_line_buffer.AppendCharacter(static_cast<char>(ch));
  if (s_line_buffer[s_line_buffer.GetLength() - 1] != '\n')
    return;

  s_line_buffer.Erase(-1);
  if (s_line_buffer.GetLength() > 0)
    Log_DevPrintf("CP/M: %s", s_line_buffer.GetCharArray());

  s_line_buffer.Clear();
}

static void HandleBDOSCommand(i8080::CPU* cpu)
{
  switch (cpu->GetRegs().c)
  {
    case 2: // print single character from reg e
    {
      AddLineCharacter(cpu->GetRegs().e);
    }
    break;

    case 9: // print string from memory at (de)
    {
      SmallString buf;
      i8080::MemoryAddress addr = cpu->GetRegs().de;
      u8 ch;
      while ((ch = cpu->GetBus()->ReadMemory(addr)) != '$')
      {
        if (ch == '\r')
        {
          addr++;
          continue;
        }

        if (ch != '\n' && !std::isprint(ch))
          ch = '?';

        AddLineCharacter(ch);
        addr++;
      }
    }
    break;

    default:
      Log_ErrorPrintf("Unknown bdos command 0x%02x", cpu->GetRegs().c);
      break;
  }
}

int main(int argc, char* argv[])
{
  Log::GetInstance().SetConsoleOutputParams(true);

  auto bus = std::make_unique<TestBus>();
  auto cpu = std::make_unique<i8080::CPU>(bus.get());

  // if (!bus->LoadFileToAddress("tests/CPUTEST.COM", 0x100))
  // if (!bus->LoadFileToAddress("tests/TST8080.COM", 0x100))
  // if (!bus->LoadFileToAddress("tests/8080PRE.COM", 0x100))
  if (!bus->LoadFileToAddress("tests/8080EXM.COM", 0x100))
    return -1;

  cpu->GetRegs().pc = 0x100;

  // inject RET into CALL 5 site for CP/M
  bus->GetRAM()[0x0005] = 0xC9;

  SmallString str;
  while (1)
  {
    if (cpu->GetRegs().pc == 0x0005)
      HandleBDOSCommand(cpu.get());
    else if (cpu->GetRegs().pc == 0x0000)
      break;

    cpu->SingleStep();
  }

  AddLineCharacter('\n');
  return 0;
}