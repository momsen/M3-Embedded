#ifndef M3_SENSOR_NODE_H__
#define M3_SENSOR_NODE_H__

#include <Crypto.h>
#include <SHA256.h>

#include <M3Protocol.h>
#include <M3EEPROM.h>

namespace m3
{

template <StateSize S, KeySize K, HmacSize H>
class M3SensorNode
{
public:
  M3SensorNode(int eepromAddress) : persistentState(eepromAddress) {}

  bool initialize()
  {
    if (!persistentState.load())
    {
      reset();
    }
    else
    {
      identifyPackage.initialize(persistentState.getNodeId(), Sensor, persistentState.getKey());
      dataPackage.initialize(persistentState.getNodeId(), persistentState.getLastWrittenPackageNumber());
    }

    return true;
  }

  void buildDataPackage()
  {
    dataPackage.pkgId++;
    memset(&(dataPackage.hmac), 0, H);

    hmac.resetHMAC(identifyPackage.key, K);
    hmac.update((byte *)&dataPackage, sizeof(dataPackage));
    hmac.finalizeHMAC(identifyPackage.key, K, dataPackage.hmac, H);
  }

  void setState(int index, byte value)
  {
    if (index >= 0 && index < S)
    {
      dataPackage.state[index] = value;
    }
  }

  void persistStateSend()
  {
    persistentState.updatePkgNumber(dataPackage.pkgId);
  }

  byte *getIdentifyPackage()
  {
    return (byte *)&(identifyPackage);
  }

  int getIdentifyPackageSize()
  {
    return sizeof(identifyPackage);
  }

  byte *getDataPackage()
  {
    return (byte *)&(dataPackage);
  }

  int getDataPackageSize()
  {
    return sizeof(dataPackage);
  }

  void reset()
  {
    NodeId id = random();

    byte newKey[K];
    for (KeySize i = 0; i < K; i++)
    {
      newKey[i] = (byte)random(0, 256);
    }

    persistentState.reset(id, newKey);
    identifyPackage.initialize(id, Sensor, newKey);
    dataPackage.initialize(id, 0);
  }

private:
  SHA256 hmac;
  IdentifyPackage<S, K, H> identifyPackage;
  DataPackage<S, H> dataPackage;
  M3NodeEEPROM<K> persistentState;
};

} // namespace m3

#endif