/*
 * Julia set for FastLED on Arduino
 * 
 * By: Andrew Tuline
 * 
 * Date: January 2021
 * 
 * An animated Julia display for a 16x16 (resizeable) FastLED array.
 * 
 * Originally based on an example in the Processing language at:
 * 
 * https://processing.org/examples/mandelbrot.html
 * 
 * 
 * Julia vs Mandelbrot Sets
 * 
 * Both sets are based on the calculation of:
 * 
 * z = z^2 + c
 * 
 * In both sets, 'z' is the value of an iterative calculation of real and imaginary numbers, such as (2,3i).
 * 
 * In a Mandelbrot set 'z' starts out at (0,0i), and 'c' is your current location on a real/imaginary plane within a pre-defined 
 * window, i.e. (-2,-2i) to (2,2i). For each location in that window, you run that calculation until you exceed a limit, or 
 * stop after some number of calculations.
 * 
 * In a Julia set, 'z' start out as an initial location in that window, while 'c' is a number on that plane
 * within the set. As a result, you get an extra number to animate with when using a Julia set.
 * 
 * To animate the Mandelbrot set, you will need to either zoom in/out and/or translate your viewpoint.
 * 
 * To animate a Julia set, in addition to the above, you can animate it by continuously modifying the value of 'c'.
 * 
 * The trick is the find the right window and the right range of 'c' in order to make your animation look great.
 * 
 */


#include <FastLED.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define LED_PIN  2
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define matrixSerpentineLayout true

const uint8_t matrixWidth = 16;
const uint8_t matrixHeight = 16;

#define NUM_LEDS matrixWidth * matrixHeight

CRGB leds[NUM_LEDS];

// This array allow addressing of the palettes by an index number rather than name.
FL_PROGMEM extern const TProgmemRGBPalette16 *const palettes[] = {
  &PartyColors_p, &CloudColors_p, &OceanColors_p, &ForestColors_p,
  &LavaColors_p, &HeatColors_p, &RainbowColors_p
  };


CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette = PartyColors_p;

// These initial values show the whole thing on the matrix.
float reAl;
float imAg;

// Calculated start/stop coordinates.
float xmin, ymin, xmax, ymax;   // Our window.
float dx;                       // Delta x is mapped to the matrix size.
float dy;                       // Delta y is mapped to the matrix size.

const int maxIterations = 15;  // How many iterations per pixel before we give up. Make it 8 bits to match our range of colours.
const uint8_t maxCalc = 16;    // How big is each calculation allowed to be before we give up.



void setup() {
  delay(1000);
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(32);
} // setup()



void loop() {
  static uint8_t palette_num = 0;
  
  EVERY_N_MILLISECONDS(10) {
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }

  EVERY_N_SECONDS(5) {
   palette_num = addmod8(1,palette_num,ARRAY_SIZE(palettes));
    targetPalette = *(TProgmemRGBPalette16 *) pgm_read_ptr_near(&palettes[palette_num]);   
  }

  resize();                               // Define the window to display.
  mandel();                               // Calculate and display that window.

  FastLED.show();                         // FastLED now displays the results.
  
} // loop()



void resize() {                           // Resize the minimum and maximum values on the fly to display.


//  reAl = -0.7;                            // Very busy with 255 calculations.
//  imAg = 0.27015;

  reAl = -0.94299;                        // PixelBlaze with 15 calculations.
  imAg = 0.3162;

  reAl += sin((float)millis()/305.)/20.;
  imAg += sin((float)millis()/405.)/20.;

  Serial.print(reAl,4); Serial.print("\t"); Serial.print(imAg,4); Serial.println(" ");

// Whole set should be within -1.2,1.2 to -.8 to 1.

  xmin = -.5;
  xmax = .5;
  ymin = -.5;
  ymax = .5;

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
        b = 2*a*b + imAg;
        a = aa - bb + reAl;
        iter++;
      } // while
  
      // We color each pixel based on how long it takes to get to infinity, or black if it never gets there.
      if (iter == maxIterations) {
        leds[XY(i,j)] = CRGB::Black;            // Calculation kept on going, so it was within the set.
      } else {
        leds[XY(i,j)] = ColorFromPalette(currentPalette, iter*255/maxIterations, 255, LINEARBLEND);
      }
      x += dx;
    }
    y += dy;
  }
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
