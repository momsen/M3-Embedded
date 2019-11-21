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
    memset(data, 0, maxSize+1);
    uint32_t headlineLength = strlen(headline);
    strcpy(data, headline);
    data[headlineLength++] = '\n';
    content = &(data[headlineLength]);
    contentPtr = 0;
    contentSize = maxSize - headlineLength;
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
    if(contentPtr >= contentSize) {
      wrap();
    }

    content[contentPtr++] = c;
  }

  void newline() {
    appendChar('\n');
  }

  String getData() {
      return data;
  }

private:
  void wrap() {
    uint32_t firstNewLine = strcspn(content, "\n");
    if(firstNewLine == contentSize) {
      // no new line found: override last char
      contentPtr--;
      return;
    }
    
    firstNewLine++;
    contentPtr = contentSize - firstNewLine;
    memcpy(content, content + firstNewLine, contentPtr);
    memset(content + contentPtr, 0, contentSize - contentPtr);
  }

  char data[maxSize+1];
  char* content;
  uint32_t contentPtr;
  uint32_t contentSize;
};

} // namespace utils

#endif