#include <ArrayQueue.h>
#include <blocking.h>

#include <M3SensorNode.h>
#include <LowPower.h>
#include <RH_ASK.h>

// TODO: build pfade anpassen
// TODO: batterie stand auslesen und mitsenden
// TODO: rtc=>aufwecken und alle 10 minuten zustand senden (einfach neuen change-pin definieren ohne request setup)

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

m3::M3SensorNode<1, KEY_SIZE, HMAC_SIZE> m3Node(EEPROM_ADDRESS);
utils::ArrayQueue<STATE_QUEUE_SIZE, byte, UNDEF_STATE> sensoreStateQueue;
RH_ASK transmitter(TX_FQ, RX_PIN, TX_PIN);
volatile bool setupModeRequested = false;

inline void enqueueSensorState()
{
    noInterrupts();
    byte state = utils::blockingReadDebouncedValueLoop(INPUT_PIN, DEBOUNCE_LOOP_COUNT, DEBOUNCE_NOPS_COUNT);
    sensoreStateQueue.enqueue(state);
    interrupts();
}

void isrEnqueueSensorState()
{
    enqueueSensorState();
}

inline void blockingSendData(uint8_t *data, uint8_t len)
{
    digitalWrite(SEND_LED_PIN, HIGH);

    transmitter.send(data, len);
    transmitter.waitPacketSent();

    digitalWrite(SEND_LED_PIN, LOW);
}

inline void sendState(byte state)
{
    byte ledValue = !state;
    digitalWrite(STATE_LED_PIN, ledValue);

    m3Node.setState(0, state);
    m3Node.buildDataPackage();
    blockingSendData((uint8_t *)m3Node.getDataPackage(), m3Node.getDataPackageSize());

    m3Node.persistStateSend();
}

inline void sendIdentity()
{
    for (int i = 0; i < 5; i++)
    {
        blockingSendData((uint8_t *)m3Node.getIdentifyPackage(), m3Node.getIdentifyPackageSize());
        delay(500);
    }
}

void isrRequestSetupMode()
{
    setupModeRequested = true;
    detachInterrupt(digitalPinToInterrupt(SETUP_MODE_PIN));
}

void tryToGoToSetupMode()
{
    digitalWrite(SETUP_LED_PIN, HIGH);

    long time = utils::blockingMeasureStateTime(SETUP_MODE_PIN, SETUP_MODE_PRESS_TIME, LOW);
    if (time >= SETUP_MODE_PRESS_TIME)
    {
        m3Node.reset();
        utils::blockingFlashLed(SETUP_LED_PIN, 3, 500, HIGH);
    }

    if (time >= SEND_IDENT_MODE_PRESS_TIME)
    {
        sendIdentity();
    }

    enqueueSensorState();

    digitalWrite(SETUP_LED_PIN, LOW);
    setupModeRequested = false;
}

void errorMode(byte errorCode)
{
    digitalWrite(SETUP_LED_PIN, errorCode & 1);
    digitalWrite(STATE_LED_PIN, errorCode & 2);

    while (true)
    {
        digitalWrite(SEND_LED_PIN, HIGH);
        delay(250);
        digitalWrite(SEND_LED_PIN, LOW);
        delay(250);
    }
}

void setup()
{
    randomSeed(analogRead(RANDOM_SEED_PIN));

    pinMode(INPUT_PIN, INPUT_PULLUP);
    pinMode(SETUP_MODE_PIN, INPUT_PULLUP);
    pinMode(SETUP_LED_PIN, OUTPUT);
    pinMode(STATE_LED_PIN, OUTPUT);
    pinMode(SEND_LED_PIN, OUTPUT);

    if (!m3Node.initialize())
    {
        errorMode(1);
    }

    if (!transmitter.init())
    {
        errorMode(2);
    }

    attachInterrupt(digitalPinToInterrupt(INPUT_PIN), isrEnqueueSensorState, CHANGE);
}

void loop()
{
    if (setupModeRequested)
    {
        tryToGoToSetupMode();
    }

    while (!sensoreStateQueue.isEmpty())
    {
        byte messageState = sensoreStateQueue.get();

        noInterrupts();
        sensoreStateQueue.deque();
        interrupts();

        if (messageState != UNDEF_STATE)
        {
            sendState(messageState);
        }
    }

    attachInterrupt(digitalPinToInterrupt(SETUP_MODE_PIN), isrRequestSetupMode, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
