#include "simple_audio.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
Log_SetChannel(SimpleAudio);

SimpleAudio::SimpleAudio() = default;

SimpleAudio::~SimpleAudio() = default;

bool SimpleAudio::Reconfigure(u32 output_sample_rate /*= DefaultOutputSampleRate*/, u32 channels /*= 1*/,
                        u32 buffer_size /*= DefaultBufferSize*/, u32 buffer_count /*= DefaultBufferCount*/)
{
  if (IsDeviceOpen())
    CloseDevice();

  m_output_sample_rate = output_sample_rate;
  m_channels = channels;
  m_buffer_size = buffer_size;
  AllocateBuffers(buffer_count);
  m_output_paused = true;

  if (!OpenDevice())
  {
    EmptyBuffers();
    m_buffers.clear();
    m_buffer_size = 0;
    m_output_sample_rate = 0;
    m_channels = 0;
    return false;
  }

  return true;
}

void SimpleAudio::PauseOutput(bool paused)
{
  if (m_output_paused == paused)
    return;

  PauseDevice(paused);
  m_output_paused = paused;

  // Empty buffers on pause.
  if (paused)
    EmptyBuffers();
}

void SimpleAudio::Shutdown()
{
  if (!IsDeviceOpen())
    return;

  CloseDevice();
  EmptyBuffers();
  m_buffers.clear();
  m_buffer_size = 0;
  m_output_sample_rate = 0;
  m_channels = 0;
  m_output_paused = true;
}

void SimpleAudio::BeginWrite(SampleType** buffer_ptr, u32* num_samples)
{
  m_buffer_lock.lock();

  if (m_num_free_buffers == 0)
    DropBuffer();

  Buffer& buffer = m_buffers[m_first_free_buffer];
  *buffer_ptr = buffer.data.data() + buffer.write_position;
  *num_samples = m_buffer_size - buffer.write_position;
}

void SimpleAudio::WriteSamples(const SampleType* samples, u32 num_samples)
{
  std::lock_guard<std::mutex> guard(m_buffer_lock);
  u32 remaining_samples = num_samples;

  while (remaining_samples > 0)
  {
    if (m_num_free_buffers == 0)
      DropBuffer();

    Buffer& buffer = m_buffers[m_first_free_buffer];
    const u32 to_this_buffer = std::min(m_buffer_size - buffer.write_position, remaining_samples);

    const u32 copy_count = to_this_buffer * m_channels;
    std::memcpy(&buffer.data[buffer.write_position * m_channels], samples, copy_count * sizeof(SampleType));
    samples += copy_count;

    remaining_samples -= to_this_buffer;
    buffer.write_position += to_this_buffer;

    // End of the buffer?
    if (buffer.write_position == m_buffer_size)
    {
      // Reset it back to the start, and enqueue it.
      buffer.write_position = 0;
      m_num_free_buffers--;
      m_first_free_buffer = (m_first_free_buffer + 1) % m_buffers.size();
      m_num_available_buffers++;
    }
  }
}

void SimpleAudio::EndWrite(u32 num_samples)
{
  Buffer& buffer = m_buffers[m_first_free_buffer];
  DebugAssert((buffer.write_position + num_samples) <= m_buffer_size);
  buffer.write_position += num_samples;

  // End of the buffer?
  if (buffer.write_position == m_buffer_size)
  {
    // Reset it back to the start, and enqueue it.
    // Log_DevPrintf("Enqueue buffer %u", m_first_free_buffer);
    buffer.write_position = 0;
    m_num_free_buffers--;
    m_first_free_buffer = (m_first_free_buffer + 1) % m_buffers.size();
    m_num_available_buffers++;
  }

  m_buffer_lock.unlock();
}

u32 SimpleAudio::ReadSamples(SampleType* samples, u32 num_samples)
{
  std::lock_guard<std::mutex> guard(m_buffer_lock);
  u32 remaining_samples = num_samples;

  while (remaining_samples > 0 && m_num_available_buffers > 0)
  {
    Buffer& buffer = m_buffers[m_first_available_buffer];
    const u32 from_this_buffer = std::min(m_buffer_size - buffer.read_position, remaining_samples);

    const u32 copy_count = from_this_buffer * m_channels;
    std::memcpy(samples, &buffer.data[buffer.read_position * m_channels], copy_count * sizeof(SampleType));
    samples += copy_count;

    remaining_samples -= from_this_buffer;
    buffer.read_position += from_this_buffer;

    if (buffer.read_position == m_buffer_size)
    {
      // Log_DevPrintf("Finish dequeing buffer %u", m_first_available_buffer);
      // End of this buffer.
      buffer.read_position = 0;
      m_num_available_buffers--;
      m_first_available_buffer = (m_first_available_buffer + 1) % m_buffers.size();
      m_num_free_buffers++;
    }
  }

  return num_samples - remaining_samples;
}

void SimpleAudio::AllocateBuffers(u32 buffer_count)
{
  m_buffers.resize(buffer_count);
  for (u32 i = 0; i < buffer_count; i++)
  {
    Buffer& buffer = m_buffers[i];
    buffer.data.resize(m_buffer_size * m_channels);
    buffer.read_position = 0;
    buffer.write_position = 0;
  }

  m_first_available_buffer = 0;
  m_num_available_buffers = 0;
  m_first_free_buffer = 0;
  m_num_free_buffers = buffer_count;
}

void SimpleAudio::DropBuffer()
{
  DebugAssert(m_num_available_buffers > 0);
  // Log_DevPrintf("Dropping buffer %u", m_first_free_buffer);

  // Out of space. We'll overwrite the oldest buffer with the new data.
  // At the same time, we shift the available buffer forward one.
  m_first_available_buffer = (m_first_available_buffer + 1) % m_buffers.size();
  m_num_available_buffers--;

  m_buffers[m_first_free_buffer].read_position = 0;
  m_buffers[m_first_free_buffer].write_position = 0;
  m_num_free_buffers++;
}

void SimpleAudio::EmptyBuffers()
{
  for (Buffer& buffer : m_buffers)
  {
    buffer.read_position = 0;
    buffer.write_position = 0;
  }

  m_first_free_buffer = 0;
  m_num_free_buffers = static_cast<u32>(m_buffers.size());
  m_first_available_buffer = 0;
  m_num_available_buffers = 0;
}
