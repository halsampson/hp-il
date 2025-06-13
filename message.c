// HP-IL message table derived from https://github.com/gIzFer/hpil

enum ControlCode {DABcc, SRQ, ENDcc, END_SRQ,  CMD, RDY, IDYcc, IDY_SRQ};

typedef struct {
	ControlCode frameControl;
	uint8_t frameData;
	uint8_t paramBits; // mask
} Frame;


//Data Or End class
const Frame DAB = {DABcc, 0, 0b1111111}; // DAta Byte
const Frame END = {ENDcc, 0, 0b1111111}; // data END

//CoMmanD class

//Addressed Command subGroup
const Frame NUL = {CMD, 0, 0}; // NULl Frame
const Frame GTL = {CMD, 0b00000001, 0}; // Go To Local
const Frame SDC = {CMD, 0b00000100, 0}; // Selected Device Clear
const Frame PPD = {CMD, 0b00000101, 0}; // Parallel Poll Disable
const Frame GET = {CMD, 0b00001000, 0}; // Group Execute Trigger
const Frame ELN = {CMD, 0b00001111, 0}; // Enable Listener Not ready
const Frame PPE = {CMD, 0b10000000, 0b111}; // Parallel Poll Enable
const Frame DDL = {CMD, 0b10100000, 0b11111}; // Device Dependent Listener
const Frame DDT = {CMD, 0b11000000, 0b11111}; // Device Dependent Talker

//Universal Command subGroup
const Frame NOP = {CMD, 0b00010000, 0}; // No OPeration
const Frame LLO = {CMD, 0b00010001, 0}; // Local LOckout
const Frame DCL = {CMD, 0b00010100, 0}; // Device CLear
const Frame PPU = {CMD, 0b00010101, 0}; // Parallel Poll Unconfigure
const Frame EAR = {CMD, 0b00011000, 0}; // Enable Asynchronous Requests
const Frame IFC = {CMD, 0b10010000, 0}; // InterFace Clear
const Frame REN = {CMD, 0b10010010, 0}; // Remote ENable
const Frame NRE = {CMD, 0b10010011, 0}; // Not Remote ENable
const Frame AAU = {CMD, 0b10011010, 0}; // Auto Address Unconfigure
const Frame LPD = {CMD, 0b10011011, 0}; // Loop Power Down

//Listen Address subGroup
const Frame LAD = {CMD, 0b00100000, 0b11111}; // Listen ADdress
const Frame MLA = {CMD, 0b00100000, 0b11111}; // My Listen Address
const Frame UNL = {CMD, 0b00111111, 0}; //UNListen

//Talk Address subGroup
const Frame TAD = {CMD, 0b01000000, 0b11111}; // Talk ADdress
const Frame MTA = {CMD, 0b01000000, 0b11111}; // My Talk Address
const Frame OTA = {CMD, 0b01000000, 0b11111}; // Other Talk Address
const Frame UNT = {CMD, 0b01011111, 0}; // UNTalk

//Secondary Address subGroup
const Frame SAD = {CMD, 0b01100000, 0b11111}; // Secondary ADdress
const Frame MSA = {CMD, 0b01100000, 0b11111}; // My Secondary Address
const Frame OSA = {CMD, 0b01100000, 0b11111}; // Other Secondary Address

//IDentifY class
const Frame IDY = {IDYcc, 0, 0};

//ReaDY class
const Frame RFC = {RDY, 0, 0}; // Ready For Command

//Addressed Ready subGroup
//End Of Transmission subSubGroup
const Frame EOT = {RDY, 0b01000000, 0}; // End of Transmission
const Frame ETO = {RDY, 0b01000000, 0}; // End of Transmission - Ok
const Frame ETE = {RDY, 0b01000001, 0}; // End of Transmission - Error
const Frame NRD = {RDY, 0b01000010, 0}; // Not Ready for Data

//Start Of Transmission subSubGroup
const Frame SDA = {RDY, 0b01100000, 0}; // Send DAta
const Frame SST = {RDY, 0b01100001, 0}; // Send STatus
const Frame SDI = {RDY, 0b01100010, 0}; // Send Device Id
const Frame SAI = {RDY, 0b01100011, 0}; // Send Accessory Id
const Frame TCT = {RDY, 0b01100100, 0}; // Take ConTrol

//Auto Address subGroup
const Frame AAD = {RDY, 0b10000000, 0b11111}; // Auto ADdress
const Frame NAA = {RDY, 0b10000000, 0b11111}; // Next Auto Address
const Frame IAA = {RDY, 0b10011111, 0}; // Illegal Auto Address

const Frame AEP = {RDY, 0b10100000, 0b11111}; // Auto Extended Primary
const Frame IEP = {RDY, 0b10111111, 0}; // Illegal Extended Primary

const Frame ZES = {RDY, 0b11000000, 0}; // Zero Extended Secondary
const Frame AES = {RDY, 0b11000000, 0b11111}; // Auto Extended Secondary
const Frame NES = {RDY, 0b11000000, 0b11111}; // Next Extended Secondary
const Frame IES = {RDY, 0b11011111, 0}; // Illegal Extended Secondary

const Frame AMP = {RDY, 0b11100000, 0b11111}; // Auto Multiple Primary
const Frame NMP = {RDY, 0b11100000, 0b11111}; // Next Multiple Primary
const Frame IMP = {RDY, 0b11111111, 0}; // Illegal Multiple Primary

