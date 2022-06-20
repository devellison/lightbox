#ifndef LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_
#define LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_

#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace zebral
{
class SerialLineChannel
{
 public:
  SerialLineChannel(const std::string& path);
  ~SerialLineChannel();

  static std::vector<std::string> Enumerate();

 protected:
  std::string path_;
  std::thread read_thread_;
  std::mutex mutex_;
};
}  // namespace zebral
#endif  // LIGHTBOX_SERIAL_SERIAL_LINE_CHANNEL_HPP_
