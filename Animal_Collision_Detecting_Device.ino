#include "application.h"

/*
#define  D2
#define BUZZERPIN D1
*/

#define _CL_RED             0
#define _CL_GREEN           1
#define _CL_BLUE            2
#define _CLK_PULSE_DELAY    20

class ChainableLED
{
public:
    ChainableLED(byte clk_pin, byte data_pin, byte number_of_leds);
    ~ChainableLED();
    
    void setColorRGB(byte led, byte red, byte green, byte blue);
    void setColorHSB(byte led, float hue, float saturation, float brightness);

private:
    byte _clk_pin;
    byte _data_pin;
    byte _num_leds; 

    byte* _led_state;
    
    float myconstrain(float in, float min, float max);
    void clk(void);
    void sendByte(byte b);
    void sendColor(byte red, byte green, byte blue);
};

float hue2rgb(float p, float q, float t);

// --------------------------------------------------------------------------------------

ChainableLED::ChainableLED(byte clk_pin, byte data_pin, byte number_of_leds) :
    _clk_pin(clk_pin), _data_pin(data_pin), _num_leds(number_of_leds)
{
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
  
    _led_state = (byte*) malloc(_num_leds*3);

    for (byte i=0; i<_num_leds; i++)
        setColorRGB(i, 0, 0, 0);
}

ChainableLED::~ChainableLED()
{
    free(_led_state);
}

float ChainableLED::myconstrain(float in, float min, float max)
{
   if(in > max) in = max;
   if(in < min) in = min;
   return in;
}

// --------------------------------------------------------------------------------------

void ChainableLED::clk(void)
{
    digitalWrite(_clk_pin, LOW);
    delayMicroseconds(_CLK_PULSE_DELAY); 
    digitalWrite(_clk_pin, HIGH);
    delayMicroseconds(_CLK_PULSE_DELAY);   
}

void ChainableLED::sendByte(byte b)
{
    // Send one bit at a time, starting with the MSB
    for (byte i=0; i<8; i++)
    {
        // If MSB is 1, write one and clock it, else write 0 and clock
        if ((b & 0x80) != 0)
            digitalWrite(_data_pin, HIGH);
        else
            digitalWrite(_data_pin, LOW);
        clk();

        // Advance to the next bit to send
        b <<= 1;
    }
}
 
void ChainableLED::sendColor(byte red, byte green, byte blue)
{
    // Start by sending a byte with the format "1 1 /B7 /B6 /G7 /G6 /R7 /R6"
    byte prefix = (1<<6) | (1<<7);
    if ((blue & 0x80) == 0)     prefix|= (1<<5);
    if ((blue & 0x40) == 0)     prefix|= (1<<4); 
    if ((green & 0x80) == 0)    prefix|= (1<<3);
    if ((green & 0x40) == 0)    prefix|= (1<<2);
    if ((red & 0x80) == 0)      prefix|= (1<<1);
    if ((red & 0x40) == 0)      prefix|= (1<<0);
    sendByte(prefix);
        
    // Now must send the 3 colors
    sendByte(blue);
    sendByte(green);
    sendByte(red);
}

void ChainableLED::setColorRGB(byte led, byte red, byte green, byte blue)
{
    // Send data frame prefix (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    
    // Send color data for each one of the leds
    for (byte i=0; i<_num_leds; i++)
    {
        if (i == led)
        {
            _led_state[i*3 + _CL_RED] = red;
            _led_state[i*3 + _CL_GREEN] = green;
            _led_state[i*3 + _CL_BLUE] = blue;
        }
                    
        sendColor(_led_state[i*3 + _CL_RED], 
                  _led_state[i*3 + _CL_GREEN], 
                  _led_state[i*3 + _CL_BLUE]);
    }

    // Terminate data frame (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
}

void ChainableLED::setColorHSB(byte led, float hue, float saturation, float brightness)
{
    float r, g, b;
    
    myconstrain(hue, 0.0, 1.0);
    myconstrain(saturation, 0.0, 1.0);
    myconstrain(brightness, 0.0, 1.0);

    if(saturation == 0.0)
    {
        r = g = b = brightness;
    }
    else
    {
        float q = brightness < 0.5 ? 
            brightness * (1.0 + saturation) : brightness + saturation - brightness * saturation;
        float p = 2.0 * brightness - q;
        r = hue2rgb(p, q, hue + 1.0/3.0);
        g = hue2rgb(p, q, hue);
        b = hue2rgb(p, q, hue - 1.0/3.0);
    }

    setColorRGB(led, (byte)(255.0*r), (byte)(255.0*g), (byte)(255.0*b));
}

// --------------------------------------------------------------------------------------

float hue2rgb(float p, float q, float t)
{
    if (t < 0.0) 
        t += 1.0;
    if(t > 1.0) 
        t -= 1.0;
    if(t < 1.0/6.0) 
        return p + (q - p) * 6.0 * t;
    if(t < 1.0/2.0) 
        return q;
    if(t < 2.0/3.0) 
        return p + (q - p) * (2.0/3.0 - t) * 6.0;

    return p;
}

///////////////////////////////////////////////////////////////////////////////
ChainableLED leds(D2, D3, 1);
int melody[] = {2551,2551,2551,2551,2551,0,2551,2551};
int noteDurations[] = {4,8,8,4,4,4,4,4};

void setup() {
   //pinMode(BUTTONPIN, INPUT);
   Serial.begin(9600);
   leds.setColorRGB(0,0,0,0);
   pinMode(D7, OUTPUT);
   pinMode(D6, OUTPUT);
}

void loop() {
    if (analogRead(A0) >= 2100){
       leds.setColorRGB(0,0,255,0);
    }
    else if(analogRead(A0) < 2100 && analogRead(A0) > 1600){
        leds.setColorRGB(0,255,255,0);
    }
    else if(analogRead(A0) <= 1600 && analogRead(A0) > 1000){
        leds.setColorRGB(0,255,0,0);
    }
    else{
        for(int thisNote = 0; thisNote < 8; thisNote++){
            int noteDuration = 1000/noteDurations[thisNote];
            tone(D1, melody[thisNote], noteDuration);
            int pauseBetweenNotes = noteDuration * 1.3;
            delay(pauseBetweenNotes);
            noTone(D1);
        }
        for(int i=0; i<=50; i++){
            leds.setColorRGB(0,255,0,0);
            delay(30);
            leds.setColorRGB(0,0,0,0);
            delay(30);
        }
    }
}