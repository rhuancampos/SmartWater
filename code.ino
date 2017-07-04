#include <EEPROM.h>     // Biblioteca para facilita o uso da EEPROM
#include <SPI.h>        
#include <MFRC522.h>
#include <Ultrasonic.h>
#include <LiquidCrystal_I2C.h>
  
boolean match = false;          // Inicia cartão como falso
boolean programMode = false;    // Inicia o modo programador como falso
boolean replaceMaster = false;

boolean condicaoPRI = false;

int successRead;
float ultra;
int valor;
const int potenciometro = 0

byte storedCard[4];   // Armazena uma ID lida da EEPROM
byte readCard[4];   // Armazena a identificação lida a partir do módulo RFID
byte masterCard[4];   // Armazena ID do cartão master lido da EEPROM

#define SS_PIN 10 //Pino do RFID
#define RST_PIN 9 //Pino do RFID
#define TRIG_PIN 5  //Pino do Sensor Ultrassônico
#define ECHO_PIN 6  //Pino do Sensor Ultrassônico

//#define MNA_PIN 2 //Medidor de nível de água
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

	  if (digitalRead(wipeB) == HIGH) {  // Quando botão pressionado pino deve ficar alto, botão conectado à 5V
		
		PrintLcd("Botão de limpeza precionado", 0, true);

		if (digitalRead(wipeB) == HIGH) {    // Se o botão estiver precionado limpa a EEPROM
	  
			PrintLcd ("Limpando EEPROM", 1, false);

			for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop pelos endereços da EEPROM
				if (EEPROM.read(x) == 0) {	//Se endereço tiver 0 pule para o próximo
					}	else	{
						EEPROM.write(x, 0);	// Senão escreve 0 no endereço para "limpar" 
						}
			  }
			  
			PrintLcd ("EEPROM limpa", 1, false);
			
		} else {
			PrintLcd ("Limpeza cancelada", 0, true);
			}
	  }
	  
	  if (EEPROM.read(1) != 143) {
		  
			PrintLcd ("Sem cartao master", 0, true);
			PrintLcd ("Aproxime cartao Master", 1, false);
	  
			do {
			  successRead = getID();            // Define successRead para 1 quando obtemos leitura do leitor caso contrário 
			  }
			  
			while (!successRead);                  // O programa fica aguardando uma leitura de cartão para prosseguir
				for ( int j = 0; j < 4; j++ ) {        
				  EEPROM.write( 2 + j, readCard[j] );  // Escreve o ID do cartão na EEPROM
				  }
				  
		EEPROM.write(1, 143);  
		PrintLcd("MASTER definido", 0, true);
		
		}
		
	for ( int i = 0; i < 4; i++ ) {          // Ler cartão master da EEPROM
		masterCard[i] = EEPROM.read(2 + i);    // Salva na variável mastercard
		Serial.print(masterCard[i], HEX);
	}
	  
	  PrintLcd("Aproxime cartao", 0, true);
	  
	}

	
void loop () {
	
	do {
		successRead = getID();  // Define successRead para 1 quando obter leitura do leitor caso contrário 0
		
		if (digitalRead(wipeB) == HIGH) {
			
			PrintLcd("Botão de limpeza precionado", 0, true);

			if (digitalRead(wipeB) == HIGH) {
				EEPROM.write(1, 0);
				PrintLcd ("EEPROM limpa", 0, true);			
				PrintLcd("Reinicie placa", 1, false);
				while (1);
				}
			}
		}
		
	while (!successRead);   //O programa fica aguardando uma leitura de cartão para prosseguir
	
	if (programMode) {
		if (isMaster(readCard)) { //Se ler o cartão master, saia do modo programador
			PrintLcd("Modo programacao", 0, true);
			PrintLcd("Saindo", 1, false);
			delay(500);
			programMode = false;
			return;
			
		} else {
			if ( findID(readCard) ) { // Se o cartão digitalizado for conhecido, exclua-o
				
				PrintLcd("Removendo cartao", 0, true);
				deleteID(readCard);
				PrintLcd("Aproxime cartao para remover ou add", 1, false);
				
				} else {                    //Se o cartão digitalizado não for conhecido adicione-o
					PrintLcd("Adicionando cartao", 0, true);
					writeID(readCard);
					PrintLcd("Aproxime cartao para remover ou add", 1, false);
				}

			}
			
	} else {
		if (isMaster(readCard)) {    // Se ler cartão master, entre no modo de programação
			programMode = true;

			PrintLcd("Modo programacao", 0, true);
			PrintLcd("ATIVO", 1, false);
		
			int count = EEPROM.read(0);   // Leia o primeiro Byte da EEPROM que armazena o número de ID's na EEPROM
			Serial.print(F("n = ")); 
			Serial.print(count);

		} else {
			if ( findID(readCard) ) { // Veja se o cartão está na EEPROM
				PrintLcd("Acesso permitido!",0, true);
				ultra = ultrasonic.convert(ultrasonic.timing(), Ultrasonic::CM);
				
				if (ultra < 7.0){ //Liberar a água
					liberaAgua (true);
					PrintLcd("Torneira Aberta",1, false);
					while(1){
						ultra = ultrasonic.convert(ultrasonic.timing(), Ultrasonic::CM);
						
						if(ultra >= 7.0){   //Ao afastar o copo a água é fechada
							break;
						}
					}
					
					liberaAgua (false);

					PrintLcd("Torneira Fechada", 1, false);
					delay(2000);
					
					PrintLcd("Aproxime cartao", 0, true);
					
					if(!checkLevelAgua()){
						PrintLcd("Agua acabando", 1, false);
					}
					
				} else {
				PrintLcd("Copo longe!", 0, true);
				liberaAgua (false);
				delay(2000);
				PrintLcd("Aproxime cartao", 0, true);
				}
			} else {      // Usuário sem permissão
				PrintLcd("Acesso negado!", 0,true);
				liberaAgua (false);
				delay(2000);
				PrintLcd("Aproxime cartao", 0,true);
			}
		}
	}
}

int getID() {

	  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Se um novo PICC colocado no leitor RFID continuar
		return 0;
	  }
	  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Uma vez que um PICC colocado obter Serial e continuar
		return 0;
	  }
	  // Só é compatível a leitura de cartões de 4bytes!
	  for (int i = 0; i < 4; i++) {  //
		readCard[i] = mfrc522.uid.uidByte[i];
		Serial.print(readCard[i], HEX);
	  }
	  
	  mfrc522.PICC_HaltA(); // para leitura
	  return 1;
}

void readID(int number) {
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
	  
		PrintLcd("Gravacao OK", 0, true);
		delay(500);
	} else {
		PrintLcd("Erro gravacao", 0, true);
		delay(600);
	}
}

void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Antes de excluir da EEPROM, verifique se tem este cartão!
   
    PrintLcd("Erro com ID ou", 0, true);
	PrintLcd("ou com a EEPROM", 1, false);
	delay(500);
	
  } else {
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
   
    PrintLcd("ID removido", 0, true);
	delay(500);
  }
}

boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL ){
	  return false;
	  }  	  // Certifique-se de que há algo na matriz primeiro

	
  for ( int k = 0; k < 4; k++ ) {   
    if ( a[k] != b[k] ){     // Se a! = B então defina match = false, um falha, todos falham
      return false;
	}
  }
  return true;
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
		
		if (checkTwo( find, storedCard )) {   // Verifique se o cartão armazenado leu da EEPROM
		  return true;
		  break; 
		}
	  }
	  return false;
}

boolean isMaster( byte test[] ) {
	  if ( checkTwo( test, masterCard ) ){
		return true;
	  } else {
		return false;
	  }
}

boolean checkLevelAgua(){
	valor = analogRead(potenciometro);
	valor = map(valor,0,1023,0,100);
	Serial.print("/N");
	Serial.println(valor);
	
	if (valor =< 40/*digitalRead(MNA_PIN)*/) {       // Nível da água está baixo
		return true;
	} else {                 // Nível da água está alto;
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

void PrintLcd(String texto, int linha, boolean clear){
	if (clear){	
		lcd.clear(); 
	} 
	  
	if (sizeof(texto) > 16){
		int i = sizeof(texto) - 16;
		int j;
		for (j = 0; j >= (-1 * i); j--){
		  lcd.setCursor(j, linha);
		  lcd.print(texto);
		  delay(100);
		}
	} else {
		lcd.setCursor(0, linha);
	lcd.print(texto);
	}
}
