#ifndef LOGBUFFER_H__
#define LOGBUFFER_H__

#include <Arduino.h>

#define BYTES_PER_ROW (withHexBuffer ? (2 + charsPerRow * 6) : (charsPerRow + 1))

namespace utils
{

template <uint8_t charsPerRow, uint8_t maxRows, bool withHexBuffer>
class LogBuffer
{
public:
  LogBuffer(const char* headline)
    : rows(0),
      headlineLength(strlen(headline)),
      content(String(headline)) {
  }

  void appendMessage(String message) {
    appendMessage((uint8_t*)message.c_str(), message.length());
  }

  void appendMessage(const char* string) {
    appendMessage((uint8_t*)string, strlen(string));
  }

  void appendMessage(uint8_t* pkg, uint32_t len) {
    int ptr = 0;
    while(ptr < len) {
        appendRow(pkg + ptr, min(charsPerRow, (uint8_t)(len - ptr)));
        ptr += charsPerRow;
    }
  }

  void appendRow(uint8_t* chars, uint8_t len) {
    if(rows >= maxRows) {
        content.remove(headlineLength, BYTES_PER_ROW);
        rows--;
    }

    for(int i = 0; i < len; i++) {
        content += isChar(chars[i]) ? (char)chars[i] : '.';
    }
    
    for(int i = 0; i < charsPerRow - len; i++) {
        content += " ";
    }
    
    if(withHexBuffer) {
      content += "\t";

      for(int i = 0; i < len; i++) {
          content += chars[i] < 0x10 ? " 0x0" : " 0x";
          content += String(chars[i], HEX);
      }
      
      for(int i = 0; i < charsPerRow - len; i++) {
          content += "     ";
      }
    }

    content += "\n";
    rows++;
  }

  bool isChar(uint8_t c) {
    return c == ' ' || c == ':' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
  }

  String get_content() {
      return content;
  }

private:
    int headlineLength;
    String content;
    String hexContent;
    String binaryContent;
    int rows;
};

} // namespace utils

#endif