/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch and https://github.com/squix78/json-streaming-parser
*/

#include "JsonStreamingParser.h"

JsonStreamingParser::JsonStreamingParser() {
    state = STATE_START_DOCUMENT;
    bufferPos = 0;
    unicodeEscapeBufferPos = 0;
    unicodeBufferPos = 0;
    unicodeHighSurrogate = -1;
    characterCounter = 0;
}

void JsonStreamingParser::setListener(JsonListener* listener) {
  myListener = listener;
}

boolean JsonStreamingParser::isFinished() const {
  return state == STATE_DONE;
}

void JsonStreamingParser::parse(char c) {
    //System.out.print(c);
    // valid whitespace characters in JSON (from RFC4627 for JSON) include:
    // space, horizontal tab, line feed or new line, and carriage return.
    // thanks:
    // http://stackoverflow.com/questions/16042274/definition-of-whitespace-in-json
    if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
        && !(state == STATE_IN_STRING || state == STATE_UNICODE || state == STATE_START_ESCAPE
            || state == STATE_IN_NUMBER)) {
      return;
    }
    switch (state) {
    case STATE_IN_STRING:
      if (c == '"') {
        endString();
      } else if (c == '\\') {
        state = STATE_START_ESCAPE;
      } else if ((uint8_t)c <= 0x1f) {
        state = STATE_ERROR;
      } else {
        buffer[bufferPos] = c;
        increaseBufferPointer();
      }
      break;
    case STATE_IN_ARRAY:
      if (c == ']') {
        endArray();
      } else {
        startValue(c);
      }
      break;
    case STATE_IN_ARRAY_AFTER_COMMA:
      startValue(c);
      break;
    case STATE_IN_OBJECT:
      if (c == '}') {
        endObject();
      } else if (c == '"') {
        startKey();
      } else {
        state = STATE_ERROR;
      }
      break;
    case STATE_IN_OBJECT_AFTER_COMMA:
      if (c == '"') {
        startKey();
      } else {
        state = STATE_ERROR;
      }
      break;
    case STATE_END_KEY:
      if (c != ':') {
        state = STATE_ERROR;
        break;
      }
      state = STATE_AFTER_KEY;
      break;
    case STATE_AFTER_KEY:
      startValue(c);
      break;
    case STATE_START_ESCAPE:
      processEscapeCharacters(c);
      break;
    case STATE_UNICODE:
      processUnicodeCharacter(c);
      break;
    case STATE_UNICODE_SURROGATE:
      unicodeEscapeBuffer[unicodeEscapeBufferPos] = c;
      unicodeEscapeBufferPos++;
      if (unicodeEscapeBufferPos == 2) {
        endUnicodeSurrogateInterstitial();
      }
      break;
    case STATE_AFTER_VALUE: {
      if (stackPos <= 0) {
        state = STATE_ERROR;
        break;
      }
      int within = stack[stackPos - 1];
      if (within == STACK_OBJECT) {
        if (c == '}') {
          endObject();
        } else if (c == ',') {
          state = STATE_IN_OBJECT_AFTER_COMMA;
        } else {
          state = STATE_ERROR;
        }
      } else if (within == STACK_ARRAY) {
        if (c == ']') {
          endArray();
        } else if (c == ',') {
          state = STATE_IN_ARRAY_AFTER_COMMA;
        } else {
          state = STATE_ERROR;
        }
      } else {
        state = STATE_ERROR;
      }
    }break;
    case STATE_IN_NUMBER:
      if (c >= '0' && c <= '9') {
        buffer[bufferPos] = c;
        increaseBufferPointer();
      } else if (c == '.') {
        if (doesCharArrayContain(buffer, bufferPos, '.')) {
          state = STATE_ERROR;
          break;
        } else if (doesCharArrayContain(buffer, bufferPos, 'e')) {
          state = STATE_ERROR;
          break;
        }
        buffer[bufferPos] = c;
        increaseBufferPointer();
      } else if (c == 'e' || c == 'E') {
        if (doesCharArrayContain(buffer, bufferPos, 'e') ||
            doesCharArrayContain(buffer, bufferPos, 'E')) {
          state = STATE_ERROR;
          break;
        }
        buffer[bufferPos] = c;
        increaseBufferPointer();
      } else if (c == '+' || c == '-') {
        if (bufferPos <= 0) {
          state = STATE_ERROR;
          break;
        }
        char last = buffer[bufferPos - 1];
        if (!(last == 'e' || last == 'E')) {
          state = STATE_ERROR;
          break;
        }
        buffer[bufferPos] = c;
        increaseBufferPointer();
      } else {
        endNumber();
        // we have consumed one beyond the end of the number
        parse(c);
      }
      break;
    case STATE_IN_TRUE:
      buffer[bufferPos] = c;
      increaseBufferPointer();
      if (bufferPos == 4) {
        endTrue();
      }
      break;
    case STATE_IN_FALSE:
      buffer[bufferPos] = c;
      increaseBufferPointer();
      if (bufferPos == 5) {
        endFalse();
      }
      break;
    case STATE_IN_NULL:
      buffer[bufferPos] = c;
      increaseBufferPointer();
      if (bufferPos == 4) {
        endNull();
      }
      break;
    case STATE_START_DOCUMENT:
      myListener->startDocument();
      if (c == '[') {
        startArray();
      } else if (c == '{') {
        startObject();
      } else {
        state = STATE_ERROR;
      }
      break;
    case STATE_DONE:
      // Whitespace is returned before the switch; any other trailing byte is invalid.
      state = STATE_ERROR;
      break;
    //default:
      // throw new ParsingError($this->_line_number, $this->_char_number,
      // "Internal error. Reached an unknown state: ".$this->_state);
    }
    characterCounter++;
  }

void JsonStreamingParser::increaseBufferPointer() {
  if (bufferPos >= BUFFER_MAX_LENGTH - 1) {
    state = STATE_ERROR;
    return;
  }
  bufferPos++;
}

void JsonStreamingParser::endString() {
    if (stackPos <= 0) {
      state = STATE_ERROR;
      return;
    }
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped == STACK_KEY) {
      buffer[bufferPos] = '\0';
      myListener->key(String(buffer));
      state = STATE_END_KEY;
    } else if (popped == STACK_STRING) {
      buffer[bufferPos] = '\0';
      myListener->value(String(buffer));
      state = STATE_AFTER_VALUE;
    } else {
      state = STATE_ERROR;
      bufferPos = 0;
      return;
    }
    bufferPos = 0;
  }
void JsonStreamingParser::startValue(char c) {
    if (c == '[') {
      startArray();
    } else if (c == '{') {
      startObject();
    } else if (c == '"') {
      startString();
    } else if (isDigit(c)) {
      startNumber(c);
    } else if (c == 't') {
      state = STATE_IN_TRUE;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else if (c == 'f') {
      state = STATE_IN_FALSE;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else if (c == 'n') {
      state = STATE_IN_NULL;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else {
      state = STATE_ERROR;
    }
  }

boolean JsonStreamingParser::isDigit(char c) {
    // Only concerned with the first character in a number.
    return (c >= '0' && c <= '9') || c == '-';
  }

void JsonStreamingParser::endArray() {
    if (stackPos <= 0) {
      state = STATE_ERROR;
      return;
    }
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped != STACK_ARRAY) {
      state = STATE_ERROR;
      return;
    }
    myListener->endArray();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
      endDocument();
    }
  }

void JsonStreamingParser::startKey() {
    if (stackPos >= (int)(sizeof(stack) / sizeof(stack[0]))) {
      state = STATE_ERROR;
      return;
    }
    stack[stackPos] = STACK_KEY;
    stackPos++;
    state = STATE_IN_STRING;
  }

void JsonStreamingParser::endObject() {
    if (stackPos <= 0) {
      state = STATE_ERROR;
      return;
    }
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped != STACK_OBJECT) {
      state = STATE_ERROR;
      return;
    }
    myListener->endObject();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
      endDocument();
    }
  }

void JsonStreamingParser::processEscapeCharacters(char c) {
    if (c == '"') {
      buffer[bufferPos] = '"';
      increaseBufferPointer();
    } else if (c == '\\') {
      buffer[bufferPos] = '\\';
      increaseBufferPointer();
    } else if (c == '/') {
      buffer[bufferPos] = '/';
      increaseBufferPointer();
    } else if (c == 'b') {
      buffer[bufferPos] = 0x08;
      increaseBufferPointer();
    } else if (c == 'f') {
      buffer[bufferPos] = '\f';
      increaseBufferPointer();
    } else if (c == 'n') {
      buffer[bufferPos] = '\n';
      increaseBufferPointer();
    } else if (c == 'r') {
      buffer[bufferPos] = '\r';
      increaseBufferPointer();
    } else if (c == 't') {
      buffer[bufferPos] = '\t';
      increaseBufferPointer();
    } else if (c == 'u') {
      state = STATE_UNICODE;
    } else {
      state = STATE_ERROR;
    }
    if (state != STATE_UNICODE && state != STATE_ERROR) {
      state = STATE_IN_STRING;
    }
  }

void JsonStreamingParser::processUnicodeCharacter(char c) {
    if (!isHexCharacter(c)) {
      state = STATE_ERROR;
      return;
    }

    unicodeBuffer[unicodeBufferPos] = c;
    unicodeBufferPos++;

    if (unicodeBufferPos == 4) {
      int codepoint = getHexArrayAsDecimal(unicodeBuffer, unicodeBufferPos);
      endUnicodeCharacter(codepoint);
      return;
      /*if (codepoint >= 0xD800 && codepoint < 0xDC00) {
        unicodeHighSurrogate = codepoint;
        unicodeBufferPos = 0;
        state = STATE_UNICODE_SURROGATE;
      } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
        if (unicodeHighSurrogate == -1) {
          // throw new ParsingError($this->_line_number,
          // $this->_char_number,
          // "Missing high surrogate for Unicode low surrogate.");
        }
        int combinedCodePoint = ((unicodeHighSurrogate - 0xD800) * 0x400) + (codepoint - 0xDC00) + 0x10000;
        endUnicodeCharacter(combinedCodePoint);
      } else if (unicodeHighSurrogate != -1) {
        // throw new ParsingError($this->_line_number,
        // $this->_char_number,
        // "Invalid low surrogate following Unicode high surrogate.");
        endUnicodeCharacter(codepoint);
      } else {
        endUnicodeCharacter(codepoint);
      }*/
    }
  }
boolean JsonStreamingParser::isHexCharacter(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

int JsonStreamingParser::getHexArrayAsDecimal(char hexArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = hexArray[i];
      int value = 0;
      if (current >= 'a' && current <= 'f') {
        value = current - 'a' + 10;
      } else if (current >= 'A' && current <= 'F') {
        value = current - 'A' + 10;
      } else if (current >= '0' && current <= '9') {
        value = current - '0';
      }
      result = result * 16 + value;
    }
    return result;
  }

boolean JsonStreamingParser::doesCharArrayContain(char myArray[], int length, char c) {
    for (int i = 0; i < length; i++) {
      if (myArray[i] == c) {
        return true;
      }
    }
    return false;
  }

void JsonStreamingParser::endUnicodeSurrogateInterstitial() {
    if (unicodeEscapeBufferPos != 2 ||
        unicodeEscapeBuffer[0] != '\\' ||
        unicodeEscapeBuffer[1] != 'u') {
      state = STATE_ERROR;
      return;
    }
    unicodeBufferPos = 0;
    unicodeEscapeBufferPos = 0;
    state = STATE_UNICODE;
  }

void JsonStreamingParser::endNumber() {
    if (!isValidNumberBuffer()) {
      state = STATE_ERROR;
      bufferPos = 0;
      return;
    }
    buffer[bufferPos] = '\0';
    String value = String(buffer);
    //float result = 0.0;
    //if (doesCharArrayContain(buffer, bufferPos, '.')) {
    //  result = value.toFloat();
    //} else {
      // needed special treatment in php, maybe not in Java and c
    //  result = value.toFloat();
    //}
    myListener->value(value.c_str());
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

boolean JsonStreamingParser::isValidNumberBuffer() const {
    int pos = 0;
    if (bufferPos <= 0) return false;
    if (buffer[pos] == '-') {
      pos++;
      if (pos >= bufferPos) return false;
    }
    if (buffer[pos] == '0') {
      pos++;
      if (pos < bufferPos && buffer[pos] >= '0' && buffer[pos] <= '9') return false;
    } else {
      if (buffer[pos] < '1' || buffer[pos] > '9') return false;
      while (pos < bufferPos && buffer[pos] >= '0' && buffer[pos] <= '9') pos++;
    }
    if (pos < bufferPos && buffer[pos] == '.') {
      pos++;
      int fractionStart = pos;
      while (pos < bufferPos && buffer[pos] >= '0' && buffer[pos] <= '9') pos++;
      if (pos == fractionStart) return false;
    }
    if (pos < bufferPos && (buffer[pos] == 'e' || buffer[pos] == 'E')) {
      pos++;
      if (pos < bufferPos && (buffer[pos] == '+' || buffer[pos] == '-')) pos++;
      int exponentStart = pos;
      while (pos < bufferPos && buffer[pos] >= '0' && buffer[pos] <= '9') pos++;
      if (pos == exponentStart) return false;
    }
    return pos == bufferPos;
  }

int JsonStreamingParser::convertDecimalBufferToInt(char myArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = myArray[length - i - 1];
      result += (current - '0') * 10;
    }
    return result;
  }

void JsonStreamingParser::endDocument() {
    myListener->endDocument();
    state = STATE_DONE;
  }

void JsonStreamingParser::endTrue() {
    buffer[bufferPos] = '\0';
    String value = String(buffer);
    if (value.equals("true")) {
      myListener->value("true");
    } else {
      state = STATE_ERROR;
      bufferPos = 0;
      return;
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::endFalse() {
    buffer[bufferPos] = '\0';
    String value = String(buffer);
    if (value.equals("false")) {
      myListener->value("false");
    } else {
      state = STATE_ERROR;
      bufferPos = 0;
      return;
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::endNull() {
    buffer[bufferPos] = '\0';
    String value = String(buffer);
    if (value.equals("null")) {
      myListener->value("null");
    } else {
      state = STATE_ERROR;
      bufferPos = 0;
      return;
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::startArray() {
    if (stackPos >= (int)(sizeof(stack) / sizeof(stack[0]))) {
      state = STATE_ERROR;
      return;
    }
    myListener->startArray();
    state = STATE_IN_ARRAY;
    stack[stackPos] = STACK_ARRAY;
    stackPos++;
  }

void JsonStreamingParser::startObject() {
    if (stackPos >= (int)(sizeof(stack) / sizeof(stack[0]))) {
      state = STATE_ERROR;
      return;
    }
    myListener->startObject();
    state = STATE_IN_OBJECT;
    stack[stackPos] = STACK_OBJECT;
    stackPos++;
  }

void JsonStreamingParser::startString() {
    if (stackPos >= (int)(sizeof(stack) / sizeof(stack[0]))) {
      state = STATE_ERROR;
      return;
    }
    stack[stackPos] = STACK_STRING;
    stackPos++;
    state = STATE_IN_STRING;
  }

void JsonStreamingParser::startNumber(char c) {
    state = STATE_IN_NUMBER;
    buffer[bufferPos] = c;
    increaseBufferPointer();
  }

void JsonStreamingParser::endUnicodeCharacter(int codepoint) {
    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
      unicodeHighSurrogate = codepoint;
      unicodeBufferPos = 0;
      state = STATE_UNICODE_SURROGATE;
      return;
    }
    if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
      if (unicodeHighSurrogate < 0) {
        state = STATE_ERROR;
        return;
      }
      codepoint = ((unicodeHighSurrogate - 0xD800) * 0x400) +
                  (codepoint - 0xDC00) + 0x10000;
    } else if (unicodeHighSurrogate >= 0) {
      state = STATE_ERROR;
      return;
    }
    int requiredBytes;
    if (codepoint <= 0x7F) requiredBytes = 1;
    else if (codepoint <= 0x7FF) requiredBytes = 2;
    else if (codepoint <= 0xFFFF) requiredBytes = 3;
    else if (codepoint <= 0x10FFFF) requiredBytes = 4;
    else {
      state = STATE_ERROR;
      return;
    }
    if (bufferPos + requiredBytes >= BUFFER_MAX_LENGTH) {
      state = STATE_ERROR;
      return;
    }
    if (requiredBytes == 1) {
      buffer[bufferPos++] = (char)codepoint;
    } else if (requiredBytes == 2) {
      buffer[bufferPos++] = (char)(0xC0 | (codepoint >> 6));
      buffer[bufferPos++] = (char)(0x80 | (codepoint & 0x3F));
    } else if (requiredBytes == 3) {
      buffer[bufferPos++] = (char)(0xE0 | (codepoint >> 12));
      buffer[bufferPos++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
      buffer[bufferPos++] = (char)(0x80 | (codepoint & 0x3F));
    } else {
      buffer[bufferPos++] = (char)(0xF0 | (codepoint >> 18));
      buffer[bufferPos++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
      buffer[bufferPos++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
      buffer[bufferPos++] = (char)(0x80 | (codepoint & 0x3F));
    }
    unicodeBufferPos = 0;
    unicodeHighSurrogate = -1;
    state = STATE_IN_STRING;
  }

char JsonStreamingParser::convertCodepointToCharacter(int num) {
    if (num <= 0x7F)
      return (char) (num);
    // if(num<=0x7FF) return (char)((num>>6)+192) + (char)((num&63)+128);
    // if(num<=0xFFFF) return
    // chr((num>>12)+224).chr(((num>>6)&63)+128).chr((num&63)+128);
    // if(num<=0x1FFFFF) return
    // chr((num>>18)+240).chr(((num>>12)&63)+128).chr(((num>>6)&63)+128).chr((num&63)+128);
    return ' ';
  }
