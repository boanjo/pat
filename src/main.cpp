#include <Arduino.h>
#include <avr/sleep.h>

#include "Adafruit_FONA.h"

#define FONA_VIO 11
#define FONA_RST 6
#define FONA_KEY 7
#define FONA_PS 8

#include <SoftwareSerial.h>

#define FONA_RX 12
#define FONA_TX 10

#define SWITCH_PIN 2

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
void wake()
{
  // cancel sleep as a precaution
  sleep_disable();
  // precautionary while we do other stuff
  detachInterrupt(0);
} // end of wake

void setup()
{

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(SWITCH_PIN, HIGH); // enable pull-up

  // Vio is used for the logic leevel. We cheat an power it from the digital
  // pin when needed (i.e. low for now)
  pinMode(FONA_VIO, OUTPUT);
  digitalWrite(FONA_VIO, LOW);

  pinMode(FONA_KEY, OUTPUT);
  digitalWrite(FONA_KEY, HIGH);

  pinMode(FONA_PS, INPUT);
}

void loop()
{
  // disable ADC
  ADCSRA = 0;

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Do not interrupt before we go to sleep, or the
  // ISR will detach interrupts and we won't wake.
  noInterrupts();

  // will be called when pin D2 goes low
  attachInterrupt(0, wake, FALLING);
  EIFR = bit(INTF0); // clear flag for interrupt 0

  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit(BODS) | bit(BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit(BODS);

  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts(); // one cycle
  sleep_cpu();  // one cycle

  // Here we are in sleep mode waiting forever until the switch is triggered
  Serial.println("Wakeeup");
  digitalWrite(LED_BUILTIN, HIGH);
  if (digitalRead(FONA_PS) == LOW)
  {
    digitalWrite(FONA_VIO, HIGH);
    digitalWrite(FONA_KEY, LOW);
    delay(2000);
    digitalWrite(FONA_KEY, HIGH);
    delay(200);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  delay(2000);

  Serial.println("Starting serial");
  fonaSerial->begin(4800);
  delay(100);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println("FONA not found");
  }

  while (fona.getNetworkStatus() != 1)
  {
    delay(100);
  }
  Serial.println("Network found");

  digitalWrite(LED_BUILTIN, LOW);

  char message[40] = "Posten! Bat=";
  uint16_t vbat = 0;
  char bat_str[5] = "?";
  if (fona.getBattVoltage(&vbat))
  {
    Serial.println(vbat);
    sprintf(bat_str, "%d", vbat);
  }

  strcat(message, bat_str);
  strcat(message, "mV");

  char toNumber[20] = "0730435567";
  if (!fona.sendSMS(toNumber, message))
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("SMS Failed");
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("SMS Sent");
  }

  // Turn FONA off
  while (digitalRead(FONA_PS) == HIGH)
  {

    digitalWrite(FONA_KEY, LOW);
    delay(2000);
    digitalWrite(FONA_KEY, HIGH);

    digitalWrite(FONA_VIO, LOW);
    delay(200);
  }
  digitalWrite(LED_BUILTIN, LOW);
}