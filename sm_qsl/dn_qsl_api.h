//=========================== typedef =========================================


//=========================== prototypes ======================================
//==== API

bool dn_qsl_init(void);
bool dn_qsl_isConnected(void);
bool dn_qsl_connect(uint16_t netID, uint8_t* joinKey, uint32_t service_ms);
bool dn_qsl_send(uint8_t* payload, uint8_t payloadSize_B, uint8_t* destIP);
uint8_t dn_qsl_read(uint8_t* payloadBuffer);
