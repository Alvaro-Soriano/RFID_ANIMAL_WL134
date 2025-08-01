
//declaramos los pines necesarios para el lector
const int lector = 16;
const int resetPin = 4;

//establecemos el tamaño del array de bytes que esperamos recibir
const int RespTam = 30;

//Almacenamos la recepcion del ultimo dato
unsigned long ultimoDato = millis();

//definimos un tiempo de espera entre envios en caso de que no llegue otro dato
const unsigned long tiempoEspera = 500;

//creamos un boolean que determinara si el lector en si esta leyendo
boolean leyendo = false;

//creamos un boolean que definira si se ha leido un codigo en si
boolean leido = false;

//creamos el array para almacenar la respuesta
uint8_t buffer[RespTam];
int conteoArray = 0;


void setup() {
  //iniciamos el serial
  Serial.begin(115200);
  while (!Serial);

  //iniciamos el serial del lector a 9600 y 8 bits
  Serial2.begin(9600, SERIAL_8N1, lector);

  //establecemos el pin de reset como salida y en high por defecto
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
}

//vamos a crear una función que recibira un array de bytes y un rango del mismo
//para calcular el total
uint64_t BufferDecimal(uint8_t* buffer, int inicio, int fin) {
  
  //debemos primero invertir el buffer ya que el lector responde con bytes en orden LSB, al invertirlo lo convertiremos a ASCII
  char hexLSB[fin + 1];
  for (size_t i = 0; i < fin; ++i) {
    hexLSB[i] = (char)buffer[inicio + i];
  }
  //indicamos un caracter nulo al final, queremos marcar el final de la cadena
  hexLSB[fin] = '\0';

  //ahora que ya tenemos los caracteres en orden hexadecimal los invertiremos a formato MSB, esto nos permitira convertirlo a decimal mas adelante
  char hexMSB[fin + 1];
  for (size_t i = 0; i < fin; ++i) {
    hexMSB[i] = hexLSB[fin - 1 - i];
  }

  //indicamos un caracter nulo al final, queremos marcar el final de la cadena
  hexMSB[fin] = '\0';


  //declaramos la variable que vamos a retornar
  uint64_t valorDecimal = 0;

  //recorremos el numero maximo
  for (size_t i = 0; i < fin; ++i) {
    //obtenemos el caracter
    char c = hexMSB[i];

    //creamos el digito
    uint8_t digit;

    //convertiremos de hexadecimal a decimal, 
    // de 0 a 9 lo dejamos por defecto
    // se recibimos de a-f y A-F lo traducimos en el rango de 10 a 15
    if (c >= '0' && c <= '9') digit = c - '0';
    else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
    else digit = 0;
    valorDecimal = (valorDecimal << 4) | digit;
  }


  return valorDecimal;
}

//funcion para autocompletar los ceros faltantes
String rellenaZero(String s, uint8_t width = 12) {
  while (s.length() < width) {
    s = "0" + s;
  }
  return s;
}

//creamos el bucle
void loop() {

  //mientras el serial esta disponible para leer
  while (Serial2.available()) {
    //obtenemos el byte de respuesta del lector
    uint8_t byteResp = Serial2.read();

    //almacenamos el tiempo de ejecución
    ultimoDato = millis();

    //si no estamos leyendo
    if (!leyendo) {
      //comprobamos si el byte de respuesta es el primer byte de la respuesta
      leyendo = byteResp == 0x02;

      //reinicamos el byte del array
      conteoArray = 0;
    }

    //si estamos leyendo
    if (leyendo) {
      //incrementa el contador del array y almacena el byte
      buffer[conteoArray++] = byteResp;

      //marcamos como el tag leido si hemos recibido el ultimo byte y el array esta leyendo marcamos un tag como leido
      leido = (byteResp == 0x03 && conteoArray >= RespTam);

      //almacenamos el valor leido en leyendo, mientras que le array no se ha relleno con toda la información del tag no se reseteara la lectura del tag
      leyendo = !leido;
    }
  }

  //si ha pasado cierto tiempo desde la ultima vez  que se leyo restandole el tiempo entre envio
  //enviamos la petición de reset a lector para que espere otro tag
  if (millis() - ultimoDato > tiempoEspera) {
    digitalWrite(resetPin, LOW);
    delay(200);
    digitalWrite(resetPin, HIGH);
    ultimoDato = millis();
  }

  //si no se ha leido nada salta a la siguiente ejecución del bucle
  if (!leido) return;
  leido = false;

  //Separamos del array la información del pais y el codigo
  String pais = String(BufferDecimal(buffer, 11, 4));
  String codigo = rellenaZero(String(BufferDecimal(buffer, 1, 10)));
  String tag = pais + codigo;
  Serial.println("pais: "+pais);
  Serial.println("codigo: "+codigo);
  Serial.println("tag: "+tag);

  //de paso vamos a visualizar el array entero en formato hex
  Serial.println("Respuesta sin filtrar:");
  String hexOut;
  for (size_t i = 0; i < 0 + 30; ++i) {
    if (buffer[i] < 0x10) hexOut += '0';
    hexOut += String(buffer[i], HEX);
  }
  Serial.println(hexOut);
}
