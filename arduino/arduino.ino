//---------- Include section ----------
#include <Servo.h>
#include <SoftwareSerial.h>

//---------- Define section ----------
#define SSID "_ssid_"
#define PASS "_pass_"
#define PORT "_port_"
#define DEBUG true
#define SENSOR_ID "_sensor_id_"
#define VALID_STATE "_state_"
#define GREEN "green"
#define RED "red"

//---------- Const section ----------
const int GREEN_LED_PIN  = 5;
const int RED_LED_PIN = 4;
const int CLOSE_BOX_PIN = 2;
const int SERVO_PIN = 3;
const int ESP_RX = 11;
const int ESP_TX = 10;

//---------- Init section ----------
Servo lock_servo;
SoftwareSerial esp8266(ESP_TX, ESP_RX);

//---------- Variable section ----------
int initial_position = 120;
int current_position = 0;
int close_box_button_state = 0;
boolean box_open = false;

//---------- Main function section ----------
void setup() {
  boolean connection_established = false;
  boolean server_setup = false;

  Serial.begin(9600);
  esp8266.begin(9600);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(CLOSE_BOX_PIN, INPUT);
  lock_servo.attach(SERVO_PIN);
  
  current_position = open_box(initial_position);
  light_up_led(GREEN);

  initialize();
  while (!connection_established) {
    connection_established = connectToNetwork();
  }
  
  while(!server_setup) {
    server_setup = startServer();
  }
  
  sendData("AT+CIFSR", 2000, DEBUG);

  // turn green and keep for 5 seconds
  
  current_position = close_box(current_position);
  light_up_led(RED);
}

void loop () {
  String response_message;
  boolean is_valid_token = false;
  
  response_message = "";
  close_box_button_state = digitalRead(CLOSE_BOX_PIN);

  long int time = millis();
  char receive_char;
  while ((time+10000) > millis()) {
    while (esp8266.available()) {
      receive_char = esp8266.read();
      response_message += receive_char;
    }
  }

  if (response_message != "") {  
    response_message = process_message(response_message);
  
    int index = response_message.indexOf(",");
    String sensor_id = response_message.substring(0, index);
    String state_value = response_message.substring(index + 1, response_message.length());
  
    if (sensor_id == SENSOR_ID && state_value == VALID_STATE) {
      is_valid_token = true;
      } else {
      is_valid_token = false;
    }
  }

  if (DEBUG) {
    Serial.print("is_valid_token = ");
    Serial.print(is_valid_token);
  }
  if (is_valid_token) {
     current_position = open_box(current_position);
     light_up_led(GREEN);
     delay(10000);
  }
  
  while (is_valid_token) {
    close_box_button_state = digitalRead(CLOSE_BOX_PIN);
    if (DEBUG) {
      Serial.print("is_valid_token. While loop value : ");
      Serial.println(close_box_button_state);
    }
    if (!close_box_button_state) {
      is_valid_token = false;
    }
  }
  
  current_position = close_box(current_position);
  light_up_led(RED);
}

//---------- led & button function section ----------
int open_box(int current_pos) {
  if (current_pos < 180) {
    for (int pos = initial_position; pos <=180; pos +=1) {
      lock_servo.write(pos);
      delay(15); // wait for servo to get to position
    }
  }
  box_open = true;
  
  return 180;
}

int close_box(int current_pos) {
  if (current_pos > initial_position) {
    for (int pos = 180; pos >=initial_position; pos -=1) {
      lock_servo.write(pos);
      delay(15); // wait for servo to get to position
    }
  }
  box_open = false;
  
  return initial_position;
}

void light_up_led (String led_color) {  
  if (led_color == GREEN) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  } else if (led_color == RED) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);    
  }
  else {
    Serial.print("wrong led color: ");
    Serial.println(led_color);
  }
}

//---------- ESP function section ----------
void initialize() {
  sendData("AT", 2000, DEBUG);
  sendData("AT+CIPSERVER=0", 5000, DEBUG);
  sendData("AT+CIPCLOSE=5", 5000, DEBUG);
  sendData("AT+CWMODE=1", 2000, DEBUG);
  sendData("AT+CIPMUX=1", 2000, DEBUG);
}

boolean startServer() {
  String response_message = "";
  String cmd = "";

  cmd += "AT+CIPSERVER=1,";
  cmd += PORT;
  response_message = sendData(cmd, 5000, DEBUG);

  if (response_message.indexOf("OK") > 0) {
    return true;
  } else {
    return false;
  }
}

boolean connectToNetwork(){
  String response_message = "";
  
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",\"";
  cmd += PASS;
  cmd += "\"";
  
  response_message = sendData(cmd, 20000, DEBUG);  
  if (response_message.indexOf("OK") > 0) {
    return true;
  } else {
    return false;
  }
}

String sendData(String command, const int timeout, boolean debug)
{
    String response_message = "";
    long int time = millis();
    char receive_char;

    esp8266.println(command);
    
    while( (time+timeout) > millis()) {
      while(esp8266.available()) {    
        receive_char = esp8266.read();
        response_message += receive_char;
      }
    }
      
    if (debug) {
      Serial.println(response_message);
    }

    return response_message;
}

//---------- Message process function section ----------
String process_message (String input_message) {  
  if (DEBUG) {
    Serial.println("Original message: ");
    Serial.println(input_message);
    Serial.println("----------");
  }

  int json_index_begin = input_message.lastIndexOf("{");
  int json_index_end = input_message.lastIndexOf("}");
  input_message = input_message.substring(json_index_begin, json_index_end);
  
  input_message.replace(" ", "");
  input_message.replace("\"", "");
  input_message.replace("{", "");
  input_message.replace("}", "");
  input_message.replace("sensor_id:", "");
  input_message.replace("state:", "");

  if (DEBUG) {
    Serial.print("preocessed message is: ");
    Serial.println(input_message);
    Serial.println("----------");
  }
  
  return input_message;
}
