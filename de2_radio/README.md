#  $\color{Aquamarine}{\textsf{$Dokumentace ke kódu FM přijímače se Si4703}} 

Tento dokument popisuje funkci programu, který běží na AVR mikrokontroléru (16 MHz) a ovládá FM přijímač Si4703, OLED displej a čte vstupy z rotačního enkodéru a dvou tlačítek pro ovládání hlasitosti.

🧱 1. Definice frekvence CPU
#ifndef F_CPU
#define F_CPU 16000000UL 
#endif


Zajišťuje, že knihovny jako _delay_ms() budou používat správnou taktovací frekvenci mikrokontroléru – 16 MHz.

📚 2. Vložené knihovny

Program používá:

avr/io.h, avr/interrupt.h, util/delay.h
→ přístup k registrům, přerušením a časovým funkcím.

freqselector.h
→ obsluha rotačního enkodéru a výběr frekvence.

timer.h, gpio.h
→ pomocné funkce GPIO a časovačů.

OLED_RDS.h, oled.h
→ displej + RDS text.

Si4703.h
→ ovladač FM tuneru Si4703.

uart.h
→ sériová komunikace pro debug.

📡 3. Pole předvolených FM frekvencí
const int presetFreqs[] = { ... };


Obsahuje 39 frekvencí v jednotkách 0.1 MHz (např. 10130 = 101.3 MHz).
Používá je rotační enkodér pro přepínání stanic.

🧩 4. Inicializace hlavních objektů
FreqSelector freqSelector(presetFreqs, 39, PD6, PD5);
extern Si4703 radio;
OledDisplay oled;
static int lastFreq = -1;


FreqSelector
– zajišťuje čtení enkodéru (piny PD6, PD5)
– debounce 50 ms
– pracuje s polem předvolených frekvencí.

radio
– instance FM tuneru (externě definovaná).

oled
– displej s RDS.

lastFreq
– pamatuje si poslední naladěnou frekvenci, aby se stanice nepřelaďovala zbytečně.

🎚️ 5. Definice pinů tlačítek hlasitosti
#define VOL_DOWN_PIN  PD7
#define VOL_UP_PIN    PB0


Obě tlačítka jsou čtena jako vstup s interním pull-up rezistorem.

▶️ 6. Funkce main()
6.1 Inicializační sekce
uart_init(...);
oled_init(OLED_DISP_ON);
sei();


inicializace UART

zapnutí OLED displeje

povolení přerušení

Debug výpisy informují o průběhu inicializace.

Inicializace tuneru Si4703
radio.start();
radio.setChannel(10700);
radio.powerDown();
radio.powerUp();
radio.start();
radio.setMute(true);
radio.setVolume(15);


Program tuner:

spustí

nastaví kanál

vypne/zapne napájení (pro reset)

zapne znovu

inicializuje hlasitost

zapne ztlumení (mute)

Aktivace výběru frekvencí
FreqSelector::attach(&freqSelector);


Rotační enkodér je nyní aktivní a může měnit frekvence.

Nastavení OLED
oled.setRdsText("HELLO FROM RADIO STREAMING SERVICE");
oled.setFrequency(radio.getChannel());


Zobrazí uvítací text a aktuální frekvenci.

Nastavení pinů tlačítek
gpio_mode_input_pullup(&DDRD, VOL_DOWN_PIN);
gpio_mode_input_pullup(&DDRB, VOL_UP_PIN);


Oba piny se nastaví jako vstupy s pull-up rezistorem.

🔁 7. Hlavní smyčka programu

Program stále dokola:

7.1 Ovládání hlasitosti — VOLUME UP
if (gpio_read(&PINB, VOL_UP_PIN) == 0) {
    _delay_ms(30);
    ...
}


Tlačítko je stisknuté → logická 0

Proběhne 30 ms debounce

Pokud není hlasitost na maximu (15), zvýší se

OLED displej se aktualizuje

Program čeká, dokud uživatel tlačítko nepustí

7.2 Ovládání hlasitosti — VOLUME DOWN

Stejná logika jako u volume UP, ale snižuje hlasitost směrem k 0.

7.3 Čtení enkodéru (změna stanice)
int freq = freqSelector.get();
if (freq != lastFreq) {
    lastFreq = freq;
    radio.setChannel(freq);
    ...
}


Pokud uživatel otočí enkodérem:

získá se nová frekvence z předvoleného seznamu

uloží se jako poslední frekvence

naladí se tuner pomocí setChannel()

vypíše se zpráva přes UART

OLED zobrazí novou frekvenci

7.4 Aktualizace OLED displeje
oled.update();


Zajistí překreslení:

frekvence

hlasitosti

RDS textu

✔️ 8. Návratová hodnota
return 0;


– zde spíše formální, protože hlavní smyčka nikdy nekončí.
