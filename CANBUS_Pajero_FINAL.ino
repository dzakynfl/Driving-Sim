#include <SPI.h>
#include <mcp2515.h>

// Pin definitions for MCP2515
#define CAN_CS_PIN 53  // CS pin for MCP2515

// Constants
const uint8_t MASK_LIGHT = 1;
const uint8_t MASK_SIGN_LEFT = 2;
const uint8_t MASK_SIGN_RIGHT = 4;
const uint8_t MASK_LCD_BL = 8;

// Global variables
MCP2515 mcp2515(CAN_CS_PIN);
uint16_t counter = 0;
uint16_t counterSign = 0;

// Vehicle parameters
uint8_t speed = 0;
uint16_t rpm = 0;
uint8_t temperature = 0x70;
uint8_t gear = 0;

// Light indicators
uint8_t lightSignR = 0;
uint8_t lightSignL = 0;
uint8_t lightSignLR = 0;
uint8_t lightHazard = 0;
uint8_t lightHead = 0;
uint8_t lightLCDBK = 0;
uint8_t cruise = 0;

// Warning lights
uint8_t lightBeam = 0;
uint8_t lightFogGreen = 0;
uint8_t lightFogOrange = 0;
uint8_t battery = 0;
uint8_t eco = 0;
uint8_t astcOff = 0;
uint8_t astc = 0;
uint8_t parkingBreak = 0;
uint8_t indabs = 0;
uint8_t oil = 0;
uint8_t oilVal = 0;
uint8_t brake = 0;
uint8_t brake1 = 0;
uint8_t brake2 = 0;

// Light status
uint8_t BL = 0;
uint8_t SR = 0;
uint8_t SL = 0;
uint8_t LL = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  printHelp();
}

void printHelp() {
  Serial.println("\nSpeedometer Control Commands:");
  Serial.println("1. Speed Control:");
  Serial.println("   Vel[value] - Set speed (0-255) e.g. Vel120");
  Serial.println("2. RPM Control:");
  Serial.println("   RPM[value] - Set RPM (0-16383) e.g. RPM2500");
  Serial.println("3. Gear Control:");
  Serial.println("   G[value] - Set gear (0-6,r,n,p) e.g. G1 or Gr");
  Serial.println("4. Temperature:");
  Serial.println("   t[value] - Set temperature (0-255) e.g. t70");
  Serial.println("5. Indicators:");
  Serial.println("   SignL[value] - Toggle Left Signal");
  Serial.println("   SignR[value] - Toggle Right Signal");
  Serial.println("   SignLR[value] - Toggle Hazard");
  Serial.println("   Head[value] - Toggle Headlight");
  Serial.println("   LCDBK[value] - Toggle LCD Backlight");
  Serial.println("6. Warning Lights:");
  Serial.println("   Beam[value] - Toggle High Beam");
  Serial.println("   FogG[value] - Toggle Fog Light");
  Serial.println("   FogO[value] - Toggle Fog Light");
  Serial.println("   Batt[value] - Toggle Battery Warning");
  Serial.println("   ECO[value] - Toggle ECO");
  Serial.println("   ASTC[value] - Toggle ASTC");
  Serial.println("   ASTCOff[value] - Toggle ASTC Off");
  Serial.println("   ABS[value] - Toggle ABS");
  Serial.println("   Oil[value] - Toggle Oil Warning");
  Serial.println("7. Other:");
  Serial.println("   ? - Show this help");
  Serial.println("   s - Show current status");
  Serial.println("   PB[value] - Toggle Parking Brake");
}

void showStatus() {
  Serial.println("\nCurrent Status:");
  Serial.print("Speed: "); Serial.println(speed);
  Serial.print("RPM: "); Serial.println(rpm);
  Serial.print("Gear: "); Serial.println(gear);
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.println("\nIndicators:");
  Serial.print("Left Signal: "); Serial.println(lightSignL);
  Serial.print("Right Signal: "); Serial.println(lightSignR);
  Serial.print("Hazard: "); Serial.println(lightSignLR);
  Serial.print("Headlight: "); Serial.println(lightHead);
  Serial.print("LCD Backlight: "); Serial.println(lightLCDBK);
  Serial.print("Parking Brake: "); Serial.println(parkingBreak ? "ON" : "OFF");
}

void sendCANMessage(uint32_t id, uint8_t* data, uint8_t length) {
  struct can_frame canMsg;
  canMsg.can_id = id;
  canMsg.can_dlc = length;
  
  for(int i = 0; i < length; i++) {
    canMsg.data[i] = data[i];
  }
  
  mcp2515.sendMessage(&canMsg);
}

void updateSpeedometer() {
  uint8_t data[8] = {0};
  
  switch(counter) {
    case 0: // Keyless
      data[0] = 0x80;
      data[1] = 0xa0;
      sendCANMessage(0x17, data, 8);
      break;
      
    case 5: // RPM
    case 105:
      data[0] = rpm & 0xFD;
      data[1] = rpm/256;
      data[2] = 0xff;
      data[3] = oilVal;
      sendCANMessage(776, data, 8);
      break;
      
    case 10: // Temperature
      data[0] = temperature;
      sendCANMessage(0x608, data, 8);
      break;
      
    case 20: // Gear
      data[0] = gear;
      data[1] = 0x05;
      sendCANMessage(0x418, data, 8);
      break;
      
    case 25: // Parking brake
      data[0] = brake1;
      data[1] = brake2; 
      sendCANMessage(0x34A, data, 8);
      break;
      
    case 35: // ASTC & Parking brake status
      data[0] = astc * 8 + (indabs * 4) + parkingBreak * 2 + astcOff;
      sendCANMessage(0x200, data, 8);
      break;
      
    case 50: // Speed
      data[0] = speed/2-speed*0.02;
      data[1] = speed*10;
      sendCANMessage(0x215, data, 8);
      break;
      
    case 60: // Turn signals
      data[0] = 4;
      data[4] = BL + SR + SL + LL;
      sendCANMessage(789, data, 8);
      break;
      
    case 65: // Lights
      data[6] = lightFogOrange * 16 + lightFogGreen * 8 + lightBeam * 4;
      sendCANMessage(1060, data, 8);
      break;
  }
}

void updateTurnSignals() {
  if (lightSignLR) {
    if (counterSign == 3) {
      SR = 0;
      SL = 0;
    }
    else if (counterSign == 303) {
      SR = MASK_SIGN_RIGHT;
      SL = MASK_SIGN_LEFT;
    }
  }
  
  if (lightSignL) {
    if (counterSign == 3) {
      SL = 0;
    }
    else if (counterSign == 303) {
      SL = MASK_SIGN_LEFT;
    }
  }
  
  if (lightSignR) {
    if (counterSign == 3) {
      SR = 0;
    }
    else if (counterSign == 303) {
      SR = MASK_SIGN_RIGHT;
    }
  }
  
  if (!lightSignLR && !lightSignL && !lightSignR) {
    SR = 0;
    SL = 0;
  }
}

void processSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    int colonIndex = cmd.indexOf(':');
    if (colonIndex == -1) return;
    
    String command = cmd.substring(0, colonIndex);
    int value = cmd.substring(colonIndex + 1).toInt();
    
    if (command == "G") {
      switch(value) {
        case 0: gear = 0x46; break;  // F
        case 1: gear = 0x31; break;  // 1
        case 2: gear = 0x32; break;  // 2
        case 3: gear = 0x33; break;  // 3
        case 4: gear = 0x34; break;  // 4
        case 5: gear = 0x35; break;  // 5
        case 6: gear = 0x36; break;  // 6
        case 7: gear = 0x37; break;  // 7
        case 8: gear = 0x38; break;  // 8
        case 9: gear = 0x39; break;  // 9
        case 10: gear = 0x41; break; // A
        case 11: gear = 0x44; break; // D
        case 12: gear = 0x4C; break; // L
        case 13: gear = 0x4E; break; // N
        case 14: gear = 0x50; break; // P
        case 15: gear = 0x52; break; // R
        case 16: gear = 0x44; break; // D (additional D position)
      }
      Serial.print("Gear set to: ");
      switch(gear) {
        case 0x46: Serial.println("F"); break;
        case 0x31: Serial.println("1"); break;
        case 0x32: Serial.println("2"); break;
        case 0x33: Serial.println("3"); break;
        case 0x34: Serial.println("4"); break;
        case 0x35: Serial.println("5"); break;
        case 0x36: Serial.println("6"); break;
        case 0x37: Serial.println("7"); break;
        case 0x38: Serial.println("8"); break;
        case 0x39: Serial.println("9"); break;
        case 0x41: Serial.println("A"); break;
        case 0x44: Serial.println("D"); break;
        case 0x4C: Serial.println("L"); break;
        case 0x4E: Serial.println("N"); break;
        case 0x50: Serial.println("P"); break;
        case 0x52: Serial.println("R"); break;
        default: Serial.println(gear, HEX); break;
      }
    }
    else if (command == "Vel") {
      speed = value;
      Serial.print("Speed set to: "); Serial.println(speed);
    }
    else if (command == "RPM") {
      rpm = value;
      Serial.print("RPM set to: "); Serial.println(rpm);
    }
    else if (command == "t") { // Temperature
      temperature = value;
      Serial.print("Temperature set to: "); Serial.println(temperature);
    }
    else if (command == "SignL") {
      lightSignL = value;
      if(lightSignL) {
        lightSignR = 0;
        lightSignLR = 0;
      }
      Serial.print("Left signal: "); Serial.println(lightSignL);
    }
    else if (command == "SignR") {
      lightSignR = value;
      if(lightSignR) {
        lightSignL = 0;
        lightSignLR = 0;
      }
      Serial.print("Right signal: "); Serial.println(lightSignR);
    }
    else if (command == "SignLR") {
      lightSignLR = value;
      if(lightSignLR) {
        lightSignL = 0;
        lightSignR = 0;
      }
      Serial.print("Hazard lights: "); Serial.println(lightSignLR);
    }
    else if (command == "Head") {
      lightHead = value;
      LL = lightHead ? MASK_LIGHT : 0;
      Serial.print("Headlight: "); Serial.println(lightHead);
    }
    else if (command == "LCDBK") {
      lightLCDBK = value;
      BL = lightLCDBK ? MASK_LCD_BL : 0;
      Serial.print("LCD Backlight: "); Serial.println(lightLCDBK);
    }
    else if (command == "Beam") {
      lightBeam = value;
      Serial.print("High beam: "); Serial.println(lightBeam);
    }
    else if (command == "FogG") {
      lightFogGreen = value;
      Serial.print("Green fog light: "); Serial.println(lightFogGreen);
    }
    else if (command == "FogO") {
      lightFogOrange = value;
      Serial.print("Orange fog light: "); Serial.println(lightFogOrange);
    }
    else if (command == "Batt") {
      battery = value;
      Serial.print("Battery warning: "); Serial.println(battery);
    }
    else if (command == "ECO") {
      eco = value;
      Serial.print("ECO mode: "); Serial.println(eco);
    }
    else if (command == "ASTC") {
      astc = value;
      Serial.print("ASTC: "); Serial.println(astc);
    }
    else if (command == "ASTCOff") {
      astcOff = value;
      Serial.print("ASTC Off: "); Serial.println(astcOff);
    }
    else if (command == "ABS") {
      indabs = value;
      Serial.print("ABS: "); Serial.println(indabs);
    }
    else if (command == "Oil") {
      oil = value;
      Serial.print("Oil warning: "); Serial.println(oil);
    }
    else if (command == "PB") {
      parkingBreak = value;
      if (parkingBreak) {
        brake1 = 0xFF;
        brake2 = 0xFF;
      } else {
        brake1 = 0x00;
        brake2 = 0x00;
      }
      Serial.print("Parking brake: "); Serial.println(parkingBreak ? "ON" : "OFF");
    }
    else if (command == "?") {
      printHelp();
    }
    else if (command == "s") {
      showStatus();
    }
  }
}

void loop() {
  processSerialCommand();
  updateSpeedometer();
  updateTurnSignals();
  
  counter++;
  if (counter >= 2000) counter = 0;
  
  counterSign++;
  if (counterSign >= 600) counterSign = 0;
}