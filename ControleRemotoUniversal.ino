#include <IRremote.h>

// DEFINIÇÃO DE PINOS
byte RECV_PIN = 11;
byte buttonPin = 7;
byte pinLed = 13;

// VARIÁVEIS GLOBAIS
bool modoGravacao = false;           // Modo de Gravação
bool estadoBotaoAnterior = HIGH;     // Estado anterior do botão
unsigned long lastDebounceTime = 0;  // Tempo da última mudança de estado
unsigned long debounceDelay = 1;    // Delay para debouncing (ajustado para 1ms)
String tipoControle = "";

// CÓDIGO INICIAL PANASONIC
unsigned long panasonicAddress = 0x4004;

/* CODIGOS HEXADECIMAIS DO CONTROLE */
unsigned long codigosPanasonic[] = { 0x100BCBD, 0x1009899, 0x100080, 0x1008889, 0x1004849, 0x100C8C9, 0x1002829, 0x100A8A9, 0x1006869, 0x100E8E9, 0x1001819, 0x1000405, 0x1008485, 0x1002C2D, 0x100ACAD, 0x1004C4D };
unsigned long codigosSony[] = { 0x750, 0xF50, 0x110, 0x010, 0x810, 0x410, 0xC10, 0x210, 0xA10, 0x610, 0xE10, 0x110, 0x490, 0xC90, 0x090, 0x890, 0x290 };
unsigned long codigoLGNEC[] = { 0x20DF10EF, 0x20DF08F7, 0x20DF8877, 0x20DF48B7, 0x20DFC837, 0x20DF28D7, 0x20DFA857, 0x20DF6897, 0x20DFE817, 0x20DF18E7, 0x20DF9867, 0x20DF40BF, 0x20DFC03F, 0x20DF00FF, 0x20DF807F };
unsigned long codigoGenerico[] = { 0x00FEA857, 0x00FE00FF, 0x00FE807F, 0x00FE40BF, 0x00FEC03F, 0x00FE20DF, 0x00FEA05F, 0x00FE609F, 0x00FEE01F, 0x00FE10EF, 0x00FE906F, 0x00FED827, 0x00FE58A7, 0x00FE9867, 0x00FE18E7, 0x00FE18E7 };

IRrecv irrecv(RECV_PIN);  // Inicializa o Objeto IRReceiver
IRsend irsend;
decode_results results;  // Decodifica o Resultado

void setup() {
  Serial.begin(9600);                // Inicializa
  pinMode(buttonPin, INPUT_PULLUP);  // Pino do Botão como Entrada Pullup
  pinMode(pinLed, OUTPUT);           // Pino do Led como Saída
  irrecv.enableIRIn();               // Inicia o receptor IR
}

void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
    tipoControle = "NEC";
  } else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
    tipoControle = "SONY";
  } else if (results->decode_type == PANASONIC) {
    Serial.print("Decoded PANASONIC - Address: ");
    tipoControle = "PANASONIC";
    Serial.print(results->address, HEX);
    Serial.print(" Value: ");
  } else if (results->decode_type == LG) {
    Serial.print("Decoded LG: ");
    tipoControle = "LG";
  } else {
    Serial.print("Decoded Unknown: ");
    tipoControle = "DESCONHECIDO";
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 1; i < count; i++) {
    if (i & 1) {
      Serial.print(results->rawbuf[i] * USECPERTICK, DEC);
    } else {
      Serial.write('-');
      Serial.print((unsigned long)results->rawbuf[i] * USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println();
}

bool isCodeInArray(unsigned long code, unsigned long array[], int size) {
  for (int i = 0; i < size; i++) {
    if (array[i] == code) {
      return true;
    }
  }
  return false;
}

void loop() {
  int leituraBotao = digitalRead(buttonPin);

  // Verifica se o botão mudou de estado
  if (leituraBotao != estadoBotaoAnterior) {
    lastDebounceTime = millis();  // Reinicia o tempo de debounce
    Serial.print("Botão mudou para: ");
    Serial.println(leituraBotao == LOW ? "PRESSIONADO" : "LIBERADO");
  }

  // Verifica se o debounce delay passou
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Ativa/desativa o modo de gravação quando o botão é pressionado
    if (leituraBotao == LOW && estadoBotaoAnterior == HIGH) {
      modoGravacao = !modoGravacao;  // Alterna o estado do modo de gravação
      if (modoGravacao) {
        Serial.println("Modo de Gravação Ativado");
        digitalWrite(pinLed, HIGH);  // Acende o LED
      } else {
        Serial.println("Modo de Gravação Desativado");
        digitalWrite(pinLed, LOW);  // Apaga o LED
      }
      // Aguarda o botão ser liberado
      while (digitalRead(buttonPin) == LOW) {
        // Não faz nada, apenas aguarda o botão ser liberado
      }
    }
  }

  estadoBotaoAnterior = leituraBotao;

  // Captura sinais IR independentemente do modo de gravação
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    dump(&results);
    
    unsigned long codigoCapturado = results.value;
    bool encontrado = false;

    if (tipoControle == "NEC") {
      if (isCodeInArray(codigoCapturado, codigoLGNEC, sizeof(codigoLGNEC) / sizeof(codigoLGNEC[0]))) {
        Serial.println("Código NEC encontrado.");
        encontrado = true;
      } else if (isCodeInArray(codigoCapturado, codigoGenerico, sizeof(codigoGenerico) / sizeof(codigoGenerico[0]))) {
        Serial.println("Código GENÉRICO encontrado.");
        encontrado = true;
      }
    } else if (tipoControle == "SONY") {
      if (isCodeInArray(codigoCapturado, codigosSony, sizeof(codigosSony) / sizeof(codigosSony[0]))) {
        Serial.println("Código SONY encontrado.");
        encontrado = true;
      }
    } else if (tipoControle == "PANASONIC") {
      if (isCodeInArray(codigoCapturado, codigosPanasonic, sizeof(codigosPanasonic) / sizeof(codigosPanasonic[0]))) {
        Serial.println("Código PANASONIC encontrado.");
        encontrado = true;
      }
    } else {
      Serial.println("Tipo de controle desconhecido ou não suportado.");
    }

    if (!encontrado) {
      Serial.println("Código não encontrado em nenhuma lista.");
    }

    irrecv.resume();  // Recebe o próximo valor
  }

  // Verifica se há entrada no Serial Monitor
  if (Serial.available() > 0) {
    char comando = Serial.read();
    Serial.print("Comando recebido: ");
    Serial.println(comando);

    if (comando == 'P') {
      if (!modoGravacao) {
        Serial.println("Modo de gravação desativado. Enviando sinal IR.");

        // Envia o sinal apropriado com base no tipo de controle
        if (tipoControle == "NEC") {
          if (isCodeInArray(codigoLGNEC[0], codigoLGNEC, sizeof(codigoLGNEC) / sizeof(codigoLGNEC[0]))) {
            sendNECTV(codigoLGNEC[0]);
          } else {
            Serial.println("Código NEC para ligar a TV não encontrado.");
          }
        } else if (tipoControle == "SONY") {
          if (isCodeInArray(codigosSony[0], codigosSony, sizeof(codigosSony) / sizeof(codigosSony[0]))) {
            sendSonyTV(codigosSony[0]);
          } else {
            Serial.println("Código SONY para ligar a TV não encontrado.");
          }
        } else if (tipoControle == "PANASONIC") {
          if (isCodeInArray(codigosPanasonic[0], codigosPanasonic, sizeof(codigosPanasonic) / sizeof(codigosPanasonic[0]))) {
            sendPanasonicTV(panasonicAddress, codigosPanasonic[0]);
          } else {
            Serial.println("Código PANASONIC para ligar a TV não encontrado.");
          }
        } else {
          Serial.println("Tipo de controle desconhecido para enviar sinal.");
        }
      } else {
        Serial.println("Modo de gravação ativado. Não é possível enviar sinais IR.");
      }
    } else {
      Serial.println("Comando não reconhecido.");
    }
  }
}

void sendNECTV(unsigned long codigoNEC) {
  Serial.print("Enviando código NEC: ");
  Serial.println(codigoNEC, HEX);
  irsend.sendNEC(codigoNEC, 32);
}

void sendSonyTV(unsigned long codigoSony) {
  Serial.print("Enviando código SONY: ");
  Serial.println(codigoSony, HEX);
  irsend.sendSony(codigoSony, 12); // Ajuste o número de bits se necessário
}

void sendPanasonicTV(unsigned long address, unsigned long codigoPanasonic) {
  Serial.print("Enviando código PANASONIC: ");
  Serial.println(codigoPanasonic, HEX);
  irsend.sendPanasonic(address, codigoPanasonic);
}
