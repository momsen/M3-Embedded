#include <ArrayQueue.h>
#include <blocking.h>
#include <HwInfoEEPROM.h>

#include <Battery.h>
#include <M3SensorNode.h>
#include <LowPower.h>
#include <PinChangeInterrupt.h>
#include <RH_ASK.h>

// Speicherbereich
#define EEPROM_ADDRESS_HWINF 0
#define EEPROM_ADDRESS_M3 HWINFO_EEPROM_DATA_NUM_BYTES

// PINS
#define SETUP_INT_PIN 2

#define SENSOR1_PIN 7
#define SENSOR2_PIN 8

#define SETUP_LED_PIN 5
#define SEND_LED_PIN 6

#define BATTERY_VOLTAGE_PIN A1
#define RANDOM_SEED_PIN A0

#define TX_PIN 4
#define RX_PIN 13

// Transmitterkonfiguration
#define TX_FQ 2000

// Optimierung der Eingabe
#define SETUP_MODE_PRESS_TIME 10000
#define SEND_IDENT_MODE_PRESS_TIME 1000

// Node-Konfiguration
#define STATE_QUEUE_SIZE 20
#define KEY_SIZE 32
#define HMAC_SIZE 32
#define UNDEF_STATE 0xee
#define NUM_8S_SLEEPS 75 // 10 Minuten (10 * 60 / 8)

Battery battery(950, 1500, A1);
m3::M3SensorNode<3, KEY_SIZE, HMAC_SIZE> m3Node(EEPROM_ADDRESS_M3);
utils::ArrayQueue<STATE_QUEUE_SIZE, byte, UNDEF_STATE> sensor1StateQueue;
utils::ArrayQueue<STATE_QUEUE_SIZE, byte, UNDEF_STATE> sensor2StateQueue;
RH_ASK transmitter(TX_FQ, RX_PIN, TX_PIN);
volatile bool setupModeRequested = false;
volatile bool sensorIsr = false;

inline void enqueueSensorStates()
{
    byte state1 = digitalRead(SENSOR1_PIN);
    if (sensor1StateQueue.isEmpty() || sensor1StateQueue.get() != state1)
    {
        sensor1StateQueue.enqueue(state1);
    }

    byte state2 = digitalRead(SENSOR2_PIN);
    if (sensor2StateQueue.isEmpty() || sensor2StateQueue.get() != state2)
    {
        sensor2StateQueue.enqueue(state2);
    }
}

void isrEnqueueSensorStates()
{
    enqueueSensorStates();
    sensorIsr = true;
}

inline void blockingSendData(uint8_t *data, uint8_t len)
{
    digitalWrite(SEND_LED_PIN, HIGH);

    uint8_t bytesSent = 0;
    while (bytesSent < len)
    {
        uint8_t bytesInThisPackage = min(len - bytesSent, RH_ASK_MAX_MESSAGE_LEN);
        transmitter.send(data + bytesSent, bytesInThisPackage);
        transmitter.waitPacketSent();
        bytesSent += bytesInThisPackage;
    }

    digitalWrite(SEND_LED_PIN, LOW);
}

inline void sendState()
{
    m3Node.setBattery(battery.voltage(), battery.level());
    m3Node.buildDataPackage();
    blockingSendData((uint8_t *)m3Node.getDataPackage(), m3Node.getDataPackageSize());
    m3Node.persistStateSend();
}

inline void sendIdentity()
{
    for (int i = 0; i < 10; i++)
    {
        m3Node.setBattery(battery.voltage(), battery.level());
        blockingSendData((uint8_t *)m3Node.getIdentifyPackage(), m3Node.getIdentifyPackageSize());
        delay(500);
    }
}

void isrRequestSetupMode()
{
    setupModeRequested = true;
}

inline void executeSetupRequest()
{
    digitalWrite(SETUP_LED_PIN, HIGH);

    long time = utils::blockingMeasureStateTime(SETUP_INT_PIN, SETUP_MODE_PRESS_TIME, HIGH);
    if (time >= SETUP_MODE_PRESS_TIME)
    {
        m3Node.reset();
        utils::blockingFlashLed(SETUP_LED_PIN, 3, 500, HIGH);
    }

    if (time >= SEND_IDENT_MODE_PRESS_TIME)
    {
        sendIdentity();
    }

    noInterrupts();
    enqueueSensorStates();
    interrupts();

    digitalWrite(SETUP_LED_PIN, LOW);
    setupModeRequested = false;
}

void errorMode(byte errorCode)
{
    digitalWrite(SETUP_LED_PIN, HIGH);

    while (true)
    {
        for (int i = 0; i < errorCode; i++)
        {
            digitalWrite(SEND_LED_PIN, HIGH);
            delay(250);
            digitalWrite(SEND_LED_PIN, LOW);
            delay(250);
        }
    }
}

void setup()
{
    randomSeed(analogRead(RANDOM_SEED_PIN));

    pinMode(SENSOR1_PIN, INPUT);
    pinMode(SENSOR2_PIN, INPUT);
    pinMode(SETUP_INT_PIN, INPUT);
    pinMode(SETUP_LED_PIN, OUTPUT);
    pinMode(SEND_LED_PIN, OUTPUT);

    if (!m3Node.initialize())
    {
        errorMode(0);
    }

    if (!transmitter.init())
    {
        errorMode(1);
    }

    utils::HwInfoEEPROM hw(EEPROM_ADDRESS_HWINF);
    if (hw.load())
    {
        battery.begin(hw.getVcc(), 1.0);
    }
    else
    {
        battery.begin(5000, 1.0);
    }

    attachPCINT(digitalPinToPCINT(SENSOR1_PIN), isrEnqueueSensorStates, CHANGE);
    attachPCINT(digitalPinToPCINT(SENSOR2_PIN), isrEnqueueSensorStates, CHANGE);
    attachInterrupt(digitalPinToInterrupt(SETUP_INT_PIN), isrRequestSetupMode, HIGH);
}

void loop()
{
    if (setupModeRequested)
    {
        executeSetupRequest();
    }

    while (!sensor1StateQueue.isEmpty() || !sensor2StateQueue.isEmpty())
    {
        m3Node.setState(0, battery.level());

        if (!sensor1StateQueue.isEmpty())
        {
            byte state1 = sensor1StateQueue.get();

            noInterrupts();
            sensor1StateQueue.deque();
            interrupts();

            if (state1 != UNDEF_STATE)
            {
                m3Node.setState(1, state1);
            }
        }

        if (!sensor2StateQueue.isEmpty())
        {
            byte state2 = sensor2StateQueue.get();

            noInterrupts();
            sensor2StateQueue.deque();
            interrupts();

            if (state2 != UNDEF_STATE)
            {
                m3Node.setState(2, state2);
            }
        }

        sendState();
    }

    sensorIsr = false;
    int numSleeps = NUM_8S_SLEEPS;
    while (numSleeps-- > 0)
    {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        if (sensorIsr || setupModeRequested)
        {
            break;
        }
    }

    if (numSleeps <= 0)
    {
        enqueueSensorStates();
    }
}
