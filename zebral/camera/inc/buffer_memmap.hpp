/// \file buffer_memmap.hpp
/// Utility class for keeping track of V4L2 memory-mapped buffers
#ifndef LIGHTBOX_CAMERA_BUFFER_MEMMAP_HPP_
#define LIGHTBOX_CAMERA_BUFFER_MEMMAP_HPP_

#include <memory>
#include <vector>
#include "device_v4l2.hpp"

namespace zebral
{
/// This class handles a V4L2 capture buffer for us.
/// BufferGroup will contain all the buffers for a device.
class BufferMemmap
{
 public:
  /// Default ctor
  BufferMemmap();

  /// Initializer
  /// \param file_desc - device handle for buffer
  /// \param idx - buffer index for the device
  BufferMemmap(DeviceV4L2Ptr& device, int idx);

  /// Dtor - unmaps memory
  ~BufferMemmap();

  /// Copy ctor - need to keep object vectors
  BufferMemmap(BufferMemmap&&);

  /// Retrieves data ptr
  /// \returns void* - pointer to mapped data buffer
  void* Data()
  {
    return data_;
  }

  /// Retrieves length
  /// \returns size_t - length of the data buffer in bytes
  size_t Length()
  {
    return length_;
  }

  /// Queues the buffer up for the devices
  void Queue();

  /// Returns true if a buffer is dequeued with data
  /// \returns true on success, false if we don't have a buffer.
  bool Dequeue();

 protected:
  DeviceV4L2Ptr device_;  ///< Device handle
  int index_;             ///< Buffer index
  size_t length_;         ///< Length of buffer in bytes
  void* data_;            ///< Ptr to buffer
};

/// This class handles all the buffers for
/// a device.
class BufferGroup
{
 public:
  /// Ctor
  /// \param device - Device handle
  /// \param numBuffers - number of buffers we want for the device
  BufferGroup(DeviceV4L2Ptr& device, size_t numBuffers);

  /// Dtor - cleans up the buffers and releases them
  ~BufferGroup();

  /// Retrieves a buffer
  BufferMemmap& Get(size_t index);

  /// Queues all the buffers for use by the device
  void QueueAll();

 private:
  DeviceV4L2Ptr device_;               ///< device handle
  std::vector<BufferMemmap> buffers_;  ///< allocated/memmapped buffers
};

}  // namespace zebral

#endif  // LIGHTBOX_CAMERA_BUFFER_MEMMAP_HPP_