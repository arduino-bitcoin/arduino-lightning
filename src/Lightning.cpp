#include "Lightning.h"
#include <Conversion.h>

#define MAX_REQUEST_SIZE 1000

LightningInvoice::LightningInvoice(){};
LightningInvoice::LightningInvoice(const char * invoice){
    size_t l = strlen(invoice);
    if(l > MAX_REQUEST_SIZE){
        return;
    }
    size_t offset = 0;
    // get rid of the `lightning:` prefix if it's there
    if(memcmp(invoice, "lightning:", 10) == 0){
        offset = 10;
    }

    size_t len = l - offset;
    size_t data_len;
    char * hrp = (char *) calloc(len-6, sizeof(char));
    uint8_t * data = (uint8_t *) calloc(len-8, sizeof(uint8_t));
    if(!bech32_decode(hrp, data, &data_len, invoice+offset)){
        return; // fail in decoding
    }

    // is it lightning invoice?
    if(memcmp(hrp, "ln", 2) != 0){
        return;
    }
    // testnet or mainnet? regtest is not implemented yet
    if(memcmp(hrp+2, "bc", 2) == 0){
        testnet = false;
    }else if(memcmp(hrp+2, "tb", 2) == 0){
        testnet = true;
    }else{
        return;
    }

    //parsing amount
    if(strlen(hrp) > 4){
        amount = atol(hrp+4);
        char m = hrp[strlen(hrp)-1];
        if(m < '0' || m > '9'){
            multiplier = m;
        }
    }
    
    buffer = (uint8_t *) calloc( data_len, sizeof(uint8_t));
    memcpy(buffer, data, data_len);
    bufLen = data_len;

    free(hrp);
    free(data);
}
LightningInvoice::~LightningInvoice(){
    free(buffer);
}
uint32_t LightningInvoice::timestamp() const{
    uint32_t ts;
    for(int i=0; i<7; i++){
        ts <<= 5;
        ts += buffer[i];
    }
    return ts;
}
String LightningInvoice::description() const{
    size_t cursor = 7;
    while(cursor < bufLen){
        uint8_t type = buffer[cursor];
        uint16_t len = buffer[cursor+1];
        len<<=5;
        len += buffer[cursor+2];
        cursor += 3;
        if(type == 13){ // description
            uint8_t * description = (uint8_t *) calloc(2*len+1, sizeof(uint8_t));
            size_t descriptionLength = len;
            if(!convert_bits(description, &descriptionLength, 8, buffer+cursor, len, 5, 0)){
                free(description);
                return "";
            }
            String s;
            for(int c=0; c<descriptionLength; c++){
                if(description[c]!=0){
                    s = String((char *)(description+c));
                    break;
                }
            }
            free(description);
            return s;
        }else if( type == 23 ){ // hash
            if(len != 52){
                return "";
            }
            // TODO: implement hash
            return "hash";
        }else{
            cursor += len;
        }
    }
    return "";
}
size_t LightningInvoice::printTo(Print &p) const{
    p.print("lightning:");
    char hrp[20] = "ln";
    if(testnet){
        memcpy(hrp+2,"tb",2);
    }else{
        memcpy(hrp+2,"bc",2);
    }
    if(amount > 0){
        sprintf(hrp+4,"%ld", amount);
        if(multiplier != ' '){
            hrp[strlen(hrp)] = multiplier;
        }
    }
    char * output = (char *) calloc(strlen(hrp)+bufLen+8+1, sizeof(char));
    bech32_encode(output, hrp, buffer, bufLen);

    p.print(output);
    return strlen(hrp)+bufLen+8+11;
}