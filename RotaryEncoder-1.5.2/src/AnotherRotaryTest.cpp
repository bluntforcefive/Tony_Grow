#define CLK 2
#define DATA 4
#define BUTTON 3
#define YLED 8

void setup() {
  pinMode(CLK, INPUT);
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DATA, INPUT);
  pinMode(DATA, INPUT_PULLUP);
  pinMode(BUTTON, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(YLED,OUTPUT);

  Serial.begin (9600);
  Serial.println("KY-040 Quality test:");
}

static uint8_t prevNextCode = 0;

void loop() {
uint32_t pwas=0;

   if( read_rotary() ) {

      Serial.print(prevNextCode&0xf,HEX);Serial.print(" ");

      if ( (prevNextCode&0x0f)==0x0b) Serial.println("eleven ");
      if ( (prevNextCode&0x0f)==0x07) Serial.println("seven ");
   }

   if (digitalRead(BUTTON)==0) {

      delay(10);
      if (digitalRead(BUTTON)==0) {
          Serial.println("Next Detent");
          while(digitalRead(BUTTON)==0);
      }
   }
}

// A vald CW or CCW move returns 1, invalid returns 0.
int8_t read_rotary() {
  static int8_t rot_enc_table[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0};

  prevNextCode <<= 2;
  if (digitalRead(DATA)) prevNextCode |= 0x02;
  if (digitalRead(CLK)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  return ( rot_enc_table[( prevNextCode & 0x0f )]);
}