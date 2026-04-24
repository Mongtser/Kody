#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

#define SERVO_PIN 13
#define ROW_NUM 4
#define COLUMN_NUM 4

Servo lockServo;
LiquidCrystal lcd(50, 48, 46, 2, 12, 40);

uint8_t id;
bool fingerprintScanCompleted = false;
bool keypadUnlocked = false;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {11, 10, 9, 8};
byte pin_column[COLUMN_NUM] = {7, 6, 5, 4};

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Public portfolio version: replace these placeholders for private testing.
const String password_1 = "<redacted>";
const String password_2 = "<redacted>";
const String password_3 = "<redacted>";

String input_password;

#define Finger_Serial Serial1
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Finger_Serial);

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  delay(100);

  Serial.println("\n\nAdafruit finger detect test");

  finger.begin(57600);
  delay(5);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.println("Sensor does not contain fingerprint data. Run enroll flow.");
    enrollFinger();
  } else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
    delay(1000);
  }

  lockServo.attach(SERVO_PIN);

  lcd.begin(16, 2);
  pinMode(3, OUTPUT);
  analogWrite(3, 0);
  lcd.print("Welcome");
  delay(3000);
  lcd.clear();
  lcd.print("Scan Finger");
  delay(3000);
  lcd.clear();

  input_password.reserve(32);
}

uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    Serial.println("Image taken");
  } else if (p == FINGERPRINT_NOFINGER) {
    Serial.println("No finger detected");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_IMAGEFAIL) {
    Serial.println("Imaging error");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  p = finger.image2Tz();
  if (p == FINGERPRINT_OK) {
    Serial.println("Image converted");
  } else if (p == FINGERPRINT_IMAGEMESS) {
    Serial.println("Image too messy");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_FEATUREFAIL || p == FINGERPRINT_INVALIDIMAGE) {
    Serial.println("Could not find fingerprint features");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    delay(500);
    Serial.println("Try again");
    return p;
  } else {
    Serial.println("Unknown error");
    delay(500);
    Serial.println("Try again");
    return p;
  }

  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  Serial.println("Keypad unlocked");

  lcd.print("Keypad Unlocked");
  delay(2000);
  lcd.clear();

  fingerprintScanCompleted = true;
  Serial.println("Enter keypad code.");
  lcd.print("Enter Code");
  delay(2000);
  lcd.clear();

  return p;
}

void deleteFingerprintDatabase()
{
  Serial.println("\n\nDeleting all fingerprint templates!");
  lcd.print("Deleting all");
  lcd.setCursor(0, 1);
  lcd.print("Scans!");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Select A to");
  lcd.setCursor(0, 1);
  lcd.print("Continue");
  delay(2000);

  lcd.clear();
  lcd.print("Press any key");
  lcd.setCursor(0, 1);
  lcd.print("To stop");

  char user = keypad.getKey();
  if (user == 'A') {
    finger.emptyDatabase();
    Serial.println("Database is empty.");
    lcd.clear();
    lcd.print("Database empty");
    delay(2000);
    lcd.clear();
  } else {
    showMenu();
  }
}

uint8_t readnumber(void)
{
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void enrollFinger()
{
  Serial.println("Ready to enroll a fingerprint.");
  Serial.println("Type the ID number from 1 to 10.");

  lcd.println("Ready to enroll");
  lcd.setCursor(0, 1);
  lcd.println("a fingerprint");
  delay(2000);
  lcd.clear();

  lcd.println("Type ID #");
  lcd.setCursor(0, 1);
  lcd.println("From 1-10");
  delay(2000);
  lcd.clear();

  id = keypad.getKey();
  if (id == 0) {
    return;
  }

  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (!getFingerprintEnroll());
}

uint8_t getFingerprintEnroll()
{
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.print("Creating model for #");
  Serial.println(id);
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    showMenu();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

void handleKeypadAccess()
{
  char key = keypad.getKey();
  if (!key) {
    return;
  }

  Serial.println(key);
  lcd.print(key);
  delay(1000);
  lcd.clear();

  if (key == '*') {
    input_password = "";
  } else if (key == '#') {
    if (input_password == password_1) {
      lcd.println("Unlocking");
      lockServo.write(0);
      delay(6000);
      lcd.clear();
      keypadUnlocked = true;
      showMenu();
    } else {
      lcd.print("Incorrect");
      delay(1000);
      lcd.clear();
      lcd.print("Try Again");
      delay(1000);
      lcd.clear();
      Serial.println("The password is incorrect, try again");
    }
    input_password = "";
  } else {
    input_password += key;
  }
}

void showMenu()
{
  Serial.println("Select options 1-3");
  lcd.println("Select Options");
  lcd.setCursor(0, 1);
  lcd.print("1 - 3");
  delay(2000);
  lcd.clear();

  lcd.print("1.Enter new scan");
  delay(4000);
  lcd.clear();
  lcd.print("2.Delete Scans");
  delay(4000);
  lcd.clear();
  lcd.print("3.Lock");
  delay(4000);
  lcd.clear();
}

void loop()
{
  if (!fingerprintScanCompleted) {
    getFingerprintID();
  }

  if (fingerprintScanCompleted) {
    handleKeypadAccess();
  }

  if (fingerprintScanCompleted && keypadUnlocked) {
    char user = keypad.getKey();

    if (user == '1') {
      Serial.println("Enrolling new fingerprint");
      lcd.print("Enrolling new");
      lcd.setCursor(0, 1);
      lcd.print("Fingerprint");
      delay(2000);
      lcd.clear();
      enrollFinger();
    } else if (user == '2') {
      Serial.println("Deleting scans");
      lcd.print("Deleting Scans");
      delay(2000);
      lcd.clear();
      deleteFingerprintDatabase();
    } else if (user == '3') {
      Serial.println("Locking");
      lcd.print("Locking");
      lockServo.write(90);
      delay(2000);
      lcd.clear();
    }
  }
}
