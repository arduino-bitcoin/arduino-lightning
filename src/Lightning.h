#ifndef __LIGHTNING_H__BDDNDVJ300
#define __LIGHTNING_H__BDDNDVJ300

#include <Arduino.h>

class LightningInvoice : public Printable{
private:
	uint8_t * buffer = NULL;
	size_t bufLen = 0;
public:
	LightningInvoice();
	LightningInvoice(const char * invoice);
	~LightningInvoice();
	uint32_t amount = 0;
	char multiplier = ' ';
	bool testnet = false;
	uint32_t timestamp() const;
	String description() const;
	size_t printTo(Print &p) const;
};

#endif // __LIGHTNING_H__BDDNDVJ300