#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Vector.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


#define LED_STRIPE_PIN        2
#define LED_NUMBER          200
#define LED_OFFSET           0

#define PROBLEM_LEN          60
#define HOLD_LEN             10
#define ZERO                 (-48)

#define START0     'l'
#define START1     '#'
#define START2STOP '#'
#define DEL        ','

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

Adafruit_NeoPixel pixels(LED_NUMBER, LED_STRIPE_PIN, NEO_RGB + NEO_KHZ800);

struct hold_{
  uint8_t type;
  uint8_t number; 
};

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

enum class State:uint32_t {wait0,wait1,acquire,ready,error};
enum class State state=State::wait0;
char hold_storage_array[HOLD_LEN];
Vector<char> current_hold(hold_storage_array);
//Vector containing the problem
struct hold_ problem_storage_array[PROBLEM_LEN];
Vector<struct hold_> problem(problem_storage_array);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

String format_unexpected(char ch, State state)
{
  String uexpected="Unexpected character: ";
  return uexpected + ch + "; state: " + static_cast<uint32_t>(state) + ".";
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        char ch;
        for (int i = 0; i < rxValue.length(); i++) {
          ch = rxValue[i];
          Serial.print(ch);
          switch (ch) {
            case START0:
              if( (state==State::wait0) || (state == State::error) )
              {
                state=State::wait1;
              }else{
                Serial.println(format_unexpected(ch,state));
                state=State::error;
              }
              break;
          
            case START2STOP:
              if (state==State::wait1)
              {
                state=State::acquire;
                current_hold.clear();
                problem.clear();
              }else if (state==State::acquire)
              {
                problem.push_back(hold_from_char(current_hold));
                current_hold.clear();
                Serial.println("NEW problem.");
                state=State::ready;
              }else{
                Serial.println(format_unexpected(ch,state));
                state=State::error;
              }
              break;
      
            case DEL:
              if(state==State::acquire)
              {
                problem.push_back(hold_from_char(current_hold));
                current_hold.clear();
              }else
              {
                Serial.println(format_unexpected(ch, state));
                state=State::error;
              }
              break;
              
            default:
              if (state==State::acquire)
              {
                current_hold.push_back(ch);
              }else
              {
                Serial.println(format_unexpected(ch, state));
                state=State::error;
              }
              break;            
          }
        }
        if(state==State::ready)
        {
          show_problem(problem, pixels);
          state=State::wait0;
        }
        Serial.println();
        Serial.println("*********");
      }

    
    }
};

void setup() {
  Serial.begin(115200);

  pixels.begin();
  pixels.show();

  // Create the BLE Device
  BLEDevice::init("MoonBoard 1");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(pService->getUUID());
  pServer->getAdvertising()->setMinPreferred(0x20);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

struct hold_ hold_from_char(Vector<char>& hold){
  struct hold_ temp_hold;
  int zero = ZERO;
  temp_hold.number=0;
  int base=1;
  for (size_t i = hold.size()-1; i > 0; i--)
  {
    temp_hold.number += (zero + ((uint8_t)hold.at(i)))*base;
    base*=10;
  }
  temp_hold.type=hold.at(0);
  return temp_hold; 
}


void show_problem(Vector<struct hold_>& problem, Adafruit_NeoPixel& pixels)
{ 
    for (size_t i = 0; i < problem.size(); i++)
    {
      switch (problem[i].type)
      {
      case 'S':
        pixels.setPixelColor(problem[i].number + LED_OFFSET,0,255,0);
        break;
      case 'P':
        pixels.setPixelColor(problem[i].number + LED_OFFSET,0,0,255);
        break;
      case 'E':
        pixels.setPixelColor(problem[i].number + LED_OFFSET,255,0,0);
        break;
      }
    }
    pixels.show();
}

void loop() {

    if (deviceConnected) {
      delay(100); // bluetooth stack will go into congestion, if too many packets are sent
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
