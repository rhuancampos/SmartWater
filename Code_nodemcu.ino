#include <ESP8266WiFi.h>  //essa biblioteca já vem com a IDE. Portanto, não é preciso baixar nenhuma biblioteca adicional
#include <stdlib.h>
#include <string.h>

//defines
#define SSID_REDE     "Filipe Tabosa"  //coloque aqui o nome da rede que se deseja conectar
#define SENHA_REDE    "infor@2015"  //coloque aqui a senha da rede que se deseja conectar
#define TEMPO_ESPERA      30     //Tempo entre envios de dados ao servidor (em segundos)
#define INTERVALO_ENVIO_THINGSPEAK  TEMPO_ESPERA * 1000  //intervalo entre envios de dados ao ThingSpeak (em milisegundos)
 
//constantes e variáveis globais
char EnderecoAPIThingSpeak[] = "api.thingspeak.com";
String ChaveEscritaThingSpeak = "JCK0QFXRVDG51B6S"; //Chave para escrita no canal: Smart Water UPE Caruaru
long lastConnectionTime; 
WiFiClient client;
unsigned int data;
int valor;
 
//prototypes
void EnviaInformacoesThingspeak(String StringDados);
void FazConexaoWiFi(void);
float FazLeituraUmidade(void);
 
/*
 * Implementações
 */
 
//Função: envia informações ao ThingSpeak
//Parâmetros: String com a  informação a ser enviada
//Retorno: nenhum
void EnviaInformacoesThingspeak(String StringDados)
{
    if (client.connect(EnderecoAPIThingSpeak, 80))
    {         
        //faz a requisição HTTP ao ThingSpeak
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+ChaveEscritaThingSpeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(StringDados.length());
        client.print("\n\n");
        client.print(StringDados);
   
        lastConnectionTime = millis();
        Serial1.println("\n- Informações enviadas ao ThingSpeak!");
     }   
}
 
//Função: faz a conexão WiFI
//Parâmetros: nenhum
//Retorno: nenhum
void FazConexaoWiFi(void)
{
    client.stop();
    Serial1.print("Conectando-se à rede WiFi..."); 
    delay(1000);
    WiFi.begin(SSID_REDE, SENHA_REDE);
 
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial1.print(".");
    }
 
    Serial1.println("");
    Serial1.println("WiFi connectado com sucesso!");  
	  Serial1.print("\nNOME DA REDE: ");
    Serial1.println(SSID_REDE);
    Serial1.print("IP OBTIDO: ");
    Serial1.println(WiFi.localIP());
 
    delay(1000);
}
 
//Função: faz a leitura das informações recebidas do arduino UNO
//Parâmetros: nenhum
//Retorno: 
//Observação: 
void FazLeitura(void)
{
      //Criar Função
}
void setup()
{  
    Serial1.begin(115200);
	Serial2.begin(9600);
    lastConnectionTime = 0; 
    Serial1.println("\n\nNodeMCU ESP8266 Ativada\n");
    Serial1.println("\tSmart Water 1.0 (UPE CARUARU)\n\tAlunos: Filipe Tabosa | Gleyson Rhuan\n");
    FazConexaoWiFi();
    Serial1.print("\nEnviando dados a cada ");
    Serial1.print(TEMPO_ESPERA);
    Serial1.print(" Segundos...");
    Serial1.print("\nAguardando para enviar dados...");
}
 
//loop principal
void loop()
{
	
	while(Serial2.available()>0){
      data=Serial2.read();
        if(data=='/N') valor = Serial.read();
    }
	
    int   NivelPercentualTruncado,
          QuantUsersCadastrados;
    char  FieldNivelAgua[40], 
          FieldUsersCadastrados[20];
    
    NivelPercentualTruncado = rand() % 100 ; //Alterar pela função que lê valor do arduino UNO
    QuantUsersCadastrados = rand() % 300 ; //Alterar pela função que lê valor do arduino UNO
    
    //Força desconexão ao ThingSpeak (se ainda estiver desconectado)
    if (client.connected())
    {
        client.stop();
        Serial1.print("  (Nível da água: ");
        Serial1.print(NivelPercentualTruncado);
        Serial1.print("% | Usuários cadastrados: ");
        Serial1.print(QuantUsersCadastrados);
        Serial1.println(")");
        Serial1.print("\n\nAguardando para enviar dados...");
    }
         
    //verifica se está conectado no WiFi e se é o momento de enviar dados ao ThingSpeak
    if(!client.connected() && (millis() - lastConnectionTime > INTERVALO_ENVIO_THINGSPEAK)){
        sprintf(FieldNivelAgua,"field1=%d",NivelPercentualTruncado);        //Formata a string para o padrão de envio do thingspeak
        sprintf(FieldUsersCadastrados,"&field2=%d",QuantUsersCadastrados);   //Formata a string para o padrão de envio do thingspeak
        strcat(FieldNivelAgua, FieldUsersCadastrados);                      //Concatena as duas strings em uma (FieldNivelAgua)
        EnviaInformacoesThingspeak(FieldNivelAgua);                         //Envia as informações
    }else{
        Serial1.print(".");
    }
 
     delay(1000);
}
