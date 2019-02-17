
#define DEBUG 1
#include "M3SensorNode.h"
#include "LowPower.h"
#include <RH_ASK.h>

// TODO: error state
// TODO: queue auslagern in bib
// TODO: blocking auslagern in bib
// TODO: build pfade anpassen

#define WASTE_CYCLE __asm__("nop\n\t");

// Speicherbereich
#define EEPROM_ADDRESS 0

// UI-Pins
#define INPUT_PIN 2
#define SETUP_MODE_PIN 3
#define SETUP_LED_PIN 5
#define STATE_LED_PIN 6
#define SEND_LED_PIN 7

#define RANDOM_SEED_PIN 0

// Transmitter
#define TX_FQ 2000
#define TX_PIN 4
#define RX_PIN 13

// Optimierung der Eingabe
#define DEBOUNCE_LOOP_COUNT 50
#define DEBOUNCE_NOPS_COUNT 10
#define SETUP_MODE_PRESS_TIME 10000
#define SEND_IDENT_MODE_PRESS_TIME 1000

// Node-Konfiguration
#define STATE_QUEUE_SIZE 10
#define KEY_SIZE 32
#define HMAC_SIZE 32
#define UNDEF_STATE 0xee

#if DEBUG == 1
void dumpPackage(byte *pkg, int size)
{
    for (int i = 0; i < size; i += 4)
    {
        for (int j = 0; j < 4; j++)
        {
            Serial.print("0x");
            Serial.print(pkg[i + j], HEX);
            Serial.print(" (");
            Serial.print((char)pkg[i + j]);
            if (j == 3)
            {
                Serial.println(")");
            }
            else
            {
                Serial.print(")\t");
            }
        }
    }
}
#endif

byte blockingReadDebouncedValueLoop(int pin, long loopCount, int wasteCount)
{
    long restDebounceCount = loopCount;
    byte readValue = digitalRead(pin);

    while (restDebounceCount-- > 0)
    {
        byte currentValue = digitalRead(pin);
        if (currentValue != readValue)
        {
            restDebounceCount = loopCount;
            readValue = currentValue;
        }

        for (int i = 0; i < wasteCount; i++)
        {
            WASTE_CYCLE
        }
    }

    return readValue;
}

long blockingMeasureStateTime(int pin, long maxTime, int expectedValue)
{
    long startTime = millis();
    while (true)
    {
        long time = millis() - startTime;
        if (time >= maxTime || digitalRead(pin) != expectedValue)
        {
            return time;
        }
    }
}

void blockingFlashLed(int pin, int times, int delayTime, int stateAfterwards)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(SETUP_LED_PIN, HIGH);
        delay(delayTime);
        digitalWrite(SETUP_LED_PIN, LOW);
        delay(delayTime);
    }
    digitalWrite(SETUP_LED_PIN, stateAfterwards);
}

M3SensorNode<1, KEY_SIZE, HMAC_SIZE> node(EEPROM_ADDRESS);
ArrayQueue<STATE_QUEUE_SIZE, byte, UNDEF_STATE> stateQueue;
RH_ASK driver(TX_FQ, RX_PIN, TX_PIN);
volatile bool setupModeRequested = false;

void enqueueSensorState()
{
    noInterrupts();
    byte state = blockingReadDebouncedValueLoop(INPUT_PIN, DEBOUNCE_LOOP_COUNT, DEBOUNCE_NOPS_COUNT);
    stateQueue.enqueue(state);
#if DEBUG == 1
    Serial.print("enqueued state ");
    Serial.println(state, DEC);
    Serial.flush();
#endif
    interrupts();
}
inline void blockingSendData(uint8_t *data, uint8_t len)
{
    digitalWrite(SEND_LED_PIN, HIGH);
    driver.send(data, len);
    driver.waitPacketSent();
    digitalWrite(SEND_LED_PIN, LOW);
}

inline void sendState(byte state)
{
    byte ledValue = !state;
#if DEBUG == 1
    Serial.print("setting led to ");
    Serial.println(ledValue, DEC);
    Serial.flush();
#endif
    digitalWrite(STATE_LED_PIN, ledValue);
    node.setState(0, state);
    node.buildDataPackage();

    blockingSendData((uint8_t *)node.getDataPackage(), node.getDataPackageSize());
    node.persistStateSend();
}

inline void sendIdentity()
{
#if DEBUG == 1
    Serial.println("sende identify:");
    dumpPackage(node.getIdentifyPackage(), node.getIdentifyPackageSize());
    Serial.flush();
#endif

    for (int i = 0; i < 5; i++)
    {
#if DEBUG == 1
        Serial.println(".");
        Serial.flush();
#endif
        blockingSendData((uint8_t *)node.getIdentifyPackage(), node.getIdentifyPackageSize());
    }
}

void requestSetupMode()
{
    setupModeRequested = true;
    detachInterrupt(digitalPinToInterrupt(SETUP_MODE_PIN));
}

void tryToGoToSetupMode()
{
#if DEBUG == 1
    Serial.println("tryToGoToSetupMode");
    Serial.flush();
#endif

    digitalWrite(SETUP_LED_PIN, HIGH);

    long time = blockingMeasureStateTime(SETUP_MODE_PIN, SETUP_MODE_PRESS_TIME, LOW);
#if DEBUG == 1
    Serial.print("got setup time=");
    Serial.println(time, DEC);
    Serial.flush();
#endif
    if (time >= SETUP_MODE_PRESS_TIME)
    {
        node.reset();
        blockingFlashLed(SETUP_LED_PIN, 3, 500, HIGH);
    }

    if (time >= SEND_IDENT_MODE_PRESS_TIME)
    {
        sendIdentity();
    }

    setupModeRequested = false;
    digitalWrite(SETUP_LED_PIN, LOW);
    enqueueSensorState();
}

void setup()
{
#if DEBUG == 1
    Serial.begin(9600);
    Serial.println("start");
#endif

    randomSeed(analogRead(RANDOM_SEED_PIN));

    pinMode(INPUT_PIN, INPUT_PULLUP);
    pinMode(SETUP_MODE_PIN, INPUT_PULLUP);
    pinMode(SETUP_LED_PIN, OUTPUT);
    pinMode(STATE_LED_PIN, OUTPUT);
    pinMode(SEND_LED_PIN, OUTPUT);

    node.initialize();
    driver.init();

    attachInterrupt(digitalPinToInterrupt(INPUT_PIN), enqueueSensorState, CHANGE);

#if DEBUG == 1
    Serial.println("init fertig.");
    Serial.flush();
#endif
}

void loop()
{
#if DEBUG == 1
    Serial.println("sleep mode");
    Serial.flush();
#endif

    attachInterrupt(digitalPinToInterrupt(SETUP_MODE_PIN), requestSetupMode, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

#if DEBUG == 1
    Serial.println("wakeup");
    Serial.flush();
#endif

    if (setupModeRequested)
    {
        tryToGoToSetupMode();
    }

    while (true)
    {
#if DEBUG == 1
        Serial.println("loop");
        Serial.flush();
#endif
        bool hasNoStates = stateQueue.isEmpty();
        if (hasNoStates)
        {
            break;
        }

#if DEBUG == 1
        Serial.print("stateQueue is not empty, has=");
        Serial.print(stateQueue.getSize(), DEC);
        Serial.println(" elements");
        Serial.flush();
#endif
        byte messageState = stateQueue.get();
        noInterrupts();
        stateQueue.deque();
        interrupts();

#if DEBUG == 1
        Serial.print("state to send is ");
        Serial.println(messageState, DEC);
        Serial.flush();
#endif

        if (messageState != UNDEF_STATE)
        {
            sendState(messageState);
        }
    }
}
