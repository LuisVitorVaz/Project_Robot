void setup() {
  Serial.begin(9600);

  // Configura os pinos dos LEDs como saída
  pinMode(7, OUTPUT); // LED1: conversão em andamento
  pinMode(8, OUTPUT); // LED2: conversão concluída

  // ----------- ADMUX CONFIG ----------- //
  ADMUX = 0;
  ADMUX |= (1 << REFS0); // Vref = AVcc (5V)
  ADMUX |= (0 << ADLAR); // Resultado à direita
  ADMUX |= (0b0000);     // Canal ADC0 (A0)

  // ----------- ADCSRB CONFIG ----------- //
  ADCSRB = 0; // Free Running Mode (ADTS = 000)

  // ----------- ADCSRA CONFIG ----------- //
  ADCSRA = 0;
  ADCSRA |= (1 << ADEN);  // Habilita o ADC
  ADCSRA |= (1 << ADATE); // Modo auto trigger (contínuo)
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128 → ADC clock = 125 kHz
  ADCSRA |= (1 << ADSC);  // Inicia a primeira conversão

  digitalWrite(7, HIGH);  // Ligado: conversão inicial em andamento
  digitalWrite(8, LOW);   // Desligado: nada lido ainda
}

void loop() {
  // Verifica se a conversão terminou (ADIF = 1)
  if (ADCSRA & (1 << ADIF)) {
    digitalWrite(7, LOW);   // Conversão terminou: LED1 desligado
    digitalWrite(8, HIGH);  // Resultado pronto: LED2 ligado

    uint16_t valor = ADC;   // Lê o valor convertido
    ADCSRA |= (1 << ADIF);  // Limpa o flag de fim de conversão

    Serial.println(valor);

    delay(5);               // Pequeno delay visual
    digitalWrite(7, HIGH);  // Começa nova conversão: LED1 ligado
    digitalWrite(8, LOW);   // Resultado será sobrescrito: LED2 desligado
  }
}
