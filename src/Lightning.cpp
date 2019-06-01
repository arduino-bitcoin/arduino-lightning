#include "Lightning.h"
#include "Conversion.h"
#include "Bitcoin.h"
#include "Hash.h"

#include <string.h>
using std::string;

#define MAX_INVOICE_SIZE 400

LightningInvoice::LightningInvoice(){};

LightningInvoice::LightningInvoice(
                    const char description[], const uint8_t preimage[32], uint32_t time,
                    uint32_t value, char mult, bool use_testnet){
    testnet = use_testnet;
    amount = value;
    multiplier = mult;

    // timestamp
    buffer = (uint8_t *) calloc( 7, sizeof(uint8_t));
    bufLen = 7;

    for(int i=0; i<6; i++){
        buffer[6-i] = (time % 32);
        time = (time / 32);
    }

    // hash
    uint8_t hash[32];
    sha256(preimage, 32, hash);
    addField(1, hash, sizeof(hash));

    // description. 
    // TODO: description hash if len > 639
    size_t l = strlen(description);
    addField(13, (uint8_t *)description, l);
}


int LightningInvoice::addRoutingInfo(uint8_t pubkey[33], uint8_t short_channel_id[8], uint32_t fee_base_msat, uint32_t fee_proportional_millionths, uint16_t cltv_expiry_delta){

    uint32_t offset = 0;
    uint8_t r[51];         // 33+8+4+4+2

    memcpy(r, pubkey, 33);
    memcpy(r + 33, short_channel_id, 8);

    uint8_t fee_base_msat_big_endian[4];
    fee_base_msat_big_endian[0] = ((uint8_t *)&fee_base_msat)[3];
    fee_base_msat_big_endian[1] = ((uint8_t *)&fee_base_msat)[2];
    fee_base_msat_big_endian[2] = ((uint8_t *)&fee_base_msat)[1];
    fee_base_msat_big_endian[3] = ((uint8_t *)&fee_base_msat)[0];
    memcpy(r + 33 + 8, fee_base_msat_big_endian, 4);
    

    uint8_t fee_proportional_millionths_big_endian[4];
    fee_proportional_millionths_big_endian[0] = ((uint8_t *)&fee_proportional_millionths)[3];
    fee_proportional_millionths_big_endian[1] = ((uint8_t *)&fee_proportional_millionths)[2];
    fee_proportional_millionths_big_endian[2] = ((uint8_t *)&fee_proportional_millionths)[1];
    fee_proportional_millionths_big_endian[3] = ((uint8_t *)&fee_proportional_millionths)[0];
    memcpy(r + 33 + 8 + 4, fee_proportional_millionths_big_endian, 4);


    uint8_t cltv_expiry_delta_big_endian[2];
    cltv_expiry_delta_big_endian[0] = ((uint8_t *)&cltv_expiry_delta)[1];
    cltv_expiry_delta_big_endian[1] = ((uint8_t *)&cltv_expiry_delta)[0];
    memcpy(r + 33 + 8 + 4 + 4, fee_proportional_millionths_big_endian, 2);


    addField(3, r, 51);

}

LightningInvoice::LightningInvoice(const char * invoice){
    size_t l = strlen(invoice);
    if(l > MAX_INVOICE_SIZE){
        return;
    }
    size_t offset = 0;
    // get rid of the `lightning:` prefix if it's there
    if(memcmp(invoice, "lightning:", 10) == 0){
        offset = 10;
    }

    size_t len = l - offset;
    size_t dataLen;
    char * hrp = (char *) calloc(len-6, sizeof(char));
    uint8_t * data = (uint8_t *) calloc(len-8, sizeof(uint8_t));
    if(!bech32_decode(hrp, data, &dataLen, invoice+offset)){
        free(data);
        free(hrp);
        return; // fail in decoding
    }

    // is it lightning invoice?
    if(memcmp(hrp, "ln", 2) != 0){
        free(data);
        free(hrp);
        return;
    }
    // testnet or mainnet? regtest is not implemented yet
    if(memcmp(hrp+2, "bc", 2) == 0){
        testnet = false;
    }else if(memcmp(hrp+2, "tb", 2) == 0){
        testnet = true;
    }else{
        free(data);
        free(hrp);
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
    // TODO: move signature from data
    buffer = (uint8_t *) calloc( dataLen, sizeof(uint8_t));
    memcpy(buffer, data, dataLen);
    bufLen = dataLen;

    free(hrp);
    free(data);
}
// LightningInvoice::LightningInvoice(const LightningInvoice &other){
//     if(other.bufLen > 0){
//         bufLen = other.bufLen;
//         buffer = (uint8_t *) calloc( bufLen, sizeof(uint8_t));
//         memcpy(buffer, other.buffer, bufLen);
//     }
//     amount = other.amount;
//     multiplier = other.multiplier;
//     testnet = other.testnet;
//     sig = other.sig;
// }
LightningInvoice::~LightningInvoice(){
    if(bufLen>0){
        free(buffer);
        bufLen = 0;
    }
}
int LightningInvoice::addField(uint8_t code, const uint8_t * data, uint16_t dataLen){
    uint8_t * arr;
    arr = (uint8_t *)calloc(dataLen*2, sizeof(uint8_t));
    if(arr == NULL){
        return 0;
    }
    size_t arrLen = 0;
    convert_bits(arr, &arrLen, 5, data, dataLen, 8, 1);

    buffer = (uint8_t *) realloc( buffer, (bufLen + arrLen + 3) * sizeof(uint8_t));
    if(buffer == NULL){
        return 0;
    }
    buffer[bufLen] = code;
    buffer[bufLen+1] = (arrLen >> 5);
    buffer[bufLen+2] = (arrLen % 32);
    memcpy(buffer+bufLen+3, arr, arrLen);
    bufLen += arrLen + 3;
    free(arr);
    return arrLen+3;
}
int LightningInvoice::addRawData(const uint8_t * data, size_t dataLen){
    buffer = (uint8_t *) realloc( buffer, (bufLen + dataLen) * sizeof(uint8_t));
    memcpy(buffer+bufLen, data, dataLen);
    bufLen += dataLen;
    return dataLen;
}
int LightningInvoice::setExpiry(uint32_t expiry){
    uint8_t arr[7];
    uint8_t len = 0;
    while(expiry > 0){
        len++;
        arr[7-len] = (expiry % 32);
        expiry = expiry/32;
    }
    buffer = (uint8_t *) realloc( buffer, (bufLen + len + 3) * sizeof(uint8_t));
    buffer[bufLen] = 6;
    buffer[bufLen+1] = (len >> 5);
    buffer[bufLen+2] = (len % 32);
    memcpy(buffer+bufLen+3, arr+7-len, len);
    bufLen += len + 3;
    return len + 3;
}
int LightningInvoice::hmr(char * arr, size_t arrSize) const{
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
    size_t len = strlen(hrp);
    if(arrSize < len){
        return 0;
    }
    memset(arr, 0, arrSize);
    memcpy(arr, hrp, len);
    return len;
}
int LightningInvoice::hash(uint8_t hashArr[32]){
    char hrp[20] = { 0 };
    size_t len = hmr(hrp, sizeof(hrp)-1);
    SHA256 h;
    h.begin();
    h.write((uint8_t *)hrp, len);
    uint8_t data[300] = { 0 };
    size_t dataLen = 0;
    convert_bits(data, &dataLen, 8, buffer, bufLen, 5, 1);
    h.write(data, dataLen);
    h.end(hashArr);
    return 32;
}
Signature LightningInvoice::sign(const PrivateKey pk){
    uint8_t h[32];
    hash(h);
    sig = pk.sign(h);
    return sig;
}
uint32_t LightningInvoice::timestamp() const{
    uint32_t ts;
    for(int i=0; i<7; i++){
        ts <<= 5;
        ts += buffer[i];
    }
    return ts;
}
string LightningInvoice::description() const{
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
            string s;
            for(int c=0; c<descriptionLength; c++){
                if(description[c]!=0){
                    s = string((char *)(description+c));
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

// size_t LightningInvoice::printTo(Print &p) const{
//     // p.print("lightning:");
//     char hrp[20] = { 0 };
//     hmr(hrp, sizeof(hrp));
//     uint8_t * data = (uint8_t *) calloc(bufLen + 120, sizeof(uint8_t));
//     memcpy(data, buffer, bufLen);
//     size_t len = bufLen;
//     uint8_t sig_bin[65];
//     sig.bin(sig_bin);
//     sig_bin[64] = sig.index;
//     size_t sig_len = 0;
//     convert_bits(data+len, &sig_len, 5, sig_bin, sizeof(sig_bin), 8, 1);
//     len += sig_len;
//     char * output = (char *) calloc(strlen(hrp)+len+20, sizeof(char));
//     bech32_encode(output, hrp, data, len);
//     p.print(output);
//     size_t l = strlen(output);
//     free(data);
//     free(output);
//     return l; // just strlen would be better
// }

size_t LightningInvoice::toCharArray(char * arr, size_t arrSize) const{
    char hrp[20] = { 0 };
    hmr(hrp, sizeof(hrp));
    uint8_t * data = (uint8_t *) calloc(bufLen + 120, sizeof(uint8_t));
    memcpy(data, buffer, bufLen);
    size_t len = bufLen;
    uint8_t sig_bin[65];
    sig.bin(sig_bin, 65);
    sig_bin[64] = sig.index;
    size_t sig_len = 0;
    convert_bits(data+len, &sig_len, 5, sig_bin, sizeof(sig_bin), 8, 1);
    len += sig_len;
    size_t l = strlen(hrp)+len+20;
    if(l > arrSize){
        free(data);
        return 0;
    }
    memset(arr, 0, arrSize);
    bech32_encode(arr, hrp, data, len);
    free(data);
    return l;
}

// LightningInvoice &LightningInvoice::operator=(LightningInvoice const &other){ 
//     if(bufLen > 0){
//         free(buffer);
//     }
//     if(other.bufLen > 0){
//         bufLen = other.bufLen;
//         buffer = (uint8_t *) calloc( bufLen, sizeof(uint8_t));
//         memcpy(buffer, other.buffer, bufLen);
//     }
//     amount = other.amount;
//     multiplier = other.multiplier;
//     testnet = other.testnet;
//     sig = other.sig;
//     return *this; 
// };
