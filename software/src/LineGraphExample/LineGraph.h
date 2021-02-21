#include <Arduino.h>
#include "CircularBuffer.h"
#include <U8g2lib.h>
#define WIDTH  180  // width of the display
#define HEIGHT 80   // height of the display
#define P      4    // padding (Rand)


/*
||
||
*/
class LineGraph {
public:
  LineGraph(U8G2_SSD1306_64X32_NONAME_F_HW_I2C *u8g2);
  //LineGraph(int width, int height);

  // add new value
  void push(float value);     // add a new float value to the buffer
  void draw();                // draw data on the disply

private:
  
  CircularBuffer <float, WIDTH-2*P> buffer;               // float buffer storing original values
  CircularBuffer <int,   WIDTH-2*P> transformBuffer;      // transformed int values
  U8G2_SSD1306_64X32_NONAME_F_HW_I2C *u8g2;                                           // reference to Display
  
  int size();                                             // size of internal queue buffer
  void transformData();                                   // transform data from raw data fitting the display
  void dump();                                            // pint the raw and transformed data
};

/* ============== public ===================*/
LineGraph::LineGraph(U8G2_SSD1306_64X32_NONAME_F_HW_I2C * _u8g2) {
  u8g2 = _u8g2;
}
/*
LineGraph::LineGraph(const int _width, const int _height) {
  //width = _width;
  //height = _height;
}
*/
void LineGraph::push(float value) {
  buffer.unshift(value);
}

void LineGraph::draw() {
  /*

          0  - (0, 0)                                      + (WIDTH, 0)
             |
          P  -
             |
             |
             |
             |
             |
             |
             |
             |
   HEIGHT-P  -
             |                                             + (WIDTH, HEIGHT)
   HEIGHT    +---|-------------------------------------|---|------>
             0   P                                WIDTH-P WIDTH
   */
  // calculate transformation of raw data
  transformData();
  dump();

  // draw axis: x axis (horizontal): drawHLine(x, y, length) <- (x,y) left end
  u8g2->drawHLine(P/2, HEIGHT - P/2, WIDTH - P);
  // draw axis: y axis (vertical): drawVLine(x, y, length)   <- (x,y) upper dnd
  u8g2->drawVLine(P/2, P/2, HEIGHT - P);
  
  // draw scatter 
  int x, y;
  for(int i = 0; i < transformBuffer.size(); i++) {
    x = i + P/2;
    y = transformBuffer[i];
    u8g2->drawPixel(x, y);
  }
}

int LineGraph::size() {
  return buffer.size();
}
/* ============== private ===================*/

/**
 * calculate the transformed data from raw data
 */
void LineGraph::transformData() {
  transformBuffer.clear();

  // calculate max value of buffer
  float max = 0;
  for(int i = 0; i < buffer.size(); i++) {
    if(max < buffer[i]) max = buffer[i];
  }

  // transform values and store it in other direction: first value comes last
  float y, trans;
  for(int i = 0; i < buffer.size(); i++) {
    y = buffer[i];
    trans = HEIGHT * (1 - y/max) - P * (1 - 2*y/max);
    transformBuffer.push(trans);
  }

}

void LineGraph::dump() {
  for(int i = 0; i < buffer.size(); i++) {
    Serial.print(buffer[i]);
    Serial.print(" -> (x=");
    Serial.print(i + P/2);
    Serial.print("; y=");
    Serial.print(transformBuffer[i]);
    Serial.println(")");
  }
}
