#pragma once

#include "mmio.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>



typedef std::map<std::string, uint32_t> paramlist;

// represents an fpga bitstream which can be programmed onto an fpga
class Bitstream {
public:
  std::string bitstream, mainRegion;
  std::vector<std::string> stubRegions;
  int slotCount;
  bool multiSlot;
  Bitstream();
  Bitstream(std::string bits, std::string main, std::vector<std::string> stubs);
  Bitstream(std::string bits, std::string main);
  bool isInstalled();
  void install();
};



// represents an accelerator which can be loaded into the shell regions
class Accel {
public:
  std::string name;                       // name of this accelerator
  std::vector<Bitstream> bitstreams;      // bitstreams with this accelerator
  std::map<std::string, int> registers;   // register -> offset mapping of this accelerator
  Accel();
  Accel(std::string name);
  void addBitstream(const Bitstream &bits);
  void addRegister(std::string name, int offset);
  int getRegister(std::string name);
  void setupSiblings();
  static Accel loadFromJSON(std::string jsonpath);
};


// represents a slot in the shell
class Region {
public:
  std::string name;               // region name
  Bitstream blank;

  long blockerAddr;               // blocker address
  mapped_device blockerDev;       // mapped blocker
  uint8_t *blockerRegs;           // blocker registers

  long periphAddr;                // peripheral address
  mapped_device periphDev;        // mapped peripheral registers
  uint32_t *periphRegs;           // mapped peripheral regs (u32)

  bool mapped;                    // is the blocker and peripheral mmap-ped?

  Accel *accel;                   // currently (or last) loaded accelerator
  Bitstream *bitstream;           // currently (or last) loaded bitstream
  bool stub;                      // this accelerator is contolled elsewhere
  bool locked;                    // accelerator is in use

  // uninintialised region
  Region();
  Region(std::string name, std::string blankName, long blocker, long address);
  ~Region();

  // copy constructo and assign operato is deleteo
  Region(const Region& a) = delete;
  Region& operator=(const Region& a) = delete;

  // moove constructo and moove assignment
  Region(Region&& a);
  Region& operator=(Region&& a);

  // mmap blockers and peripheral address ranges
  void mapDevs();
  void unmapDevs();

  void setBlock(bool status);
  bool canElideLoad(Bitstream &bs);
  void loadAccel(Accel &acc, Bitstream &bs);
  void loadStub(Accel &acc, Bitstream &bs);
  void unloadAccel();
};

class AccelInst {
public:
  Accel *accel;             // accelerator
  Bitstream *bitstream;     // bitstream loaded
  Region *region;           // controlling region

  void programAccel(paramlist &regvals);
  void runAccel();
  bool running();
  void wait();
};


// manages the fpga, its regions, accelerators, and jobs
class PRManager {
public:
  std::map<std::string, Region> regions;  // shell regions
  std::map<std::string, Accel> accels;    // loadable accelerators
  // std::vector<AccelInst> jobs;            // running accelerators

  PRManager();

  void fpgaYeet(Accel& acc, Bitstream &bs);
  void fpgaUnload(AccelInst &inst);
  AccelInst fpgaRun(std::string accname, paramlist &regvals);

  // check if regions used by a bitstream are free and cached
  bool canQuickLoadBitstream(Bitstream &bs);
  bool canLoadBitstream(Bitstream &bs);

  void loadAccel(std::string name);
  void loadShell(std::string name);

  // sets up initial datastructures with bitstream info
  void loadDefs();
};