#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
//#include <b64.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <esp_wpa2.h>
#include <ArduinoJson.h>
#include "Secrets.h"
#include <Keypad.h>
#include <Arduino.h>
#include <vector>
#include <map>

#define RST_PIN         15
#define SS_PIN          5
#define LED_BUILTIN     2
#define GreenLED        4
#define RedLED          26

// MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Variable to store the UID
String uid = "";
String StudentNo;
String StaffNo;

#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET      -1 // Reset pin

#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM]      = {13, 12, 14, 27}; // GPIO19, GPIO18, GPIO5, GPIO17 connect to the row pins
byte pin_column[COLUMN_NUM] = {3, 25, 33, 32};   // GPIO16, GPIO4, GPIO0, GPIO2 connect to the column pins

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

int buttonState = 0;

String arr[5] = {"Option 1", "Option 2", "Option 3", "Option 4", "Option 5"};
int timeValues[7] = {0, 1, 2, 3, 4, 5, 6};

int itemCount = sizeof(arr) / sizeof(arr[0]);

String menuArr[3] = {"Add Student", "Add Staff Member", "Start Lecture"};
int menuCount = sizeof(menuArr) / sizeof(menuArr[0]);

std::vector<String> lectureList;
std::map<String, String> lectureMap;
std::map<String, String> lectureStartTimeMap;  // Map to store start time keyed by display text
std::map<String, String> lectureFinishTimeMap;

// Buffer for storing received QR code data
String qrCodeData = "";
 
int currentItemIndex = 0;
int num = 0;
bool isModuleSelected = false;
bool inMenu = false;
bool readCard = false;
bool navMode = false;
bool isScanLec = false;
bool isSelectLec = false;
bool isDisplayLec = false;
bool isScanOut = false;
bool isBack = false;

int itemsPerPage = 5; 
int MenuItemsPerPage = 4; 
int currentPage = 0;   

String selectedModule = ""; 
int selectedTime = 0; 

String studentNumber = "";

String selectedStartTime;
String selectedFinishTime;

void setup()
{
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  digitalWrite(GreenLED, LOW);
  digitalWrite(RedLED, LOW);
  
  // Initialize SPI bus and MFRC522
  SPI.begin();
  mfrc522.PCD_Init();

  // Connect to WiFi
  connectToWiFi();

  //String modules = GetModules(); // Get the modules as a String
  //char *charModules = new char[modules.length() + 1]; // Allocate a new char array
  //strcpy(charModules, modules.c_str()); 
  
 /* for(int i = 0; i < 5;i++){
    arr[i] = getModuleAt(charModules, i);
  }*/

  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.display();
}

void displayMenu() {
  bool cancel = false;

  display.clearDisplay();
  display.setCursor(0,0);

  int startItem = currentPage * itemsPerPage;
  int endItem = startItem + itemsPerPage;

  if (endItem > itemCount){
    endItem = itemCount;
  } 
   
while(!cancel)
{
display.clearDisplay();
  display.setCursor(0,0);

for (int i = startItem; i < endItem; i++) {
    if (i == currentItemIndex) {
      display.print(">");
    } else {
      display.print(" ");
    }
    display.println(arr[i]);
  }

  char key = getKeyPress();

  if (key == '8') {
    currentItemIndex++;

    if (currentItemIndex >= itemCount) {
      currentItemIndex = 0;
    }

    if (currentItemIndex >= (currentPage + 1) * itemsPerPage) {
      currentPage++;
    }
    
    if (currentItemIndex < currentPage * itemsPerPage) {
      currentPage--;
    }
  }

  if(key == '2')
  {
   currentItemIndex--;

    if (currentItemIndex < 0) {
      currentItemIndex = itemCount - 1;
    }

    if (currentItemIndex < currentPage * itemsPerPage) {
      currentPage--;
    }

    if (currentItemIndex >= (currentPage + 1) * itemsPerPage) {
      currentPage++;
    }
  }
  if(key == '5')
  {
     display.clearDisplay();
     display.setCursor(0,10);
     isModuleSelected = true;
     delay(100);
     display.display();
     Serial.println(arr[currentItemIndex]);

     displayTime(); 
     return;
  }
  if (key == 'C') {
        display.clearDisplay();
        delay(100);
        isModuleSelected = false;
        cancel = true;
      }
  display.display();
}
}

char getKeyPress() {
   static unsigned long lastPressTime = 0;
   char key = keypad.getKey();
   if (key) {
      if (millis() - lastPressTime > 200) { // Debounce delay (200ms)
         lastPressTime = millis();
         Serial.println(key);
         return key;
      }
   }
   return '\0';  // Return null char if no valid key press
}

String PostNewStudent() {
  char key = getKeyPress();
  delay(50);

  HTTPClient http;
  String response = "";

  String url = String(ProdApiUrl) + "/Add";
  http.begin(url);

  String jsonPayload = "{\"UserId\":\"" + uid + "\",\"StudentNo\":\"" + "ST" + StudentNo + "\"}";
  Serial.println(jsonPayload);

  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    digitalWrite(GreenLED, HIGH);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    response = http.getString();
    Serial.println(response);
    delay(2000);
    digitalWrite(GreenLED, LOW);

  } else {
    digitalWrite(RedLED, HIGH);
    Serial.print("Error in HTTP POST request: ");
    Serial.println(httpResponseCode);
    delay(2000);
    digitalWrite(RedLED, LOW);
  }

  http.end();  

  return response; 
}

String PostNewStaff() {
  char key = getKeyPress();
  delay(50);

  HTTPClient http;
  String response = "";

  String url = String(ProdApiUrl) + "/Staff" + "/Add";
  http.begin(url);

  String jsonPayload = "{\"UserId\":\"" + uid + "\",\"StaffNo\":\"" + StaffNo + "\"}";
  Serial.println(jsonPayload);

  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    digitalWrite(GreenLED, HIGH);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    response = http.getString();
    Serial.println(response);
    delay(2000);
    digitalWrite(GreenLED, LOW);

  } else {
    digitalWrite(RedLED, HIGH);
    Serial.print("Error in HTTP POST request: ");
    Serial.println(httpResponseCode);
    delay(2000);
    digitalWrite(RedLED, LOW);
  }

  http.end();  

  return response; 
}

String parseJsonResponse(String response) {
    StaticJsonDocument<2000> doc;  
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return "";
    }

    lectureList.clear();
    lectureMap.clear();
    lectureStartTimeMap.clear();
    lectureFinishTimeMap.clear();

    JsonArray lectures = doc.as<JsonArray>();
    for (JsonVariant v : lectures) {
        String moduleCode = v["moduleCode"].as<String>();
        String date = v["date"].as<String>();
        String lectureID = v["lectureId"].as<String>();
        String start = v["start"].as<String>();  
        String finish = v["finish"].as<String>(); 

        String displayText = moduleCode + " " + date;
        lectureList.push_back(displayText);
        lectureMap[displayText] = lectureID; 
        lectureStartTimeMap[displayText] = start; 
        lectureFinishTimeMap[displayText] = finish;  
    }

    String result;
    for (size_t i = 0; i < lectureList.size(); i++) {
        result += lectureList[i];
        if (i < lectureList.size() - 1) {
            result += ",";
        }
    }
    return result;
}

char* getModuleAt(const char* text, int i) {
    // Make a copy of the input string for tokenization
    char* copy = strdup(text);
    if (copy == nullptr) {
        return nullptr; // strdup failed
    }

    int count = 0;
    char* token = strtok(copy, ",");

    while (token != NULL) {
        if (count == i) {
            // Create a copy of the token to return
            char* result = strdup(token);
            free(copy); // Free the copied string
            return result; // Caller must free this memory
        }
        token = strtok(NULL, ",");
        count++;
    }

    free(copy); // Free the copied string
    return nullptr; // Index out of bounds
}

void displayTime()
{
 char key = getKeyPress();

 bool cancel = false;

  int timeCount = sizeof(timeValues) / sizeof(timeValues[0]);
  
  while(!cancel)
  {
     key = getKeyPress();
if(key == '8')
{
  delay(200);
  num++;
  if (num >= timeCount)
  {
      num = 0;
  }
}

if(key == '2')
{
  delay(200);
  num--;
  if (num < 0)
  {
      num = timeCount - 1;
  }
}

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(arr[currentItemIndex]);
  display.println("Duration:");
  display.println(timeValues[num]);
  display.display();

    if (key == '5') {
    delay(500);
    selectedModule = arr[currentItemIndex]; 
    selectedTime = timeValues[num];
    inMenu = true;
    Summary();
    return;
  }
  if (key == 'C') {
        display.clearDisplay();
        delay(100);
        isModuleSelected = false;
        inMenu = false;
        num = 0;
        cancel = true;
      }
  }
}

void Summary(){
   char key = getKeyPress();

   bool cancel = false;

   while(!cancel)
   {
    key = getKeyPress();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Added Lecture!");
  display.setCursor(0, 10);
  display.println("Code: ");
  display.println(selectedModule);
  display.println("Duration: ");
  display.println(selectedTime);

  
  display.display();

  if(key == 'C')
  {
      display.clearDisplay();
      delay(100);
      isModuleSelected = false;
      inMenu = false;
      num = 0;
      cancel = true;
  }
 }
}

String AddLectureData(String lectureId)
{
   delay(50);

  HTTPClient http;
  String response = "";

  String url = String(ProdApiUrl) + "/Lecture?lectureId=" + lectureId + "&classroomCode=" + classroomCode;
  http.begin(url);

  String jsonPayload = "{\"lectureId\":\"" + lectureId + "\",\"classroomCode\":\"" + classroomCode + "\"}";
  Serial.println(url);

  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(url);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error in HTTP POST request: ");
    Serial.println(httpResponseCode);
  }

  http.end();  

  return response; 
}

void PostAttendance() {
  HTTPClient http;
  http.begin(ProdApiUrl);

  String selectedLecture = lectureList[currentItemIndex];
  String lID = lectureMap[selectedLecture];

  // Construct JSON payload with variables
  String jsonPayload = "{\"LectureID\":\"" + lID + "\",\"UserID\":\"" + uid + "\",\"classroomCode\":\"" + classroomCode + "\",\"moduleCode\":\"" + selectedModule + "\"}";
  Serial.println(jsonPayload);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
   
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error in HTTP POST request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

String getLectures(String userID)
{
  String response;
  HTTPClient http;

  String url = String(ProdApiUrl) + "/Lecture?UserId=" + userID;
  Serial.println(url);
  http.begin(url);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    response = http.getString();
    Serial.println(response);
    
  } else {   
    Serial.print("Error in HTTP GET request: ");
    Serial.println(httpResponseCode);
    response = http.getString();
    Serial.println(response);
  }

  http.end();

  return parseJsonResponse(response);
}

 String GetModules(){
  String response;
  HTTPClient http;
  http.begin(ProdApiUrl);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    response = http.getString();
    Serial.println(response);
  } else {   
    Serial.print("Error in HTTP GET request: ");
    Serial.println(httpResponseCode);
    response = http.getString();
    Serial.println(response);
  }

  http.end();

  return parseJsonResponse(response);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    digitalWrite(RedLED, HIGH);
    delay(1000);
    Serial.println("...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ErikWifi, ErikPW);
    //Code to connect with VC account
   //WiFi.begin(VarsityWifi, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
    digitalWrite(RedLED, LOW);
  }
  Serial.println("Connected to WiFi");
}

bool GetStudentNumber()
{
  bool isCancel = false;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1.5);
  display.println("Please insert studentnumber: ");
  display.setCursor(0, 30);
  display.display();

  char key = getKeyPress();

   while (!isCancel) {
        if (key >= '0' && key <= '9' && StudentNo.length() < 8) { // Ensure only numeric input is accepted
            display.print(key);
            display.display();
            StudentNo += key;
        }

        if (key == 'B') {
            if (!StudentNo.isEmpty()) {
                StudentNo.remove(StudentNo.length() - 1);
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(1);
                display.println("Please insert studentnumber: ");
                display.setCursor(0, 30);
                display.print(StudentNo);
                display.display();
            }
        }

        if(key == 'C'){
          isCancel = true;
          StudentNo = "";
        }

        if(key == 'A' && StudentNo.length() == 8){
          Serial.println("Student Number: " + StudentNo);
          return true;
          }

        key = getKeyPress();
    }  
  return false;
}

bool ScanNewStudent(){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1.9);
  display.println("Please scan student  card");
  display.display();

  if (mfrc522.PCD_PerformSelfTest()) {
    Serial.println("Self-test passed. Scanner is powered and working.");
} else {
    Serial.println("Self-test failed. Check power or connections.");
}

  char key = getKeyPress();

  uid = "";
  while(uid == "" && key != 'C'){
    if (mfrc522.PICC_IsNewCardPresent()) {
      delay(50); // Short delay to stabilize the card reading process
      if (mfrc522.PICC_ReadCardSerial()) {
      
        uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          uid += String(mfrc522.uid.uidByte[i], HEX);
        }
      uid.toUpperCase();
      Serial.println("Card UID: " + uid);

      PostNewStudent();

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

        uid = "";
        return true;
        StudentNo = "";
      } 
    }
    key = getKeyPress();
  }
  return false;
}

bool ScanNewStaff(){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1.9);
  display.println("Please scan staff    card");
  display.display();

  char key = getKeyPress();

  uid = "";
  while(uid == "" && key != 'C'){
    if (mfrc522.PICC_IsNewCardPresent()) {
      delay(50); // Short delay to stabilize the card reading process
      if (mfrc522.PICC_ReadCardSerial()) {
      
        uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          uid += String(mfrc522.uid.uidByte[i], HEX);
        }
      uid.toUpperCase();
      Serial.println("Card UID: " + uid);

      PostNewStaff();

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

       uid = "";
       return true;
       StaffNo = "";
      } 
    }
    key = getKeyPress();
  }
  return false;
}

void MainMenu(char key)
{
   //currentPage = 0;
   //currentItemIndex = 0;
   uid = "";
   qrCodeData = "";

  display.setTextSize(1.9);
  display.clearDisplay();
  display.setCursor(0,0);

  

  int startItem = currentPage * MenuItemsPerPage;
  int endItem = startItem + MenuItemsPerPage;

  if (endItem > menuCount){
    endItem = menuCount;
  } 

  for (int i = startItem; i < endItem; i++) {
    if (i == currentItemIndex) {
      display.print(">");
    } else {
      display.print(" ");
    }
    display.println(menuArr[i]);
  }

if (key == '8') {
    delay(200); 
    currentItemIndex++;

    if (currentItemIndex >= menuCount) {
      currentItemIndex = 0;
    }

    if (currentItemIndex >= (currentPage + 1) * MenuItemsPerPage) {
      currentPage++;
    }
    
    if (currentItemIndex < currentPage * MenuItemsPerPage) {
    currentPage--;
      }
  }

  if(key == '2')
  {
    delay(200);
   currentItemIndex--;

    if (currentItemIndex < 0) {
      currentItemIndex = menuCount - 1;
    }

    if (currentItemIndex < currentPage * MenuItemsPerPage) {
      currentPage--;
    }

    if (currentItemIndex >= (currentPage + 1) * MenuItemsPerPage) {
      currentPage++;
    }
  }

  if(key == '5' && menuArr[currentItemIndex] == "Add Student")
  {
     GetStudentNumber();
     if(StudentNo.length() == 8){
          ScanNewStudent();
          }
  }
  else if(key == '5' && menuArr[currentItemIndex] == "Add Staff Member")
{
 GetStaffNumber();
 if(StaffNo.length() == 5){
          ScanNewStaff();
          }
}
else if(key == '5' && menuArr[currentItemIndex] == "Start Lecture")
{
  scanLecturer();
  if(isScanLec == true)
  {
    selectLecture();
    if(isSelectLec == true)
    {
      displayLectures();
      if(isDisplayLec == true)
      {
        scanOut();
        if(isScanOut == true)
        {
          lecturerScanOut();
        }
        else 
        {
          return;
        }
      }
      else 
      {
        return;
      }
    }
    else 
    {
      return;
    }
  }
  else 
  {
    return;
  }
}
 display.display();
}

bool GetStaffNumber()
{
  bool isCancel = false;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1.5);
  display.println("Please insert staff  number: ");
  display.setCursor(0, 30);
  display.display();

  char key = getKeyPress();

   while (!isCancel) {
        if (key >= '0' && key <= '9' && StaffNo.length() < 5) { // Ensure only numeric input is accepted
            display.print(key);
            display.display();
            StaffNo += key;
        }

        if (key == 'B') {
            if (!StaffNo.isEmpty()) {
                StaffNo.remove(StaffNo.length() - 1);
                display.clearDisplay();
                display.setCursor(0, 0);
                display.setTextSize(1);
                display.println("Please insert staff number: ");
                display.setCursor(0, 30);
                display.print(StaffNo);
                display.display();
            }
        }

        if(key == 'C'){
          isCancel = true;
          StaffNo = "";
        }

        if(key == 'A' && StaffNo.length() == 5){
          Serial.println("Staff Number: " + StaffNo);
          return true;
          }

        key = getKeyPress();
    }  
  return false;
}

bool scanLecturer()
{
  bool cancel = false;
  bool qrProcess = false;

  unsigned long lastScanTime = 0; 
  unsigned long scanCooldown = 2000;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1.9);
  display.println("Please Scan Lecturer Card or QR Code");
  display.display();

  if (mfrc522.PCD_PerformSelfTest()) {
    Serial.println("Self-test passed. Scanner is powered and working.");
} else {
    Serial.println("Self-test failed. Check power or connections.");
}
  
  while(!cancel)
  {
    qrCodeData = "";

    if (millis() - lastScanTime >= scanCooldown)
  {
    qrProcess = false;
  }

     char key = getKeyPress();

     while (Serial2.available())
  {
    char incomingChar = (char)Serial2.read();
    qrCodeData += incomingChar;

    uid = "";

    if (incomingChar == '\n' && !qrProcess)
    {
      qrCodeData.trim();

      if (qrCodeData.startsWith("QR:"))
      {
        // Extract and print the QR code payload
        String payload = qrCodeData.substring(3); 
       
        Serial.println("QR Code: " + payload);
        uid = payload;

        if(uid.isEmpty())
        {
           digitalWrite(RedLED, HIGH);
           Serial.println("QR Code Null");
           qrProcess = true;
           lastScanTime = millis();
           delay(2000);
           digitalWrite(RedLED, LOW);
        }
        else
        {
        digitalWrite(GreenLED, HIGH);

        // Send HTTP request
        getLectures(uid);

        qrProcess = true;
        lastScanTime = millis();

        //delay(3000);
        digitalWrite(GreenLED, LOW);

         uid = "";
         qrCodeData = "";
    
         isScanLec = true;
         return true;
        }
        
      }
      else if (qrCodeData.startsWith("Invalid QR:"))
      {
        Serial.println("Received an invalid QR code.");
      }

      uid = "";
      qrCodeData = "";
    }
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    delay(50); // Short delay to stabilize the card reading process
    if (mfrc522.PICC_ReadCardSerial()) {
      digitalWrite(GreenLED, HIGH);
      
      uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();
      Serial.println("Card UID: " + uid);

      // Send HTTP request
      getLectures(uid);
      
      delay(2000);
      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      digitalWrite(GreenLED, LOW);

      uid = "";
      qrCodeData = "";
      //selectLecture();
      isScanLec = true;
      return true;
    } 
  }
    if(key == 'C'){
          //cancel = true;
          isScanLec = false;
          return true;
      }
}
return false;
}

bool selectLecture()
{
    bool cancel = false;
    display.setTextSize(0.5);
    display.clearDisplay();
    display.setCursor(0, 0);
   
    while (!cancel)
    {
      int startItem = currentPage * MenuItemsPerPage;
    int endItem = startItem + MenuItemsPerPage;

    if (endItem > lectureList.size())
    {
        endItem = lectureList.size();
    }
        display.setTextSize(0.5);
        display.clearDisplay();
        display.setCursor(0, 0);
        for (int i = startItem; i < endItem; i++)
        {
           if (i == currentItemIndex) {
                display.print(">");
            } else {
                display.print(" ");
            }
            display.println(lectureList[i]);
            
        }

        char key = getKeyPress();
        if (key == '8')
        {
            delay(200);
            currentItemIndex++;
            if (currentItemIndex >= lectureList.size())
            {
                currentItemIndex = 0;
                currentPage = 0;
            }
            if (currentItemIndex >= (currentPage + 1) * MenuItemsPerPage)
            {
                currentPage++;
            }
            if (currentItemIndex < currentPage * MenuItemsPerPage)
            {
                currentPage--;
            }
        }
        if (key == '2')
        {
            delay(200);
            currentItemIndex--;
            if (currentItemIndex < 0)
            {
                currentItemIndex = lectureList.size() - 1;
                currentPage = (lectureList.size() - 1) / MenuItemsPerPage;
            }
            if (currentItemIndex < currentPage * MenuItemsPerPage)
            {
                currentPage--;
            }
            if (currentItemIndex >= (currentPage + 1) * MenuItemsPerPage)
            {
                currentPage++;
            }
        }
        if (key == '5')
        {
            display.clearDisplay();
            display.setCursor(0, 10);
            delay(100);
            display.display();
            Serial.println(lectureList[currentItemIndex]);
            String selectedLecture = lectureList[currentItemIndex];
            String lectureId = lectureMap[selectedLecture];
            selectedModule = selectedLecture.substring(0, selectedLecture.indexOf(' '));
            selectedStartTime = lectureStartTimeMap[selectedLecture];
            selectedFinishTime = lectureFinishTimeMap[selectedLecture];

            Serial.println(lectureId);

            qrCodeData = "";
            uid = "";

            AddLectureData(lectureId);

            //displayLectures();
            isSelectLec = true;
            return true;
        }
         
        if (key == 'C')
        {
            uid = "";
            qrCodeData = "";
            //cancel = true;
            isSelectLec = false;
            return true;
        }
       display.display();
    }
    return false;
}

bool displayLectures()
{
  bool cancel = false;
  bool qrProcess2 = false;

  unsigned long lastScanTime = 0; 
  unsigned long scanCooldown = 2000;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Students Scan In:");
  display.println("\n" + lectureList[currentItemIndex]);
  display.display();

   qrCodeData = "";
   uid = "";
  Serial.println(qrCodeData + " qrCodeData");

while(!cancel)
{
   char key = getKeyPress();
  if (millis() - lastScanTime >= scanCooldown)
  {
    qrProcess2 = false;
  }
  
   while (Serial2.available())
  {
    
    char incomingChar = (char)Serial2.read();
    qrCodeData += incomingChar;

    

    if (incomingChar == '\n' && !qrProcess2)
    {
      qrCodeData.trim();

      if (qrCodeData.startsWith("QR:"))
      {
        // Extract and print the QR code payload
        String payload = qrCodeData.substring(3); 
       
        Serial.println("QR Code: " + payload);
        uid = payload;

        if(uid.isEmpty())
        {
           digitalWrite(RedLED, HIGH);
           Serial.println("QR Code Null");
           qrProcess2 = true;
           lastScanTime = millis();
           delay(2000);
           digitalWrite(RedLED, LOW);
        }
        else
        {
        digitalWrite(GreenLED, HIGH);

        PostAttendance(); 

        qrProcess2 = true;
        lastScanTime = millis();

        //delay(3000);
        digitalWrite(GreenLED, LOW);
        }
        
      }
      else if (qrCodeData.startsWith("Invalid QR:"))
      {
        Serial.println("Received an invalid QR code.");
      }

      qrCodeData = "";
      uid = "";
    }
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    delay(50); // Short delay to stabilize the card reading process
    if (mfrc522.PICC_ReadCardSerial()) {
      digitalWrite(GreenLED, HIGH);
      
      uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();
      Serial.println("Card UID: " + uid);

      // Send HTTP request
      PostAttendance();

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      digitalWrite(GreenLED, LOW);
      uid = "";
    } 
  }
  if(key == 'A')
  { 
    isDisplayLec = true;
    return true;
  }
  if(key == 'C')
  {
    isDisplayLec = false;
    return true;
  }
}
return false;
}

bool scanOut()
{
  bool cancel = false;
  bool qrProcess2 = false;

  unsigned long lastScanTime = 0; 
  unsigned long scanCooldown = 2000;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Students Scan Out:");
  display.println("\n" + lectureList[currentItemIndex]);
   
  display.display();

  while(!cancel)
{
   char key = getKeyPress();
  if (millis() - lastScanTime >= scanCooldown)
  {
    qrProcess2 = false;
  }

   while (Serial2.available())
  {
    char incomingChar = (char)Serial2.read();
    qrCodeData += incomingChar;

    uid = "";

    if (incomingChar == '\n' && !qrProcess2)
    {
      qrCodeData.trim();

      if (qrCodeData.startsWith("QR:"))
      {
        // Extract and print the QR code payload
        String payload = qrCodeData.substring(3); 
       
        Serial.println("QR Code: " + payload);
        uid = payload;

        if(uid.isEmpty())
        {
           digitalWrite(RedLED, HIGH);
           Serial.println("QR Code Null");
           qrProcess2 = true;
           lastScanTime = millis();
           delay(2000);
           digitalWrite(RedLED, LOW);
        }
        else
        {
        digitalWrite(GreenLED, HIGH);

        PostAttendance(); 

        qrProcess2 = true;
        lastScanTime = millis();

        //delay(3000);
        digitalWrite(GreenLED, LOW);
        }
        
      }
      else if (qrCodeData.startsWith("Invalid QR:"))
      {
        Serial.println("Received an invalid QR code.");
      }

      uid = "";
      qrCodeData = "";
    }
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    delay(50); // Short delay to stabilize the card reading process
    if (mfrc522.PICC_ReadCardSerial()) {
      digitalWrite(GreenLED, HIGH);
      
      uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();
      Serial.println("Card UID: " + uid);

      // Send HTTP request
      PostAttendance();

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      digitalWrite(GreenLED, LOW);

      uid = "";
    } 
  }
  if(key == 'A')
  {
     isScanOut = true;
     return true;
  }
  if(key == 'C')
  {
    isScanOut = false;
    return true;
  }
}
return false;
}

bool lecturerScanOut()
{
  bool cancel = false;
  bool qrProcess2 = false;

  unsigned long lastScanTime = 0; 
  unsigned long scanCooldown = 2000;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("End Class?");
  String selectedLecture = lectureList[currentItemIndex];
  String lectureId = lectureMap[selectedLecture];
  Serial.println(lectureId);
  display.display();

  while(!cancel)
{
  char key = getKeyPress();

  if(key == 'A')
  {
    AddLectureData(lectureId);
    delay(100);
    return true;
  }

  if(key == 'C')
  {
    return true;
  }
   
}
return false;
}

void loop() {
  char key = getKeyPress();
  
  MainMenu(key);
}




 

  

  

