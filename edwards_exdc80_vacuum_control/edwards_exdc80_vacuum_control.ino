#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_STM32.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

unsigned char mode = 0;
uint8_t pump_set_volt = 50; // 5.0 volt
bool active = false;
#define MODES 3

void setup()   {                
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  display.clearDisplay();

  pinMode(PA0, INPUT);
  pinMode(PA1, INPUT);

  pinMode(PA7, OUTPUT);

  pinMode(PB12, OUTPUT);   
  digitalWrite(PB12, LOW);

  pinMode(PA8, INPUT_PULLUP);           // set pin to input
  pinMode(PA9, INPUT_PULLUP);           // set pin to input
  pinMode(PA10, INPUT_PULLUP);           // set pin to input
  pinMode(PA11, INPUT_PULLUP);           // set pin to input
  
  // text display tests
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Ver: ");
  display.println(1.00);
  display.display();
  delay(1000);
  display.clearDisplay();

  mode = EEPROM.read(0);
  if(mode < 0) mode = 0;
  mode = mode % MODES;
  pump_set_volt = EEPROM.read(1);
  if(pump_set_volt > 100)
    pump_set_volt = 50;

  display.setCursor(0,0);
  display.println("EEPROM:");
  for(int i = 0; i < 4; i++)
    display.print(EEPROM.read(i) & 0xFF, HEX);
  display.println("");    
  for(int i = 4; i < 8; i++)
    display.print(EEPROM.read(i) & 0xFF, HEX);
  display.display();
  delay(2000);
  display.clearDisplay();
}

#define B0_PIN PA8
#define B1_PIN PA9
#define B2_PIN PA10
#define SW1_PIN PA11

#define SW1 1
#define B0 2
#define B1 4
#define B2 8

#define BTN(B) (1 - digitalRead(B))
#define SW(B) (digitalRead(B))

int last_btns, last_time;

int get_buttons()
{
  int btns = 0;
  //btns |= (SW(SW1_PIN) ? SW1 : 0);
  btns |= (BTN(B0_PIN) ? B0 : 0);
  btns |= (BTN(B1_PIN) ? B1 : 0);
  btns |= (BTN(B2_PIN) ? B2 : 0);
  return btns;
}

int get_switches()
{
  int btns = 0;
  btns |= (SW(SW1_PIN) ? SW1 : 0);
  return btns;
}

bool pump_run = false;
bool pump_act = false;

void set_row(int row)
{
  display.setCursor(0,0);
  for(int i = 0; i < row; i++)
    display.println("");
}

float pump_volt;
void loop() {
  float pump_volt1 = analogRead(PA1) * 4.1 * 0.852564 / 1000.0;
  pump_volt = 0.92 * pump_volt + 0.08 * pump_volt1;

  if(pump_run)
  {
    float pump_set_volt_float = pump_set_volt / 10.0;
    if(pump_volt > pump_set_volt_float)
      pump_act = false;
    
    float pump_start_volt = pump_set_volt_float - 0.4;
    if(pump_start_volt <= 0.05)
      pump_start_volt = 0.05;
    if(pump_volt < pump_start_volt)
        pump_act = true;
  } else
  {
    pump_act = false;
  }

  digitalWrite(PA7, pump_act ? 1 : 0);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  int btns = get_buttons();
  int sws = get_switches();

  pump_run = sws & SW1;

  int change = 0;
  if((btns & B0) && millis() - last_time  > 1000 && !active)
  {
    display.println("SAVING");
    display.println("EEPROM");
    display.display();
    EEPROM.update(0, mode);
    EEPROM.update(1, pump_set_volt);
    delay(1500);
    btns = get_buttons();
    while(btns & B0)
    {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("RELEASE");
      display.display();
      delay(100);
      btns = get_buttons();
    }
    last_btns = 0;
    last_time = millis();
    return;
  }
  if(btns == 0 && last_btns != 0)
  {
    if(last_btns & B0)
    {
      active = !active;
    }
    if(last_btns & B1)
    {
      if(!active) 
        mode = (mode + 1 + MODES) % MODES;
      else
        change = 1;
    }
    if(last_btns & B2)
    {
      if(!active) 
        mode = (mode - 1 + MODES) % MODES;
      else
        change = -1;
    }
  }

#define ACTB() \
  if(active)\
    display.setTextColor(BLACK, WHITE); // 'inverted' text
#define ACTE()\
  if(active)\
    display.setTextColor(WHITE);

  ACTB();
  display.print("M");
  display.print(mode);
  ACTE(); 
  display.print(" ");
  
  //if(mode == 0)
  {
    display.print("B:");
    display.print((btns & B0 ) ? 1 : 0);
    display.print((btns & B1 ) ? 1 : 0);
    display.print((btns & B2 ) ? 1 : 0);
    display.print((sws & SW1) ? 1 : 0);
    display.println("");
  }
  if(mode == 0)
  {  
    display.print("PWR: ");
    display.println(analogRead(PA0) * 11 / 1000.0);
    display.print("PMP: ");
    display.println(pump_volt);
  }

  if(mode == 1)
  {
    ACTB();
    display.println("Pump Set");
    display.println(pump_set_volt / 10.0);
    ACTE();
    pump_set_volt += change * 2;
  }

  //if(mode == 0)
  {
    set_row(3);
    display.print("PMP:");
    //ACTB();
    display.print(pump_run);
    //ACTE();
    display.print(pump_act ? " ACT" : "");
    //pump_run ^= change ? 1 : 0;
  }

  if(btns != last_btns)
  {
    last_btns = btns;
    last_time = millis();
  }
  if(btns == 0)
  {
    last_time = millis();
  }
  
  display.display();
  //delay(200);
}
