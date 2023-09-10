#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <array>
// 3rd-party libraries
#include <Adafruit_MAX31856.h>
#include <HX711.h>

#define SERIAL_MODE 0
#define SD_MODE     1
#define TC_MODE     1
#define LC_MODE     1

// Pin #def's for the thermocouples
#define SPI_CS1   10
#define SPI_CS2   36
#define SPI_CS3   37
#define SPI_MOSI  11
#define SPI_MISO  12
#define SPI_SCK   13 

#define DRDY 5

// Pin #def's for the load cells
#define LC_DOUT1  21
#define LC_DOUT2  22
#define LC_DOUT3  23
#define LC_SCK    39 

// These pin def's are for teensy 4.1. The pinout for the 3.6 might be different

#define SAMPLES_PER_SECONDS 10
#define NUM_TC 3
#define NUM_LC 3

float currTime {0.0f};
float prevTime {0.0f};
float offset {0.0f};

#if TC_MODE
Adafruit_MAX31856 thermocoupleA = Adafruit_MAX31856{SPI_CS1, SPI_MOSI, SPI_MISO, SPI_SCK};
Adafruit_MAX31856 thermocoupleB = Adafruit_MAX31856{SPI_CS2, SPI_MOSI, SPI_MISO, SPI_SCK};
Adafruit_MAX31856 thermocoupleC = Adafruit_MAX31856{SPI_CS3, SPI_MOSI, SPI_MISO, SPI_SCK};
const char* thermocoupleAFileName {"Thermocouple A Data.txt"};
const char* thermocoupleBFileName {"Thermocouple B Data.txt"};
const char* thermocoupleCFileName {"Thermocouple C Data.txt"};
File thermocoupleFile;
//std::array<Adafruit_MAX31856*, 1> arrayOfThermocouples {&thermocoupleA};
std::array<Adafruit_MAX31856, NUM_TC> arrayOfThermocouples {&thermocoupleA, &thermocoupleB, &thermocoupleC};
//std::shared_ptr<Adafruit_MAX31856> share1 = std::make_shared<Adafruit_MAX31856>(SPI_CS1, SPI_MOSI, SPI_MISO, SPI_SCK);
//std::weak_ptr<Adafruit_MAX31856> weak1 = share1;

#endif

#if LC_MODE
HX711 loadCellA;
HX711 loadCellB;
HX711 loadCellC;
File loadCellFile;
const char* loadCellAFileName {"Load Cell A Data.txt"};
const char* loadCellBFileName {"Load Cell B Data.txt"};
const char* loadCellCFileName {"Load Cell C Data.txt"};
float calibrationFactor {-3200.0f};
std::array<HX711*, NUM_LC> arrayOfLoadCells {&loadCellA, &loadCellB, &loadCellC};
//std::array<HX711*, 1> arrayOfLoadCells {&loadCellA};

#endif

#if SD_MODE
/*!
  @brief
  Initialize SD card. If an SD card is missing, insert one as it's running to see if it reinitializes.
  @return
  void.
*/
void initSD()
{
#if SERIAL_MODE
  Serial.println("Initializing SD card");
#endif
  if(!SD.begin(BUILTIN_SDCARD))
  {
#if SERIAL_MODE
    Serial.println("Could not initilize SD card");
#endif
    while (1)
    {
      // Try this: insert SD mid run and see if it breaks out of loop
      Serial.println("Reinitializing...");
      if(SD.begin(BUILTIN_SDCARD))
      {
        break;
      }
      delay(1000);
    }
  }

/*
  while(!SD.begin(BUILTIN_SDCARD))
  {
#if SERIAL_MODE
    Serial.println("Could not initilize SD card");
#endif

    // Experimental: insert SD mid run and see if it breaks out of loop
    if(SD.begin(BUILTIN_SDCARD))
    {
      break;
    }
  }
*/

#if SERIAL_MODE
  Serial.println("SD card initialized sucessfully");
#endif
}
#endif

#if TC_MODE
/*!
  @brief
  Initialize and set up thermocouple and write stuff to a .txt file.
  @returns
  void
*/
void initThermocouple(Adafruit_MAX31856& thermocouple, max31856_thermocoupletype_t TCType = MAX31856_TCTYPE_K,
                                                       max31856_noise_filter_t NoiseFilter = MAX31856_NOISE_FILTER_60HZ,
                                                       max31856_conversion_mode_t conversionMode = MAX31856_CONTINUOUS)
{
  if(!thermocouple.begin())
  {
#if SERIAL_MODE
    Serial.print("Thermocouple sensor");
    Serial.print(thermocouple.name.c_str());
    Serial.println(" could not be initialized...");
#endif
    while (1)
    {
    };
  }
  thermocouple.setThermocoupleType(TCType);
  thermocouple.setNoiseFilter(NoiseFilter); // hmmmmmmmmmmmmmmm is this needed?
  thermocouple.setConversionMode(conversionMode);
#if SERIAL_MODE
  Serial.print("Thermocouple ");
  Serial.print(thermocouple.name.c_str());
  Serial.print(" type: ");
  switch (thermocouple.getThermocoupleType())
  {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x32 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
#endif
  
  // SD Stuff
#if SD_MODE
  SD.remove(thermocouple.saveFileName.c_str());
  thermocoupleFile = SD.open(thermocouple.saveFileName.c_str(), FILE_WRITE);
  thermocoupleFile.println("MAX31856 thermocouple sensor");
  thermocoupleFile.print("Thermocouple type: ");

  switch (thermocouple.getThermocoupleType())
  {
    case MAX31856_TCTYPE_B: thermocoupleFile.println("B Type"); break;
    case MAX31856_TCTYPE_E: thermocoupleFile.println("E Type"); break;
    case MAX31856_TCTYPE_J: thermocoupleFile.println("J Type"); break;
    case MAX31856_TCTYPE_K: thermocoupleFile.println("K Type"); break;
    case MAX31856_TCTYPE_N: thermocoupleFile.println("N Type"); break;
    case MAX31856_TCTYPE_R: thermocoupleFile.println("R Type"); break;
    case MAX31856_TCTYPE_S: thermocoupleFile.println("S Type"); break;
    case MAX31856_TCTYPE_T: thermocoupleFile.println("T Type"); break;
    case MAX31856_VMODE_G8: thermocoupleFile.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: thermocoupleFile.println("Voltage x32 Gain mode"); break;
    default: thermocoupleFile.println("Unknown"); break;
  }

  thermocoupleFile.print("Time that it takes the IMU to begin measuring since powered on: ");
  thermocoupleFile.print(millis()/1000);
  thermocoupleFile.println("s");
  thermocoupleFile.println("Time(s), Temperature(C)");
  thermocoupleFile.close();
#endif
}
#endif

#if LC_MODE
/*!
  @brief 
  Initialize and setup load cell. If fails, it will try to reinitialize every 0.25 seconds up to 100 times
  @return
  void.
*/
void initLoadCell(HX711& loadCell, uint16_t DOUT, uint16_t SCK)
{
  loadCell.begin(DOUT, SCK);

  if (!loadCell.wait_ready_retry(100,250))
  {
    // this will execute after 25 seconds
#if SERIAL_MODE
    Serial.println("Timedout...");
    Serial.print("Could not initialize load cell sensor");
    Serial.println(loadCell.name.c_str());
#endif
    while (1) 
    {

    };
  }
  loadCell.set_scale();
  loadCell.tare(); //Reset the scale to 0

  long zero_factor = loadCell.read_average(); //Get a baseline reading
#if SERIAL_MODE
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  Serial.println("-----------------------------");
#endif

  // SD STUFF
#if LC_MODE
  SD.remove(loadCell.saveFileName.c_str());
  loadCellFile = SD.open(loadCell.saveFileName.c_str(), FILE_WRITE);
  loadCellFile.println("HX711 load cell sensor");
  loadCellFile.print("Zero factor: ");
  loadCellFile.println(zero_factor);
  loadCellFile.print("Time that it takes the IMU to begin measuring since powered on: ");
  loadCellFile.println(millis()/1000);
  loadCellFile.println("Time(s), Weight(lbs)");
  loadCellFile.close();
#endif

}
#endif

void setup()
{
  // Are these needed?
  //Wire.begin();
  //SPI.begin();
#if SERIAL_MODE
  Serial.begin(115200);
  while (!Serial) delay(10);
#endif

#if SD_MODE
  initSD();
#endif 

  // TC STUFF
#if TC_MODE
  thermocoupleA.name = "A";
  thermocoupleB.name = "B";
  thermocoupleC.name = "C";
  thermocoupleA.saveFileName = thermocoupleAFileName;
  thermocoupleB.saveFileName = thermocoupleBFileName;
  thermocoupleC.saveFileName = thermocoupleCFileName;
  initThermocouple(thermocoupleA);
  initThermocouple(thermocoupleB);
  initThermocouple(thermocoupleC);
  
#endif 

  // LC STUFF
#if LC_MODE
  loadCellA.name = "A";
  loadCellB.name = "B";
  loadCellC.name = "C";
  loadCellA.saveFileName = loadCellAFileName;
  loadCellB.saveFileName = loadCellBFileName;
  loadCellC.saveFileName = loadCellCFileName;
  initLoadCell(loadCellA,LC_DOUT1,LC_SCK);
  initLoadCell(loadCellB,LC_DOUT2,LC_SCK);
  initLoadCell(loadCellC,LC_DOUT3,LC_SCK);
#if SERIAL_MODE
  Serial.println("HX711 calibration sketch (Apply to all load cells, make sure they \
                  are the same model or something)");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
#endif
#endif

  // Configuring the CS pins for the thermocouples and deselect them
  pinMode(SPI_CS1, OUTPUT);
  pinMode(SPI_CS2, OUTPUT);
  pinMode(SPI_CS3, OUTPUT);
  digitalWrite(SPI_CS1, HIGH);
  digitalWrite(SPI_CS2, HIGH);
  digitalWrite(SPI_CS3, HIGH);

  currTime = millis();
  offset = currTime;
#if SERIAL_MODE
  Serial.println("");
  Serial.println("delay.........");
#endif
  //delay(10000000); // Test Setup first ///////////////////////////////////////////////////////////////////////////////////
}

void loop() 
{
  currTime = millis();
  if (currTime - prevTime >= 1000.00/(SAMPLES_PER_SECONDS*120))
  {
#if SERIAL_MODE
  Serial.println("--------------------------------");
  Serial.print("Time: ");
  Serial.print((currTime-offset)/1000);
  Serial.println("s");
#endif
  // TC STUFF
#if TC_MODE
  for (auto thermocouple_ptr: arrayOfThermocouples)
  { // Test with boiling water maybe?
    digitalWrite(thermocouple_ptr->CS_PIN, LOW);
#if SD_MODE
    thermocoupleFile = SD.open(thermocouple_ptr->saveFileName.c_str(), FILE_WRITE);
    thermocoupleFile.print((currTime-offset)/1000);
    thermocoupleFile.print(",");
    thermocoupleFile.println(thermocouple_ptr->readThermocoupleTemperature(), 2);
    thermocoupleFile.close();
#endif
#if SERIAL_MODE
    Serial.print("Thermocouple ");
    Serial.print(thermocouple_ptr->name.c_str());
    Serial.print(" Type ");
    Serial.print(thermocouple_ptr->getThermocoupleType());
    Serial.println(":");
    Serial.print("Hot Junction Temperature: ");
    Serial.print(thermocouple_ptr->readThermocoupleTemperature());
    Serial.println("C");
    Serial.print("Cold Junction Temperature: ");
    Serial.print(thermocouple_ptr->readCJTemperature());
    Serial.println("C\r\n");
    digitalWrite(thermocouple_ptr->CS_PIN, HIGH);
#endif
  }
#endif

  // LC STUFF
#if LC_MODE
  for (auto loadCell_ptr: arrayOfLoadCells)
  {
  loadCell_ptr->set_scale(calibrationFactor);
#if SD_MODE
  loadCellFile = SD.open(loadCell_ptr->saveFileName.c_str(), FILE_WRITE);
  loadCellFile.print((currTime-offset)/1000);
  loadCellFile.print(",");
  loadCellFile.println(loadCell_ptr->get_units(), 2);
  // or loadcel_ptr->get_value()??????
  loadCellFile.close();
#endif
#if SERIAL_MODE
  Serial.print("Load Cell ");
  Serial.print(loadCell_ptr->name.c_str());
  Serial.print(" Reading: ");
  Serial.print(loadCell_ptr->get_units(), 2);
  Serial.println("lbs");

  //Serial.printf("Load Cell %s Reading: %.2flbs \r\n", loadCell_ptr->name.c_str(), loadCell_ptr->get_units());

#endif
  }
#if SERIAL_MODE
  Serial.print("Load cell calibration factor: ");
  Serial.println(calibrationFactor);

  if (Serial.available() > 0)
    {
      char temp = Serial.read();
      if(temp == '+' || temp == 'a')
        calibrationFactor += 10;
      else if(temp == '-' || temp == 'z')
        calibrationFactor -= 10;
    }

  /*
  if (Serial.available() > 0)
    {
      byte buffer[8];
      int64_t temp = Serial.readBytesUntil('\n',buffer,sizeof(buffer)); // seems to return the number of bytes,
                                                                        // not the actual value.
      calibrationFactor = temp;
    }
  */

#endif
#endif
  prevTime = currTime;
  }
}
