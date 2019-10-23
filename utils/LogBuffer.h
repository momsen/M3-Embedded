#ifndef LOGBUFFER_H__
#define LOGBUFFER_H__

#include <Arduino.h>

namespace utils
{

template <uint32_t maxSize>
class LogBuffer
{
public:
  LogBuffer(const char* headline) {
    memset(content, 0, maxSize);
    headlineLength = strlen(headline);
    strcpy(content, headline);
    content[headlineLength++] = '\n';
    contentPtr = headlineLength;
  }

  void appendMessage(String message) {
    appendMessage(message.c_str());
  }

  void appendMessage(const char* string) {
    while(*string != '\0') {
      appendChar(*string++);
    }
  }

  void appendChar(const char c) {
    if(contentPtr >= maxSize) {
      wrap();
    }

    content[contentPtr++] = c;
  }

  void newline() {
    appendChar('\n');
  }

  String getContent() {
      return content;
  }

private:
  void wrap() {
    uint32_t firstNewLine = headlineLength;
    while(content[firstNewLine++] != '\n' && firstNewLine < maxSize) {
    }

    if(firstNewLine >= maxSize) {
      // no \n found, using removing first 100 bytes
      firstNewLine = headlineLength + 100;
    }

    memcpy(content + headlineLength, content + firstNewLine, firstNewLine * sizeof(char));
    memset(content + maxSize - firstNewLine, 0, firstNewLine * sizeof(char));
    contentPtr -= firstNewLine;
  }

  uint8_t headlineLength;
  char content[maxSize];
  uint8_t x;
  uint8_t y;
  uint32_t contentPtr;
};

} // namespace utils

#endif