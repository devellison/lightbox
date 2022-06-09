#ifndef LIGHTBOX_CAMERA_STORE_ERROR_HPP_
#define LIGHTBOX_CAMERA_STORE_ERROR_HPP_

#include <string>

namespace zebral
{
class StoreError final
{
 public:
  StoreError();
  ~StoreError();
  std::string ToString() const;
  int Get() const;

 private:
  int last_error_;
};
}  // namespace zebral
#endif