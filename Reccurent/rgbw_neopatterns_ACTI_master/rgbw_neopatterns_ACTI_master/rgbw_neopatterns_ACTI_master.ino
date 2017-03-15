#include <Wire.h>
#include <Adafruit_NeoPixel.h>

bool sunWipeComplete = false;
uint8_t earthIterations = 0;

bool ACT2FadeOut = true;

bool firstTimeInACT3 = true;
bool ACT3FadeOut = false;

bool ACT4FadeOutComplete = false;

uint8_t rotationCounter = 0;
uint8_t ACT5Cycles = 0;
bool firstTimeInACT5 = true;

uint8_t ACT = 1;

unsigned long timeSinceACTStart = 0;

// Pattern types supported:
enum  pattern { NONE, COLOR_WIPE, SCANNER, FADE, GRADIENT, ORBIT, ONLY_WHITE, FADE_OUT };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
  public:

    // Member Variables:
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern

    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position

    uint32_t Color1, Color2, Color3;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern

    void (*OnComplete)();  // Callback on completion of pattern

    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
      : Adafruit_NeoPixel(pixels, pin, type)
    {
      OnComplete = callback;
    }

    // Update the pattern
    void Update()
    {
      if ((millis() - lastUpdate) > Interval) // time to update
      {
        Serial.print("ACT: ");
        Serial.println(ACT);

        lastUpdate = millis();
        switch (ActivePattern)
        {
          case COLOR_WIPE:
            ColorWipeUpdate();
            break;
          case SCANNER:
            ScannerUpdate();
            break;
          case FADE:
            FadeUpdate();
            break;
          case GRADIENT:
            GradientUpdate();
            break;
          case ORBIT:
            OrbitUpdate();
            break;
          case ONLY_WHITE:
            OnlyWhiteUpdate();
            break;
          case FADE_OUT:
            FadeOutUpdate();
            break;
          default:
            break;
        }
      }
    }

    // Increment the Index and reset at the end
    void Increment()
    {
      if (Direction == FORWARD)
      {
        Index++;
        if (Index >= TotalSteps)
        {
          Index = 0;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback
          }
        }
      }
      else // Direction == REVERSE
      {
        --Index;
        if (Index <= 0)
        {
          Index = TotalSteps - 1;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback
          }
        }
      }
    }

    // Reverse pattern direction
    void Reverse()
    {
      if (Direction == FORWARD)
      {
        Direction = REVERSE;
        //Index = TotalSteps - 1; ////////
      }
      else
      {
        Direction = FORWARD;
        //Index = 0; ////////
      }
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = COLOR_WIPE;
      Interval = interval;
      TotalSteps = numPixels();
      Color1 = color;
      Index = 0;
      Direction = dir;
    }

    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
      setPixelColor(Index, Color1);
      show();
      Increment();
    }

    void Orbit(uint32_t color1, uint8_t index)
    {
      ActivePattern = ORBIT;
      Interval = 40;
      TotalSteps = (numPixels() - 1) * 2;
      Color1 = color1;
      Index = index;
    }

    void OrbitUpdate()
    {
      Serial.println(Index);

      for (int i = 0; i < numPixels(); i++)
      {
        if (i == Index)  // Scan Pixel to the right
        {
          setPixelColor(i, Color1);
        }
        else if (i == TotalSteps - Index) // Scan Pixel to the left
        {
          setPixelColor(i, Color1);
        }
        else // Fading (white rabbit) tail
        {
          setPixelColor(i, DimColor(getPixelColor(i)));
        }
      }

      rotationCounter++;
      if (rotationCounter == 15)
      {
        Reverse();
        //rotationCounter = 0;
      }

      else if (rotationCounter == 30)
      {
        Reverse();
        //Index -= rotationCounter/2;
        rotationCounter = 0;
      }


      show();
      Increment();
    }

    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
      ActivePattern = SCANNER;
      Interval = interval;
      TotalSteps = (numPixels() - 1) * 2;
      Color1 = color1;
      Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    {

      //Check if all are aligned
      Wire.requestFrom(8, 8);
      uint16_t slave1Index = Wire.read();
      if (slave1Index > 144)
        slave1Index = 144 - (slave1Index - 144);

      Wire.requestFrom(9, 8);
      uint16_t slave2Index = Wire.read();
      if (slave2Index > 144)
        slave2Index = 144 - (slave2Index - 144);

      uint16_t index = Index;
      if (index > 144)
        index = 144 - (index - 144);

      //DEBUG

      //      Serial.print("ACT: ");
      //      Serial.println(ACT);
      Serial.print("Master Index: ");
      Serial.println(index);
      Serial.print("Slave1 Index: ");
      Serial.println(slave1Index);
      Serial.print("Slave2 Index: ");
      Serial.println(slave2Index);
      Serial.println();
      Serial.print("Timer: " );
      Serial.println(millis() - timeSinceACTStart);

      if (ACT == 1)
      {
        //      if (sunWipeComplete && index > 15 &&
        //          (abs(index - slave1Index) <= 5 && abs(index - slave2Index) <= 5) ||
        //          (abs(slave1Index - index) <= 5 && abs(slave1Index - slave2Index) <= 5) ||
        //          (abs(slave2Index - index) <= 5 && abs(slave2Index - slave1Index) <= 5))
        if (millis() - timeSinceACTStart > 5000 &&
            sunWipeComplete &&
            index > 5 &&
            (abs(index - slave1Index) <= 5 &&
             abs(index - slave2Index) <= 5))
        {
          byte x = 2;
          Wire.beginTransmission(8);
          Wire.write(x);
          Wire.endTransmission();

          Wire.beginTransmission(9);
          Wire.write(x);
          Wire.endTransmission();
          //Start next ACT
          //Serial.println("AYY GUESS WHAT, ALL DOSE MOTHERFUCKERS BE ALIGNED! YIHUUU!");

          //fade sun moon earth
          //Scanner(Color(0, 0, 0), 80);
          //TotalSteps = 10;

          FadeOut(50, 10);

          ACT++;
        }
      }
      //end of act 3
      else if (ACT == 3)
      {
        //3 white met, transition to next scene
        if (millis() - timeSinceACTStart > 5000 &&
            index < 130 &&
            abs(index - slave1Index) <= 5 &&
            abs(index - slave2Index) <= 5)
        {
          byte x = 3;
          Wire.beginTransmission(8);
          Wire.write(x);
          Wire.endTransmission();

          Wire.beginTransmission(9);
          Wire.write(x);
          Wire.endTransmission();

          //Scanner(Color(0, 0, 0), 80);
          //TotalSteps = 10;
          FadeOut(50, 10);

          ACT3FadeOut = true;
        }
      }
      //end of act 4
      else if (ACT == 4)
      {
        if (index > 5 &&
            (abs(index - slave1Index) <= 5 && abs(index - slave2Index) <= 5) ||
            (abs(slave1Index - index) <= 5 && abs(slave1Index - slave2Index) <= 5) ||
            (abs(slave2Index - index) <= 5 && abs(slave2Index - slave1Index) <= 5))
        {
          byte x = 4;
          Wire.beginTransmission(8);
          Wire.write(x);
          Wire.endTransmission();

          Wire.beginTransmission(9);
          Wire.write(x);
          Wire.endTransmission();

          /////////fade out and transition to act 5
          //Scanner(Color(0, 0, 0), 80);
          //TotalSteps = 10;
          FadeOut(50, 10);

          ACT4FadeOutComplete = true;

        }
      }

      for (int i = 0; i < numPixels(); i++)
      {
        if (i == Index)  // Scan Pixel to the right
        {
          setPixelColor(i, Color1);
        }
        else if (i == TotalSteps - Index) // Scan Pixel to the left
        {
          setPixelColor(i, Color1);
        }
        else // Fading (white rabbit) tail
        {
          if (sunWipeComplete)
          {
            if (ACT == 1)
            {
              //don't fade sun until earth has passed by
              if (getPixelColor(i) != Wheel(15))
                setPixelColor(i, DimColor(getPixelColor(i)));
            }
            else
            {
              setPixelColor(i, DimColor(getPixelColor(i)));
            }
          }
        }
      }
      show();
      Increment();
    }

    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = FADE;
      Interval = interval;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Index = 0;
      Direction = dir;
    }

    // Update the Fade Pattern
    void FadeUpdate()
    {
      // Calculate linear interpolation between Color1 and Color2
      // Optimise order of operations to minimize truncation error
      uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
      uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
      uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;

      ColorSet(Color(red, green, blue));
      show();
      Increment();
    }

    void Gradient(uint32_t color1, uint32_t color2, uint8_t interval, uint32_t color3 = 0, uint16_t steps = 144, direction dir = FORWARD)
    {
      ActivePattern = GRADIENT;
      Interval = interval;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Color3 = color3;
      Index = 0;
      Direction = dir;
    }

    void GradientUpdate()
    {
      if (ACT == 7 || ACT == 8 || ACT == 9)
      {
        if (Index == TotalSteps / 2 + 2)
          OnComplete();
      }

      //two color gradient
      if (Color3 == 0 && ACT != 9)
      {
        for (uint8_t i = 0; i < Index; i++)
        {
          uint8_t red = ((Red(Color1) * (TotalSteps - i)) + (Red(Color2) * i)) / TotalSteps;
          uint8_t green = ((Green(Color1) * (TotalSteps - i)) + (Green(Color2) * i)) / TotalSteps;
          uint8_t blue = ((Blue(Color1) * (TotalSteps - i)) + (Blue(Color2) * i)) / TotalSteps;

          setPixelColor(i, Color(red, green, blue));
        }
      }
      //three color gradient
      else
      {
        for (uint8_t i = 0; i < Index; i++)
        {
          uint8_t red, green, blue;

          if (i <= numPixels() / 2)
          {
            red = ((Red(Color1) * ((TotalSteps / 2) - i)) + (Red(Color2) * i)) / (TotalSteps / 2);
            green = ((Green(Color1) * ((TotalSteps / 2) - i)) + (Green(Color2) * i)) / (TotalSteps / 2);
            blue = ((Blue(Color1) * ((TotalSteps / 2) - i)) + (Blue(Color2) * i)) / (TotalSteps / 2);
          }
          else
          {
            uint8_t j = i % (TotalSteps / 2);

            red = ((Red(Color2) * ((TotalSteps / 2) - j)) + (Red(Color3) * j)) / (TotalSteps / 2);
            green = ((Green(Color2) * ((TotalSteps / 2) - j)) + (Green(Color3) * j)) / (TotalSteps / 2);
            blue = ((Blue(Color2) * ((TotalSteps / 2) - j)) + (Blue(Color3) * j)) / (TotalSteps / 2);
          }

          setPixelColor(i, Color(red, green, blue));
        }

        //push in from top and bottom
        if (ACT == 7 || ACT == 9)
        {
          for (uint8_t i = TotalSteps; i > TotalSteps - Index; i--)
          {
            uint8_t red, green, blue;

            if (i <= numPixels() / 2)
            {
              red = ((Red(Color1) * ((TotalSteps / 2) - i)) + (Red(Color2) * i)) / (TotalSteps / 2);
              green = ((Green(Color1) * ((TotalSteps / 2) - i)) + (Green(Color2) * i)) / (TotalSteps / 2);
              blue = ((Blue(Color1) * ((TotalSteps / 2) - i)) + (Blue(Color2) * i)) / (TotalSteps / 2);
            }
            else
            {
              uint8_t j = i % (TotalSteps / 2);

              red = ((Red(Color2) * ((TotalSteps / 2) - j)) + (Red(Color3) * j)) / (TotalSteps / 2);
              green = ((Green(Color2) * ((TotalSteps / 2) - j)) + (Green(Color3) * j)) / (TotalSteps / 2);
              blue = ((Blue(Color2) * ((TotalSteps / 2) - j)) + (Blue(Color3) * j)) / (TotalSteps / 2);
            }

            setPixelColor(i, Color(red, green, blue));
          }
        }
      }

      if (ACT == 6 && Index != TotalSteps - 1 )
      {
        setPixelColor(Index, Wheel(15));
        if (Index > 1)
          setPixelColor(Index - 1, Wheel(15));
        if (Index > 2)
          setPixelColor(Index - 2, Wheel(15));
      }

      show();
      Increment();
    }

    void OnlyWhite(uint8_t interval, uint8_t steps)
    {
      ActivePattern = ONLY_WHITE;
      Interval = interval;
      TotalSteps = steps;
      Index = 0;
    }

    void OnlyWhiteUpdate()
    {
      if (Index == TotalSteps / 2 + 2)
        OnComplete();
      /*
        for (int i = 0; i < numPixels(); i++)
        {
        //pixel has no red, and therefore isn't white in our case
        if (Red(getPixelColor(i)) < 50)
        {
          uint8_t red = ((Red(getPixelColor(i)) * (TotalSteps - Index)) + (Red(Color(0, 0, 0)) * Index)) / TotalSteps;
          uint8_t green = ((Green(getPixelColor(i)) * (TotalSteps - Index)) + (Green(Color(0, 0, 0)) * Index)) / TotalSteps;
          uint8_t blue = ((Blue(getPixelColor(i)) * (TotalSteps - Index)) + (Blue(Color(0, 0, 0)) * Index)) / TotalSteps;

          setPixelColor(i, Color(red, green, blue));

          //setPixelColor(i, DimColor(getPixelColor(i)));
        }
        else
        {
          uint8_t red = ((Red(getPixelColor(i)) * (TotalSteps - Index)) + (Red(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t green = ((Green(getPixelColor(i)) * (TotalSteps - Index)) + (Green(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t blue = ((Blue(getPixelColor(i)) * (TotalSteps - Index)) + (Blue(Color(255, 255, 255)) * Index)) / TotalSteps;

          setPixelColor(i, Color(red, green, blue));
        }
        }
      */
      for (uint8_t i = 0; i < Index; i++)
      {
        uint8_t red, green, blue;

        if (i <= numPixels() / 2 )
        {
          red = ((Red(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Red(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
          green = ((Green(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Green(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
          blue = ((Blue(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Blue(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
        }
        else
        {
          uint8_t j = i % (TotalSteps / 2);

          red = ((Red(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Red(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
          green = ((Green(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Green(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
          blue = ((Blue(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Blue(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
        }

        //if not white
        if (Red(getPixelColor(i)) < 50)
        {
          setPixelColor(i, Color(red, green, blue));
        }
        else
        {
          uint8_t r = ((Red(getPixelColor(i)) * (TotalSteps - Index)) + (Red(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t g = ((Green(getPixelColor(i)) * (TotalSteps - Index)) + (Green(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t b = ((Blue(getPixelColor(i)) * (TotalSteps - Index)) + (Blue(Color(255, 255, 255)) * Index)) / TotalSteps;

          setPixelColor(i, Color(r, g, b));
        }
      }

      for (uint8_t i = TotalSteps; i > TotalSteps - Index; i--)
      {
        uint8_t red, green, blue;

        if (i <= numPixels() / 2)
        {
          red = ((Red(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Red(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
          green = ((Green(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Green(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
          blue = ((Blue(Color(0, 0, 0)) * ((TotalSteps / 2) - i)) + (Blue(Color(0, 0, 0)) * i)) / (TotalSteps / 2);
        }
        else
        {
          uint8_t j = i % (TotalSteps / 2);

          red = ((Red(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Red(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
          green = ((Green(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Green(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
          blue = ((Blue(Color(0, 0, 0)) * ((TotalSteps / 2) - j)) + (Blue(Color(0, 0, 0)) * j)) / (TotalSteps / 2);
        }

        //if not white
        if (Red(getPixelColor(i)) < 50)
        {
          setPixelColor(i, Color(red, green, blue));
        }
        else
        {
          uint8_t r = ((Red(getPixelColor(i)) * (TotalSteps - Index)) + (Red(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t g = ((Green(getPixelColor(i)) * (TotalSteps - Index)) + (Green(Color(255, 255, 255)) * Index)) / TotalSteps;
          uint8_t b = ((Blue(getPixelColor(i)) * (TotalSteps - Index)) + (Blue(Color(255, 255, 255)) * Index)) / TotalSteps;

          setPixelColor(i, Color(r, g, b));
        }
      }

      show();
      Increment();
    }

    void FadeOut(uint8_t interval, uint8_t steps)
    {
      ActivePattern = FADE_OUT;
      Interval = interval;
      TotalSteps = steps;
      Index = 0;
    }

    //change to just dim every pixel by 50%
    void FadeOutUpdate()
    {
      /*
        for (int i = 0; i < numPixels(); i++)
        {
          if (getPixelColor(i) != 0)
          {
            uint8_t red = ((Red(getPixelColor(i)) * (TotalSteps - Index)) + (Red(Color(0, 0, 0)) * Index)) / TotalSteps;
            uint8_t green = ((Green(getPixelColor(i)) * (TotalSteps - Index)) + (Green(Color(0, 0, 0)) * Index)) / TotalSteps;
            uint8_t blue = ((Blue(getPixelColor(i)) * (TotalSteps - Index)) + (Blue(Color(0, 0, 0)) * Index)) / TotalSteps;

            setPixelColor(i, Color(red, green, blue));
          }
      */

      for (int i = 0; i < numPixels() / 2; i++)
      {
        setPixelColor(i, Color(0, 0 ,0));
      }

      for (int i = numPixels() / 2; i < numPixels(); i++)
      {
        if (getPixelColor(i) != 0)
          setPixelColor(i, DimColor(getPixelColor(i)));
      }

      show();
      Increment();
    }

    uint8_t DivideColor(uint8_t _color)
    {
      _color = _color - (_color / 8);

      if (_color <= 10)
        _color = 0;

      return _color;
    }

    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
      // Shift R, G and B components one bit to the right
      uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
      return dimColor;
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
      for (int i = 0; i < numPixels(); i++)
      {
        setPixelColor(i, color);
      }
      show();
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
      return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
      return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
      return color & 0xFF;
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
      WheelPos = 255 - WheelPos;
      if (WheelPos < 85)
      {
        return Color(255 - WheelPos * 3, 0, WheelPos * 3);
      }
      else if (WheelPos < 170)
      {
        WheelPos -= 85;
        return Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      else
      {
        WheelPos -= 170;
        return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
      }
    }
};



void StickComplete();

// Define some NeoPatterns for the two rings and the stick
//  as well as some completion routines
NeoPatterns Stick(144, 7, NEO_GRBW + NEO_KHZ800, &StickComplete);

// Initialize everything and prepare to start
void setup()
{
  Serial.begin(115200);

  Stick.begin();

  Wire.begin();

  switch (ACT)
  {
    case 1:
      //Stick.Scanner(Stick.Wheel(15), 60); //~40 is a good delay
      Stick.ColorWipe(Stick.Wheel(15), 60);
      break;

    case 2:
      sunWipeComplete = true;
      //uint32_t color1, uint32_t color2, uint8_t interval, uint32_t color3 = 0, uint16_t steps = 144, direction dir = FORWARD)
      Stick.Gradient(Stick.Wheel(155), Stick.Color(255, 100, 0), 60, Stick.Color(100, 100, 100));

    case 3:
      sunWipeComplete = true;
      Stick.Scanner(Stick.Color(255, 255, 255), 34);
      Stick.Index = Stick.numPixels();
      firstTimeInACT3 = false;
      break;
    case 4:
      sunWipeComplete = true;
      Stick.Scanner(Stick.Wheel(15), 80);
      Stick.Index = random(Stick.numPixels() * 2);
      ACT4FadeOutComplete = false;
      break;

    case 5:
      Stick.Orbit(Stick.Color(0, 0, 255), 20);
      break;

    case 6:
      Stick.Gradient(Stick.Color(100, 100, 100), Stick.Color(0, 0, 255), 80);
      break;

    case 7:
      Stick.Gradient(Stick.Color(0, 0, 255), Stick.Color(100, 100, 150), 80, Stick.Color(0, 0, 255));
      break;
  }
}

void sendMessageToSlaves(byte x)
{
  Wire.beginTransmission(8);
  Wire.write(x);
  Wire.endTransmission();

  Wire.beginTransmission(9);
  Wire.write(x);
  Wire.endTransmission();
}

//michael nyman

// Main loop
void loop()
{
  Stick.Update();
}

//------------------------------------------------------------
//Completion Routines - get called on completion of a pattern
//------------------------------------------------------------

// Stick Completion Callback
void StickComplete()
{
  switch (ACT)
  {
    case 1:
      if (!sunWipeComplete)
      {
        timeSinceACTStart = millis();
        //Stick.Color1 = Stick.Wheel(155);
        sunWipeComplete = true;
        Stick.Scanner(Stick.Wheel(155), 60);
        Serial.println("Sun wipe complete");
      }
      else if (earthIterations < 2)
      {
        earthIterations++;
        if (earthIterations == 1)
        {
          //send a message to slave moon to start
          //1.5x speed of earth
          byte x = 1;
          Wire.beginTransmission(8);
          Wire.write(x);
          Wire.endTransmission();
        }
        else if (earthIterations == 2)
        {
          //send message to slave sun to start
          //0.5x speed of earth
          byte x = 1;
          Wire.beginTransmission(9);
          Wire.write(x);
          Wire.endTransmission();
        }
      }
      break;

    case 2:
      if (ACT2FadeOut)
      {
        Stick.Gradient(Stick.Wheel(155), Stick.Color(255, 100, 0), 60, Stick.Color(100, 100, 100));
        ACT2FadeOut = false;
      }
      else
      {
        Stick.ColorWipe(Stick.Color(0, 0, 0), 60);
        ACT++;
      }
      break;

    case 3:
      if (firstTimeInACT3)
      {
        timeSinceACTStart = millis();

        Stick.Scanner(Stick.Color(255, 255, 255), 34);
        Stick.Index = Stick.numPixels();
        firstTimeInACT3 = false;
      }
      else if (ACT3FadeOut)
      {
        //transition into act 4
        //sun
        Stick.Scanner(Stick.Wheel(15), 80);
        Stick.Index = 5;
        ACT++;
      }
      break;

    case 4:
      if (ACT4FadeOutComplete)
      {
        Stick.Orbit(Stick.Wheel(15), 10);
        ACT++;
      }
      break;

    case 5:
      //      if (ACT5Cycles < 5)
      //      {
      //        ACT5Cycles++;
      //      }
      //      else if (ACT5Cycles == 5)
      //      {
      if (firstTimeInACT5)
      {
        sendMessageToSlaves(5);

        //Stick.Scanner(Stick.Color(0, 0, 0), 80);
        //Stick.TotalSteps = 10;
        /*
                delay(50);
                Stick.FadeOut(50, 10);
                Serial.println("Fading out from Act 5");
                firstTimeInACT5 = false;
                ACT = 99;
        */
        Stick.Gradient(Stick.Color(100, 100, 100), Stick.Color(0, 0, 255), 80);
        ACT = 6;
      }
      //      }
      //fade out complete, start next scene
      //      else
      //      {////////this isn't used now
      //Serial.println("Starting ACT 6");
      //Stick.Gradient(Stick.Color(100, 100, 100), Stick.Color(0, 0, 255), 80);
      //ACT++;
      //      }
      break;

    case 6:
      Stick.Gradient(Stick.Color(0, 0, 255), Stick.Color(100, 100, 150), 80, Stick.Color(0, 0, 255));
      ACT++;
      break;

    case 7:
      Stick.OnlyWhite(60, 144);
      ACT++;
      break;

    case 8:
      Stick.Gradient(Stick.Color(0, 0, 0), Stick.Color(0, 0, 0), 80, Stick.Color(0, 0, 0));
      Stick.Index = 36;
      ACT++;
      break;

    //reset everything and start over
    case 9:
      sunWipeComplete = false;
      earthIterations = 0;
      ACT2FadeOut = true;
      firstTimeInACT3 = true;
      ACT3FadeOut = false;
      ACT4FadeOutComplete = false;
      rotationCounter = 0;
      ACT5Cycles = 0;
      firstTimeInACT5 = true;
      ACT = 1;
      Stick.ColorWipe(Stick.Wheel(15), 60);
      break;

    //transition from 5 to 6
    case 99:
      Serial.println("Starting ACT 6");
      Stick.Gradient(Stick.Color(100, 100, 100), Stick.Color(0, 0, 255), 80);
      ACT = 6;
      break;

    default:
      break;
  }

  //thank you for existing ^^

}
