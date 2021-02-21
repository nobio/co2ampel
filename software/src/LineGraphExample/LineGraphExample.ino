#include "LineGraph.h"

// Bufferinstance
U8G2_SSD1306_64X32_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 
LineGraph graph(&u8g2);

void setup() {

  Serial.begin(115200);
  
  for (int n = 0; n < 200; n++) {
    graph.push(10 + 10 * sin(n));
  }
  graph.draw();

}

void loop() {
}
