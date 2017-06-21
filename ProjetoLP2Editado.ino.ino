#include <EEPROM.h>     // Biblioteca para facilita o uso da EEPROM
#include <SPI.h>        
#include <MFRC522.h>
#include <Ultrasonic.h>
#include <LiquidCrystal_I2C.h>
  
boolean match = false;          // Inicia cartão como falso
boolean programMode = false;    // Inicia o modo programador como falso
boolean replaceMaster = false;
boolean trocarGarrafao = false; // Variável para uso do sensor de nível de água
boolean condicaoPRI = false;

int successRead;
float ultra;

byte storedCard[4];   // Armazena uma ID lida da EEPROM
byte readCard[4];   // Armazena a identificação lida a partir do módulo RFID
byte masterCard[4];   // Armazena ID do cartão master lido da EEPROM

#define SS_PIN 10 //Pino do RFID
#define RST_PIN 9 //Pino do RFID
#define TRIG_PIN 5  //Pino do Sensor Ultrassônico
#define ECHO_PIN 6  //Pino do Sensor Ultrassônico

#define MNA_PIN 2 //Medidor de nível de água
#define AR_PIN 4 //Relé
#define wipeB 3 //Apagando memória de credenciais

MFRC522 mfrc522(SS_PIN, RST_PIN);
Ultrasonic ultrasonic(TRIG_PIN,ECHO_PIN);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

void setup() {
  Serial.begin(9600);  
  SPI.begin();  
  lcd.begin(16,2);  
  mfrc522.PCD_Init();    // Inicializar MFRC522 Hardware

  ShowReaderDetails();  // Mostrar detalhes de PCD - MFRC522 leitor de cartão

  //Apagar Código se Botão Pressionado enquanto a configuração é executada (ligado) limpa EEPROM
  if (digitalRead(wipeB) == HIGH) {  // Quando botão pressionado pino deve ficar baixo, botão conectado à terra
    
    Serial.println(F("Botão de limpeza precionado"));
    Serial.println(F("Você tem 4 segundos para cancelar"));
    Serial.println(F("Será removido todos os registro, e não será poderá ser desfeito"));
    
    delay(4000);                        // Tempo suficiente para cancelar a operação
    if (digitalRead(wipeB) == HIGH) {    // Se o botão estiver precionado limpa a EEPROM
  
      Serial.println(F("Iniciando limpeza da EEPROM"));
    PrintLcd ("Limpando EEPROM", 0);

      for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop pelos endereços da EEPROM
        if (EEPROM.read(x) == 0) {             
           //Se endereço tiver 0 pule para o próximo
        }
        else {
          EEPROM.write(x, 0);      // Senão escreve 0 no endereço para "limpar" 
        }
      }
    
      Serial.println(F("EEPROM limpa com sucesso"));
    PrintLcd ("EEPROM limpa", 0);

    }
    else {
    
      Serial.println(F("Reset Cancelado"));
    PrintLcd ("Clear cancelado", 0);
    
    }
  }
  if (EEPROM.read(1) != 143) {
    
    Serial.println(F("Cartão Master Não Definido"));
  PrintLcd ("Sem cartao master", 0);
    Serial.println(F("Digitalizar um PICC para definir como Master Card"));
  PrintLcd ("Aproxime cartao Master", 1);
  
    do {
      successRead = getID();            // Define successRead para 1 quando obtemos leitura do leitor caso contrário 0

    }
    while (!successRead);                  // O programa fica aguardando uma leitura de cartão para prosseguir
    for ( int j = 0; j < 4; j++ ) {        
      EEPROM.write( 2 + j, readCard[j] );  // Escreve o ID do cartão na EEPROM
    }
    EEPROM.write(1, 143);  
  
    Serial.println(F("Definido Master Card"));
  PrintLcd("Cartao master definido", 0);

  }
  Serial.println(F("-------------------"));
  Serial.println(F("ID do Master Card"));
  for ( int i = 0; i < 4; i++ ) {          // Ler cartão master da EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Salva na variável mastercard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo pronto"));
  PrintLcd("Aproxime cartao", 0);
 }

void loop () {
  if(!checkLevelAgua()){
  Serial.println ("-> Agua acabando <-");
  PrintLcd("Agua acabando", 1);
  }
  do {
    successRead = getID();  // Define successRead para 1 quando obter leitura do leitor caso contrário 0
    if (digitalRead(wipeB) == HIGH) {
      Serial.println(F("Botão de limpeza pressionado"));
      Serial.println(F("Master Card será apagado! Em 3 segundos"));
    PrintLcd("Clear irá comecar em 3 segundos", 0);
      delay(3000);
      if (digitalRead(wipeB) == HIGH) {
        EEPROM.write(1, 0);                  
        Serial.println(F("Reiniciar o dispositivo para reprogramar Mastercard"));
    PrintLcd("Reinicie placa", 1);
        while (1);
      }
    }
  }
  
  while (!successRead);   //O programa fica aguardando uma leitura de cartão para prosseguir

  if (programMode) {
    if ( isMaster(readCard) ) { //Se ler o cartão master, saia do modo programador
      Serial.println(F("-----------------------------"));
    Serial.println(F("Cartão Mestre Digitalizado"));
    Serial.println(F("MODO PROGRAMACAO"));
      Serial.println(F("-----------------------------"));
    PrintLcd("Modo programacao", 0);    
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // Se o cartão digitalizado for conhecido, exclua-o
        
    Serial.println(F("ID achado, removendo..."));
    PrintLcd("Removendo cartao", 0);
        deleteID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Aproxime o cartão para ADD ou REMOVER da EEPROM"));
    Serial.println(F("-----------------------------"));
    PrintLcd("Aproxime cartao para remover ou add", 1);
    
      }
      else {                    //Se o cartão digitalizado não for conhecido adicione-o
        Serial.println(F("Novo cartão lido, adicionando na EEPROM ..."));
    PrintLcd("Adicionando cartao", 0);
    writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Aproxime o cartão para ADD ou REMOVER da EEPROM"));
    Serial.println(F("-----------------------------"));
    PrintLcd("Aproxime cartao para remover ou add", 1);
      }
    }
  }
  else {
    if (isMaster(readCard)) {    // Se ler cartão master, entre no modo de programação
      programMode = true;
      Serial.println(F("Modo de programação ativo"));
    PrintLcd("Modo programacao", 0);
    
      int count = EEPROM.read(0);   // Leia o primeiro Byte da EEPROM que armazena o número de ID's na EEPROM
      Serial.print(F("Existem ")); 
      Serial.print(count);
      Serial.print(F(" ID(s) na EEPROM"));
    Serial.println(F("-----------------------------"));
      Serial.println(F("Aproxime um cartão para ADD ou REMOVER da EEPROM"));
      Serial.println(F("Aproxime o cartão Master para sair do modo programação"));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(readCard) ) { // Veja se o cartão está na EEPROM
        Serial.println(F("Acesso permitido!")); 
    PrintLcd("Acesso permitido!",0);
    
        ultra = ultrasonic.convert(ultrasonic.timing(), Ultrasonic::CM);;
        if (ultra < 7.0){ //Liberar a água
          liberaAgua (true);
          Serial.println(F("Torneira Aberta"));
      PrintLcd("Torneira Aberta",1);
          while(1){
            ultra = ultrasonic.convert(ultrasonic.timing(), Ultrasonic::CM);;
            if(ultra >= 7.0){   //Ao afastar o copo a água é fechada
              break;
            }
          }
          liberaAgua (false);
          Serial.println(F("Torneira fechada"));
      PrintLcd("Torneira Fechada", 1);
          
          if(!checkLevelAgua()){
            Serial.println ("-> Agua acabando <-");
      PrintLcd("Agua acabando", 1);
      }
      } else {
    Serial.println("Copo longe do bebedouro!");
    PrintLcd("Copo longe!", 1);
        liberaAgua (false);

     }
      }
      else {      // Usuário sem permissão
        Serial.println(F("Acesso negado!"));
    PrintLcd("Acesso negado!", 0);
        liberaAgua (false);
      }
    }
  }
}

int getID() {
  // Preparando-se para a leitura de PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Se um novo PICC colocado no leitor RFID continuar
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Uma vez que um PICC colocado obter Serial e continuar
    return 0;
  }
  // Só é compatível a leitura de cartões de 4bytes!
  Serial.println(F("UID do cartão:"));
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // para leitura
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    while (true); // do not go further
  }
}

void readID( int number ) {
  int start = (number * 4 ) + 2;    // Descobrir a posição inicial
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 vezes para obter os 4 bytes
    storedCard[i] = EEPROM.read(start + i);   // Atribuir valores lidos da EEPROM para o array
  }
}

void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Antes de escrever para a EEPROM, verificar se cartão já é cadastrado
    int num = EEPROM.read(0);     // Obter o número de espaços utilizados, a posição 0 armazena o número de cartões de identificação
    int start = ( num * 4 ) + 6;  // Descobrir onde começa o próximo slot
    num++;          
    EEPROM.write( 0, num );     // Escreva a nova contagem para o contador
    for ( int j = 0; j < 4; j++ ) {   
      EEPROM.write( start + j, a[j] );  // Escreva os valores do array para EEPROM na posição correta
    }
  
    Serial.println(F("ID adicionado com sucesso à EEPROM"));
  }
  else {
  
    Serial.println(F("ERRO! Algum problema com o ID do cartão"));
  }
}

void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Antes de excluir da EEPROM, verifique se tem este cartão!
   
    Serial.println(F("ERRO! Há algo de errado com ID ou EEPROM ruim"));
  }
  else {
    int num = EEPROM.read(0);   // Obter o número de espaços utilizados, a posição 0 armazena o número de cartões de identificação
    int slot;       // Descobrir o número do slot do cartão
    int start;      // = ( num * 4 ) + 6; // Descobrir onde começa o próximo slot
    int looping;    // O número de vezes que o loop repete
    int j;
    int count = EEPROM.read(0); // Leia o primeiro Byte da EEPROM que armazena o número de cartões
    slot = findIDSLOT( a );   // Descobrir o número do slot do cartão para apagar
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;    
    EEPROM.write( 0, num );   // Define um novo valor para o contador
    for ( j = 0; j < looping; j++ ) {       
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   //Desloque os valores da matriz para 4 posições anteriores na EEPROM
    }
    for ( int k = 0; k < 4; k++ ) {         // Deslocando Loop
      EEPROM.write( start + j + k, 0);
    }
   
    Serial.println(F("ID removida com sucesso da EEPROM"));
  }
}

boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL )       // Certifique-se de que há algo na matriz primeiro
    match = true;       // Suponha que eles correspondam no início
  for ( int k = 0; k < 4; k++ ) {   
    if ( a[k] != b[k] )     // Se a! = B então defina match = false, um falha, todos falham
      match = false;
  }
  if ( match ) {     
    return true;     
  }
  else  {
    return false;  
  }
}

int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0);       // Leia o primeiro Byte da EEPROM
  for ( int i = 1; i <= count; i++ ) {    // Repetir uma vez para cada entrada EEPROM
    readID(i);                // Ler uma ID da EEPROM, ela é armazenada no storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Verifique se o cartão armazenado leu da EEPROM
      // É o mesmo que o cartão de identificação find [] passou
      return i;         // O número do slot do cartão
      break;      
    }
  }
}

boolean findID( byte find[] ) {
  int count = EEPROM.read(0);     // Leia o primeiro Byte da EEPROM
  for ( int i = 1; i <= count; i++ ) {    // Repetir uma vez para cada entrada EEPROM
    readID(i);          // Ler uma ID da EEPROM, ela é armazenada em storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Verifique se o cartão armazenado leu da EEPROM
      return true;
      break; 
    }
    else {    
    }
  }
  return false;
}

boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

boolean checkLevelAgua(){
  
  if (digitalRead(MNA_PIN)) {       // Nível da água está baixo;
    trocarGarrafao = true;      //Trocar garrafão;  
    return true;
  }
  else  {                 // Nível da água está alto;
    trocarGarrafao = false;       
    return false;
  }
}

void liberaAgua(boolean condicao){
  if (condicao /*true*/){
    digitalWrite(AR_PIN, HIGH);
  } else {
    digitalWrite(AR_PIN, LOW);
  }
}

void PrintLcd(String texto, int linha){
  lcd.clear ();
  int centro = 8 - (sizeof(texto) / 2);
  lcd.setCursor (centro, linha);
  
  if (sizeof(texto) > 16){
    int i = sizeof(texto) - 16;
    int j;
    for (j = 0; j >= (-1 * i); j--){
      lcd.setCursor(j, linha);
      lcd.print(texto);
      delay(100);
    }
  } else {
    lcd.setCursor(centro, linha);
    lcd.print(texto);
  }
}
