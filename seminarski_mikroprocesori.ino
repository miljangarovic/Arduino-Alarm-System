#include <SoftwareSerial.h>
#include <Wire.h>
#include <IRremote.h>

#define LED 2
#define LED_RED 5
#define LED_GREEN 6
#define BUZZER 4
#define PIR 3
#define BUTTON 11
#define READY  13
#define RESET 12
#define ATTEMPTS 3
#define CHECK_PERIOD 30000
#define LED_SIGNAL_PERIOD  300


const int RECV_PIN = 10;
IRrecv irrecv(RECV_PIN);
decode_results results;

long int x[4] = {0, 0, 0, 0};
int i = 0;
bool turn_on = false;
bool turn_off = false;

SoftwareSerial modemSerial(7, 8);      // RX, TX for ESP8266

int pir_val, pir_state = 0, foto_val;
int button_val, button_val_old = 0, button_state = 0;


void(* resetFunc) (void) = 0;//declare reset function at address 0

bool DEBUG = true;   //show more logs
int responseTime = 10000; //communication timeout

unsigned long myTime;
String responses;
String channel;
String telNum = "+38267324363";
String tel_num = "";
String poruka = "";
String poruka_temp = "";
int indexInStr;
String odgovor = "";
int change = 0;
unsigned long pmillis, pled;
int pn = 0;
int sms_send = 1;
int button_first_time = 1;


void SetUpShield()
{
  if (sendToModem("AT", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
  if (sendToModem("ATE1", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
  if (sendToModem("AT+CREG?", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
  if (sendToModem("AT+CMGF=1", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
  if (sendToModem("AT+CNMI=2,2,0,0,0", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
}

boolean find(String string, String value) {
  if (string.indexOf(value) >= 0)
    return true;
  return false;
}

void printingRemoteControllerAction() {
  for (int j = 0; j < 4; j++) {
    if (x[j] == 16724175)Serial.print("1 ");
    else if (x[j] == 16718055) Serial.print("2 ");
    else if (x[j] == 16743045) Serial.print("3 ");
    else if (x[j] == 16716015) Serial.print("4 ");
    else if (x[j] == 16726215) Serial.print("5 ");
    else if (x[j] == 16734885) Serial.print("6 ");
    else if (x[j] == 16728765) Serial.print("7 ");
    else if (x[j] == 16730805) Serial.print("8 ");
    else if (x[j] == 16732845) Serial.print("9 ");
    else if (x[j] == 0) Serial.print("0 ");
  }
  Serial.println();
}

int sendToModem(String command, const int timeout, int Attempts, boolean debug, String rsp, String response) {
  response = "";
  int rez = 1;
  int atmp = 0;
  while (atmp < Attempts)
  {
    if (debug)Serial.println(command);
    modemSerial.println(command); // send the read character to the SIM900
    long int time = millis();
    while ( (time + timeout) > millis())
    {
      if (modemSerial.available())
      {
        // The esp has data so display its output to the serial window
        char c = modemSerial.read(); // read the next character.
        response += c;
      }
      if (rsp != "")
      {
        if (find(response, rsp)) {
          rez = 0;
          break;
        }
      }
      else {
        rez = 0;
        break;
      }
    }
    if (debug)
    {
      Serial.println(response); Serial.println(rez);
    }
    if (!rez)break;
    atmp++;
  }
  return rez;
}

void SendSMS(String Number)
{


  String thData = "Neko je u prostoriji!";
  thData += (char)26; // ASCII kod od CTRL+Z
  Serial.println(thData);
  // SMS mesage sending
  if (sendToModem("AT+CMGS=\"" + Number + "\"\r\n", responseTime, ATTEMPTS, DEBUG, ">", responses) != 0) {
    digitalWrite(RESET, LOW);  // broj na koji šaljemo
    resetFunc();
  }
  modemSerial.println(thData); // SMS tekst koji zelimo da posaljemo
  if (sendToModem("", responseTime, ATTEMPTS, DEBUG, "OK", responses) != 0) {
    digitalWrite(RESET, LOW);
    resetFunc();
  }
}

void remoteController() {
  if (irrecv.decode(&results)) {
    x[i++] = results.value;
    if (i == 4) {
      if (x[0] == 16724175 && x[1] == 16718055 && x[2] == 16743045 && x[3] == 16716015) {
        // password : 1234
        printingRemoteControllerAction();
        Serial.println("Ispravan password! Alarm uključen! ");
        turn_on = true;
      }
      else if (x[0] == 16716015 && x[1] == 16743045 && x[2] == 16718055  && x[3] == 16724175) {
        // password: 4321
        printingRemoteControllerAction();
        turn_off = true;
        Serial.println("Ispravan password! Alarm isključen!");
      }
      else {
        printingRemoteControllerAction();
        Serial.println("Neispravan password! ");
      }
      i = 0;
      x[0] = 0;
      x[1] = 0;
      x[2] = 0;
      x[3] = 0;

    }
    printingRemoteControllerAction();
    myTime = millis();
    while (millis() - myTime < 600) {}
    irrecv.resume();
  }
}


void setup()
{
  irrecv.enableIRIn();
  irrecv.blink13(true);
  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PIR, INPUT);;
  pinMode(BUTTON, INPUT);
  Serial.begin(9600);

  //Inicijalizacija pinova Arduino UNO razvojne ploce
  pinMode(RESET, OUTPUT);
  pinMode(READY, OUTPUT);
  digitalWrite(RESET, HIGH);
  digitalWrite(READY, LOW);

  // Open serial communications
  modemSerial.begin(9600);

  //Inicijalizacija modema
  Serial.println("Start!");
  SetUpShield();

  //Ciscenje buffera hardverskog seriala
  while (Serial.available() > 0) // Provjerava da li postoje podaci koji dolaze preko serijskog porta
    Serial.write(Serial.read());

  //Ciscenje buffera softverskog seriala
  while (modemSerial.available() > 0) // Provjerava da li postoje podaci koji dolaze preko serijskog porta
    Serial.write(modemSerial.read());

  //Signalizacija spremnosti uredjaja
  digitalWrite(READY, HIGH);
  Serial.println("Connection is running!");


}

void buttonClicked() {
  button_val = digitalRead(BUTTON);
  if (button_val == HIGH && button_val_old == LOW) {
    button_state = 1 - button_state;
    delay(10);
  }
  button_val_old = button_val;
}


void getMessageFromSMS() {

  while (modemSerial.available() > 0) {

    odgovor += char(modemSerial.read());

    delay(1);
    change = 1;
    tel_num = "";
    poruka = "";
  }
  if (change == 1) {

    for (int i = 9; i <= 20 ; i++) {
      tel_num += odgovor[i];
    }

    for (int i = 50; i <= 53 ; i++) {
      poruka += odgovor[i];
    }

    Serial.println(poruka);
    //    Serial.println(odgovor);
    odgovor = "";
    change = 0;
    poruka_temp = poruka;
  }

}

void loop()
{
  //  getMessageFromSMS();
  // citanje sa senzora
  foto_val = analogRead(A0);
  pir_val = digitalRead(PIR);
  analogWrite(LED_GREEN, 255);
  //  button_val = digitalRead(BUTTON);
  //  if (button_val == HIGH && button_val_old == LOW) {
  //    button_state = 1 - button_state;
  //    delay(10);
  //  }
  //  button_val_old = button_val;
  remoteController();
  buttonClicked();

  while (button_state == 1 || turn_on == true) {
    remoteController();
    if (turn_off == true) {
      turn_on = false;
      turn_off = false;
      pir_state = 0;
      button_state = 0;
      button_first_time = 1;
      break;
    }
    analogWrite(LED_GREEN, 0);
    myTime = millis();
    if (button_first_time == 1) {
      digitalWrite(LED, HIGH);
      while (millis() - myTime < 10000) {}
      button_first_time = 0;
    }

    pir_val = digitalRead(PIR);
    if (pir_val == 1) {
      pir_state = 1;
    }

    if (sms_send == 0 ) {
      getMessageFromSMS();
    }


    if (pir_state == 1) {
      if (sms_send == 1) {
        SendSMS(telNum);
        sms_send = 0;
      }

      analogWrite(LED_RED, 255);
      tone(BUZZER, 1000);
      myTime = millis();
      while (millis() - myTime < 500) {}
      analogWrite(LED_RED, 0);
      noTone(BUZZER);
      myTime = millis();
      while (millis() - myTime < 500) {}
    }
    else {
      digitalWrite(LED, HIGH);
      myTime = millis();
      while (millis() - myTime < 500) {}
      digitalWrite(LED, LOW);
      myTime = millis();
      while (millis() - myTime < 500) {}
    }
    if (poruka == "Stop") {
      Serial.println("Uspjesno poslato");
      poruka = "";
      pir_state = 0;
      button_state = 0;
      turn_off = false;
      turn_on = false;
      button_first_time = 1;
      break;
    }
    //
  }
  sms_send = 1;


  // alarm
  //  if (pir_state == 1) {
  //    Serial.println(foto_val);
  //    while (!button_state) {
  //      // svijetlo
  //      if (foto_val >= 500 && foto_val <= 680) {
  //        digitalWrite(LED, HIGH);
  //      }
  //      // sirena
  //      tone(BUZZER, 1000);
  //      delay(500);
  //      noTone(BUZZER);
  //      delay(500);
  //      // blokada
  //      button_val = digitalRead(BUTTON);
  //
  //    }
  //    pir_state = 0;
  //    button_state = 0;
  //  }
  /* */
  if (Serial.available() > 0) {
    switch (Serial.read())
    {
      case 's': // Ako je ulaz 's' program ce pozvati f-ju za slanje SMS sa GSM modema
        SendSMS(telNum);
        while (Serial.available() > 0) // Provjerava da li postoje podaci pristigli preko serijskog porta
          Serial.write(Serial.read());
        break;
    }
  }

}
/*

  // Provjera da li postoje podaci koji dolaze preko softverskog serijala i prhihvat istih
  while (modemSerial.available() > 0) {
  //    Serial.write(modemSerial.read());
  odgovor += char(modemSerial.read());
  delay(1);
  change = 1;
  tel_num="";
  poruka="";
  }
  if (change == 1) {
  for (int i = 9; i <= 20 ; i++) {
    tel_num += odgovor[i];
  }
  for (int i = 50; i <=odgovor.length()-3 ; i++) {
    poruka+=odgovor[i];
  }
  odgovor="";
  change=0;
  poruka_temp=poruka;
  }


  if(poruka=="TempVlaz"){
  Serial.println("Uspjesno poslato");
  SendTempHumidSMS(tel_num);
  poruka="";
  }

  delay(1);
*/


void sendData(String chn, String str) {
  String len = "";
  len += str.length();
  sendToModem("AT+CIPSEND=" + chn + "," + len, responseTime, ATTEMPTS, DEBUG, "OK", responses);
  delay(100);
  sendToModem(str, responseTime, ATTEMPTS, DEBUG, "OK", responses);
  delay(100);
  //sendToModem("AT+CIPCLOSE=5", responseTime, DEBUG);
}



String  readSerialMessage() {
  char value[100];
  int index_count = 0;
  while (Serial.available() > 0) {
    value[index_count] = Serial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
    delay(2);
  }
  String str(value);
  str.trim();
  return str;
}

String  readmodemSerialMessage() {
  char value[100];
  int index_count = 0;
  while (modemSerial.available() > 0) {
    value[index_count] = modemSerial.read();
    index_count++;
    value[index_count] = 'null'; // Null terminate the string
    if (value[index_count - 1] == '#')break;
    delay(2);
  }
  String str(value);
  str.trim();
  return str;
}



String sendToUno(String command, const int timeout, boolean debug) {
  String response = "";
  Serial.println(command); // send the read character to the esp8266
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (Serial.available())
    {
      // The esp has data so display its output to the serial window
      char c = Serial.read(); // read the next character.
      response += c;
      delay(2);
    }
    if (response != "")break;
  }
  if (debug)
  {
    Serial.println(response);
  }
  return response;
}
