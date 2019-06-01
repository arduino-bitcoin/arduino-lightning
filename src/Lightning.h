#ifndef __LIGHTNING_H__BDDNDVJ300
#define __LIGHTNING_H__BDDNDVJ300

#include <Arduino.h>
#include <Bitcoin.h>

class LightningInvoice : public Printable{
private:
	uint8_t * buffer = NULL;
	size_t bufLen = 0;
public:
	LightningInvoice();
	LightningInvoice(const char * invoice);
	LightningInvoice(const char * description, const uint8_t preimage[32], uint32_t time,
					uint32_t value = 0, char mult = ' ', 
					bool use_testnet = false);
	// LightningInvoice(const LightningInvoice &other);
	~LightningInvoice();
	uint32_t amount = 0;
	char multiplier = ' ';
	bool testnet = false;
	Signature sig;
	int hash(uint8_t hashArr[32]);
	Signature sign(const PrivateKey pk);
	uint32_t timestamp() const;
	String description() const;
	int addField(uint8_t code, const uint8_t * data, uint16_t dataLen);
	int setExpiry(uint32_t expiry);
	int addRawData(const uint8_t * data, size_t dataLen);
        int addRoutingInfo(uint8_t pubkey[33], uint8_t short_channel_id[8], uint32_t fee_base_msat, uint32_t fee_proportional_millionths, uint16_t cltv_expiry_delta);
	int hmr(char * arr, size_t arrSize) const;
	size_t printTo(Print &p) const;
	size_t toCharArray(char * arr, size_t arrSize) const;
    // LightningInvoice &operator=(LightningInvoice const &other);
};

#endif // __LIGHTNING_H__BDDNDVJ300
