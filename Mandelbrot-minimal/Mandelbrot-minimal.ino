/*
 * Minimal Mandelbrot for FastLED on Arduino
 * 
 * By: Andrew Tuline
 * 
 * Date: January 2021
 * 
 * A static Mandelbrot display for a 16x16 (resizeable) FastLED array.
 * 
 * Based on an example in the Processing language at:
 * 
 * https://processing.org/examples/mandelbrot.html
 * 
 * An online interactive Mandelbrot set in order to help determine valid min and max values:
 * 
 * https://mandel.gart.nz/
 * 
 * Although this will run on an Arduino UNO, it does so at about 7 frames per second, whereas, you can get about 100 on an ESP32.
 * 
 * 
 * How to calculate a Mandelbrot set:
 * 
 * Either a series of calculations stays within a maximum value (and is in the Mandelbrot set), or they approach infinity.
 * We start with a grid of x,y coordinates.
 * We need to determine which x,y grid co-ordinates are inside and outside of the Mandelbrot set.
 * The y axis uses complex numbers.
 * A complex number includes 'i', where i^2 = -1.
 * An example complex number is 3i, so (3i)^2 = -9.
 * The Mandelbrot formula is z = z^2 + c.
 * Where 'c' is the current x,y or (real, imaginary) location.
 * The initial value of 'z' is 0.
 * It's an iterative function, meaning you run it again and again. . . 
 * After some iterations, if abs(z) > a maxiumum value, then stop.
 * You can pre-define the maximum number of iterations.
 * 
 * z = z^2 + c
 * 
 * Since we have a point 'c' in a real and complex plane, an example of that point is (2,3i).
 * The initial value of z is 0.
 * The next value of z will be 'c' or (2,3i).
 * You can use the variables x and y for those values.
 * For the next iteration, for z^2, you need to calculate:
 * 
 * (2,3i) * (2,3i)
 * 
 * Using the foil multiplocation method (first, outer, inner, last), you get:
 * 
 * = 2*2 + 2*3i + 2*3i + 3i*3i    Where 3i*3i is the same as (3i)^2, which is 9*(-1) or -9.
 * = 4 + 6i + 6i + 9*i^2
 * = 4 + 12i -9                   Where i^2 = -1 and 3*3 = 9 so result is -9.
 * = -5 + 12i                     This is the result of z^2.
 * 
 * Finishing off, z = z^2 + c
 * 
 * z = (-5 + 12i) + 2 + 3i
 * z = -3 + 15i
 *
 * 
 * To get the maximum value of that point, we calculate its length:
 * 
 * |z| = sqrt(x^2 + y^2)
 * or z^2 = x^2 + y^2         In order to avoid the square root calculation, just make maximum value larger.
 *      = -3*-3 + 15i*15i     Remember, i*i = -1
 *      = 9 - 225
 *      = -216                The absolute value of this is out of any reasonable range, so stop immediately.
 *      
 * Count the number of times you get to the maximum value, and use that for your colour.
 * If you never make it, the colour will be black and that point is INSIDE the Mandelbrot set.
 *   
 * Review the source code below to see how these are easily calculated.
 * 
 */


#include <FastLED.h>

#define LED_PIN  2
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define matrixSerpentineLayout true

const uint8_t matrixWidth = 16;
const uint8_t matrixHeight = 16;

#define NUM_LEDS matrixWidth * matrixHeight

CRGB leds[NUM_LEDS];
// Palette definitions
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;

// These initial values show the whole thing on the matrix.
float reAl = -.5;               // Relatively close to https://mandel.gart.nz/#/
float imAg = 0.1;
float zoOm = 550.;

// Calculated start/stop coordinates.
float xmin, ymin, xmax, ymax;   // Our window.
float dx;                       // Delta x is mapped to the matrix size.
float dy;                       // Delta y is mapped to the matrix size.

int maxIterations = 15;        // How many iterations per pixel before we give up. Make it 8 bits to match our range of colours.
float maxCalc = 16.0;           // How big is each calculation allowed to be before we give up.



void setup() {
  delay(1000);
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(32);
} // setup()



void loop() {

  resize();                               // Define the window to display.
  mandel();                               // Calculate and display that window.

  FastLED.show();                         // FastLED now displays the results.
  
} // loop()



void resize() {                           // Resize the minimum and maximum values on the fly to display.

//  reAl = -1.304605388;                    // Try another location.
//  imAg = 0.;
//  zoOm = 92682.;

//  zoOm = 5000 * abs(sin((float)millis()/2000.))+400;   // Animate the zoom.
  
  const float zoomAdj = .0016;             // Adjust zoom factor so that it's compatible with https://mandel.gart.nz/#/
  const float xadj = 0.;                   // Same for x and y values.
  const float yadj = .001;
  
  xmin = reAl - 1./zoOm/zoomAdj+xadj;     // Scale the windows in accordance with the zoom factor.
  xmax = reAl + 1./zoOm/zoomAdj+xadj;
  ymin = imAg - 1./zoOm/zoomAdj+yadj;
  ymax = imAg + 1./zoOm/zoomAdj+yadj;

  dx = (xmax - xmin) / (matrixWidth);     // Scale the delta x and y values to our matrix size.
  dy = (ymax - ymin) / (matrixHeight);

} // resize()



void mandel() {                             // Calculate and display the Mandelbrot set for the current window.
  // Start y
  float y = ymin;
  for (int j = 0; j < matrixHeight; j++) {
    
    // Start x
    float x = xmin;
    for (int i = 0; i < matrixWidth; i++) {
  
      // Now we test, as we iterate z = z^2 + c does z tend towards infinity?
      float a = x;
      float b = y;
      int iter = 0;
  
      while (iter < maxIterations) {    // Here we determine whether or not we're out of bounds.
        float aa = a * a;
        float bb = b * b;
        float len = aa + bb;
        if (len > maxCalc) {            // |z| = sqrt(a^2+b^2) OR z^2 = a^2+b^2 to save on having to perform a square root.
          break;  // Bail
        }
        
       // This operation corresponds to z -> z^2+c where z=a+ib c=(x,y). Remember to use 'foil'.      
        b = 2*a*b + y;
        a = aa - bb + x;
        iter++;
      } // while
  
      // We color each pixel based on how long it takes to get to infinity, or black if it never gets there.
      if (iter == maxIterations) {
        leds[XY(i,j)] = CRGB::Black;            // Calculation kept on going, so it was within the set.
      } else {
        leds[XY(i,j)] = CHSV(iter*255/maxIterations,255,255);   // Near the edge of the set.
      }
      x += dx;
    }
    y += dy;
  }

//  blur2d( leds, matrixWidth, matrixHeight, 64);

} // mandel()



// Use this for your matrix. Could be serpentine or not.
uint16_t XY( uint8_t x, uint8_t y) {

  uint16_t i;
 
  if( matrixSerpentineLayout == false) {
    i = (y * matrixWidth) + x;
  }
 
  if( matrixSerpentineLayout == true) {
    if( y & 0x01) {
      uint8_t reverseX = (matrixWidth - 1) - x;
      i = (y * matrixWidth) + reverseX;
    } else {
      i = (y * matrixWidth) + x;
    }
  }
  return i;
 
} // XY()
