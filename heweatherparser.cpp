#include "heweatherparser.h"
#include "JsonListener.h"


void heweatherListener::whitespace(char c) {
  Serial.println("whitespace");
}

void heweatherListener::startDocument() {
  Serial.println("start document");
}

void heweatherListener::key(String key) {
   
  
  Serial.println("key: " + key);
}

void heweatherListener::value(String value) {
 
  
  Serial.println("value: " + value);
}

void heweatherListener::endArray() {
  Serial.println("end array. ");
}

void heweatherListener::endObject() {
  Serial.println("end object. ");
}

void heweatherListener::endDocument() {
  Serial.println("end document. ");
}

void heweatherListener::startArray() {
   Serial.println("start array. ");
}

void heweatherListener::startObject() {
   Serial.println("start object. ");
}

