#ifndef LIGHTBOX_TEXTURE_HPP_
#define LIGHTBOX_TEXTURE_HPP_
#include "nano_inc.hpp"

#include "camera_frame.hpp"

namespace zebral
{
class Texture
{
 public:
  Texture() : textureId_(0) {}
  ~Texture()
  {
    if (textureId_)
    {
      glDeleteTextures(1, &textureId_);
      textureId_ = 0;
    }
  }

  GLuint Id() const
  {
    return textureId_;
  }

  void Load(const CameraFrame& frame)
  {
    GLint internalFormat = GL_RGB8;
    GLint format         = GL_BGR;
    if (textureId_)
    {
      glDeleteTextures(1, &textureId_);
      textureId_ = 0;
    }

    glGenTextures(1, &textureId_);
    glBindTexture(GL_TEXTURE_2D, textureId_);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, frame.width(), frame.height(), 0, format,
                 GL_UNSIGNED_BYTE, frame.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

 private:
  GLuint textureId_;
};

}  // namespace zebral
#endif  // LIGHTBOX_TEXTURE_HPP_