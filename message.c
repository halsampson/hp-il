// HP-IL message table derived from https://github.com/gIzFer/hpil

enum ControlCode {DABcc, DAB_SRQ, ENDcc, END_SRQ,  CMD, RDY, IDYcc, IDY_SRQ};

#define SRQ 1
#define EOI 2

typedef struct {  // in big endian transfer order: 3 frame control bits first
  uint8_t frameData; 
  ControlCode frameControl;
} XferFrame;

typedef struct {
	ControlCode frameControl;
	uint8_t frameData;
	uint8_t paramBits; // mask
} Frame;


//Data Or End class
const Frame DAB = {DABcc, 0, 0xFF}; // DAta Byte
const Frame END = {ENDcc, 0, 0xFF}; // data END

//CoMmanD class

//Addressed Command subGroup
const Frame NUL = {CMD, 0, 0}; // NULl Frame
const Frame GTL = {CMD, 0x01, 0}; // Go To Local
const Frame SDC = {CMD, 0x04, 0}; // Selected Device Clear
const Frame PPD = {CMD, 0x05, 0}; // Parallel Poll Disable
const Frame GET = {CMD, 0x08, 0}; // Group Execute Trigger
const Frame ELN = {CMD, 0x0F, 0}; // Enable Listener Not ready
const Frame PPE = {CMD, 0x80, 7}; // Parallel Poll Enable
const Frame DDL = {CMD, 0xA0, 31}; // Device Dependent Listener
const Frame DDT = {CMD, 0xC0, 31}; // Device Dependent Talker

//Universal Command subGroup
const Frame NOP = {CMD, 0x10, 0}; // No OPeration
const Frame LLO = {CMD, 0x11, 0}; // Local LOckout
const Frame DCL = {CMD, 0x14, 0}; // Device CLear
const Frame PPU = {CMD, 0x15, 0}; // Parallel Poll Unconfigure
const Frame EAR = {CMD, 0x18, 0}; // Enable Asynchronous Requests
const Frame IFC = {CMD, 0x90, 0}; // InterFace Clear
const Frame REN = {CMD, 0x92, 0}; // Remote ENable
const Frame NRE = {CMD, 0x93, 0}; // Not Remote ENable
const Frame AAU = {CMD, 0x9A, 0}; // Auto Address Unconfigure
const Frame LPD = {CMD, 0x9B, 0}; // Loop Power Down

//Listen Address subGroup
const Frame LAD = {CMD, 0x20, 31}; // Listen ADdress
const Frame MLA = {CMD, 0x20, 31}; // My Listen Address
const Frame UNL = {CMD, 0x3F, 0}; //UNListen

//Talk Address subGroup
const Frame TAD = {CMD, 0x40, 31}; // Talk ADdress
const Frame MTA = {CMD, 0x40, 31}; // My Talk Address
const Frame OTA = {CMD, 0x40, 31}; // Other Talk Address
const Frame UNT = {CMD, 0x5F, 0}; // UNTalk

//Secondary Address subGroup
const Frame SAD = {CMD, 0x60, 31}; // Secondary ADdress
const Frame MSA = {CMD, 0x60, 31}; // My Secondary Address
const Frame OSA = {CMD, 0x60, 31}; // Other Secondary Address

//IDentifY class
const Frame IDY = {IDYcc, 0, 0};

//ReaDY class
const Frame RFC = {RDY, 0, 0}; // Ready For Command

//Addressed Ready subGroup
//End Of Transmission subSubGroup
const Frame EOT = {RDY, 0x40, 0}; // End of Transmission
const Frame ETO = {RDY, 0x40, 0}; // End of Transmission - Ok
const Frame ETE = {RDY, 0x41, 0}; // End of Transmission - Error
const Frame NRD = {RDY, 0x42, 0}; // Not Ready for Data

//Start Of Transmission subSubGroup
const Frame SDA = {RDY, 0x60, 0}; // Send DAta
const Frame SST = {RDY, 0x61, 0}; // Send STatus
const Frame SDI = {RDY, 0x62, 0}; // Send Device Id
const Frame SAI = {RDY, 0x63, 0}; // Send Accessory Id
const Frame TCT = {RDY, 0x64, 0}; // Take ConTrol

//Auto Address subGroup
const Frame AAD = {RDY, 0x80, 31}; // Auto ADdress
const Frame NAA = {RDY, 0x80, 31}; // Next Auto Address
const Frame IAA = {RDY, 0x9F, 0}; // Illegal Auto Address

const Frame AEP = {RDY, 0xA0, 31}; // Auto Extended Primary
const Frame IEP = {RDY, 0xBF, 0}; // Illegal Extended Primary

const Frame ZES = {RDY, 0xC0, 0}; // Zero Extended Secondary
const Frame AES = {RDY, 0xC0, 31}; // Auto Extended Secondary
const Frame NES = {RDY, 0xC0, 31}; // Next Extended Secondary
const Frame IES = {RDY, 0xDF, 0}; // Illegal Extended Secondary

const Frame AMP = {RDY, 0xE0, 31}; // Auto Multiple Primary
const Frame NMP = {RDY, 0xE0, 31}; // Next Multiple Primary
const Frame IMP = {RDY, 0xFF, 0}; // Illegal Multiple Primary

