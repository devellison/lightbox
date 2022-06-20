/// \file buffer_memmap.cpp
/// Implementation of memory mapped buffers for v4l2
#if __linux__

#include "buffer_memmap.hpp"

#include <linux/videodev2.h>
#include <sys/mman.h>
#include <cstring>
#include <memory>

#include "errors.hpp"
#include "log.hpp"
#include "platform.hpp"

namespace zebral
{
BufferGroup::BufferGroup(DeviceV4L2Ptr& device, size_t numBuffers) : device_(device)
{
  // Request buffers
  struct v4l2_requestbuffers reqbuf;
  std::memset(&reqbuf, 0, sizeof(reqbuf));
  reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  reqbuf.count  = numBuffers;

  if (-1 == device_->ioctl(VIDIOC_REQBUFS, &reqbuf))
  {
    ZBA_THROW("Error allocating buffers.", Result::ZBA_CAMERA_ERROR);
  }

  if (reqbuf.count < numBuffers)
  {
    ZBA_THROW("Error allocating buffers.", Result::ZBA_CAMERA_ERROR);
  }

  // Memory map them
  for (size_t i = 0; i < numBuffers; ++i)
  {
    buffers_.emplace_back(device_, i);
  }
}

BufferGroup::~BufferGroup()
{
  buffers_.clear();

  // Free all buffers
  struct v4l2_requestbuffers reqbuf;
  memset(&reqbuf, 0, sizeof(reqbuf));
  reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  reqbuf.count  = 0;

  if (-1 == device_->ioctl(VIDIOC_REQBUFS, &reqbuf))
  {
    ZBA_ERRNO("Error freeing buffers!");
  }
}

BufferMemmap& BufferGroup::Get(size_t index)
{
  if (index >= buffers_.size())
  {
    ZBA_THROW("Invalid buffer index.", Result::ZBA_INVALID_RANGE);
  }
  return buffers_[index];
}

void BufferGroup::QueueAll()
{
  for (auto& curBuffer : buffers_)
  {
    curBuffer.Queue();
  }
}

BufferMemmap::BufferMemmap() : index_(0), length_(0), data_(nullptr) {}

BufferMemmap::BufferMemmap(DeviceV4L2Ptr& device, int idx)
    : device_(device),
      index_(idx),
      length_(0),
      data_(nullptr)
{
  // allocate new
  struct v4l2_buffer buffer;
  std::memset(&buffer, 0, sizeof(buffer));
  buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;
  buffer.index  = idx;

  if (-1 == device_->ioctl(VIDIOC_QUERYBUF, &buffer))
  {
    ZBA_THROW("Error querying buffer", Result::ZBA_CAMERA_ERROR);
  }

  length_ = buffer.length;
  data_ = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, *device_, buffer.m.offset);

  if (MAP_FAILED == data_)
  {
    ZBA_THROW("Error mapping memory!", Result::ZBA_CAMERA_ERROR);
  }
}

BufferMemmap::BufferMemmap(BufferMemmap&& buf) : index_(0), length_(0), data_(nullptr)
{
  std::swap(device_, buf.device_);
  std::swap(index_, buf.index_);
  std::swap(length_, buf.length_);
  std::swap(data_, buf.data_);
}

BufferMemmap::~BufferMemmap()
{
  if (data_)
  {
    munmap(data_, length_);
    data_   = nullptr;
    length_ = 0;
  }
}

void BufferMemmap::Queue()
{
  v4l2_buffer buffer;
  memset(&buffer, 0, sizeof(buffer));
  buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;
  buffer.index  = index_;
  if (-1 == device_->ioctl(VIDIOC_QBUF, &buffer))
  {
    ZBA_THROW("Error queuing buffer", Result::ZBA_CAMERA_ERROR);
  }
}

bool BufferMemmap::Dequeue()
{
  v4l2_buffer buffer;
  memset(&buffer, 0, sizeof(buffer));
  buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;
  buffer.index  = index_;
  if (-1 == device_->ioctl(VIDIOC_DQBUF, &buffer))
  {
    if (EAGAIN == errno) return false;
    ZBA_THROW_ERRNO("Error dequeuing buffer", Result::ZBA_CAMERA_ERROR);
  }
  return true;
}

}  // namespace zebral

#endif  // __linux__