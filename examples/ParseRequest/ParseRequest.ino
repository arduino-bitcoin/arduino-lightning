#include <Lightning.h>
#include <Conversion.h>
#include <TimeLib.h>

void setup() {
  Serial.begin(9600);  
  while(!Serial){
    ; // wait for serial port to open
  }
  Serial.println("Parsing invoice...");
  char url[] = "lightning:lntb34u1pdjuh8upp5tmfc8y2gx36nnq3fwegn6tuhsjr0d7jljvv45a7ce7r37jldne6sdpzxgsy2umswfjhxum0yppk76twypgxzmnwvycqp26t859kdv2cm5umyuq7j45k5ha6yzc30kgmxvquk4stzp6482xrrrcepuets3ue5lqra73metx35uczy3vfkq56mfcrzntcrwvpf2n8gpl8geal";
  LightningInvoice invoice(url);

  Serial.print("Network: ");
  if(invoice.testnet){
    Serial.println("Testnet");
  }else{
    Serial.println("Mainnet");
  }
  
  TimeElements tm;
  breakTime(invoice.timestamp(), tm);
  Serial.print(String("Invoice date: ") + monthStr(tm.Month) + " " + tm.Day + ", " + (1970+tm.Year));
  Serial.println(String(", ") + tm.Hour + ":" + tm.Minute + ":" + tm.Second);

  Serial.print("Amount: ");
  if(invoice.amount == 0){
    Serial.println("any");
  }else{
    Serial.print(invoice.amount);
    Serial.print(" ");
    if(invoice.multiplier== 'u'){
      Serial.println("bits (µBTC)");
    }else{
      Serial.print(invoice.multiplier); // _, m, u, n, p
      Serial.println("BTC");
    }
  }

  Serial.print("Description: ");
  Serial.println(invoice.description());
  
  Serial.println(invoice);
}

void loop() {
  
}

