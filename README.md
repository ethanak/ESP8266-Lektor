# ESP8266-Lektor
Polish speech synthesizer for ESP8266 - Syntezator mowy języka polskiego dla ESP8266
### UWAGA!
Syntezator pracujący w trybie PWM7 wymaga taktowania procesora 160 MHz!
Przy kompilacji programów używających biblioteki należy bezwzgldnie
przełączyć taktowanie na 160 MHz, w przeciwnym razie głos będzie
zniekształcony i praktycznie nieczytelny.

Biblioteki Arduino dla ESP8266 w wersji na dziś zawierają krytyczny błąd.
Aby go poprawić, należy zlokalizować plik core_esp8266_i2s.c
(w przypadku Linuksa dla wersji 2.3.0 będzie to
`~/.arduino15/packages/esp8266/hardware/esp8266/2.3.0/cores/esp8266/core_esp8266_i2s.c`)
i sprawdzić, czy w funkcji `i2s_slc_end()` (linia 143 w wersji 2.3.0)
występuje wywołanie funkcji `free`. Jeśli nie, należy dopisać fragment kodu
odpowiedzialny za zwolnienie pamięci.

Jeśli więc kod funkcji wygląda tak:
```cpp
void ICACHE_FLASH_ATTR i2s_slc_end(){
  ETS_SLC_INTR_DISABLE();
  SLCIC = 0xFFFFFFFF;
  SLCIE = 0;
  SLCTXL &= ~(SLCTXLAM << SLCTXLA); // clear TX descriptor address
  SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address
}
```
należy go przerobić na następujący:
```cpp
void ICACHE_FLASH_ATTR i2s_slc_end(){
  ETS_SLC_INTR_DISABLE();
  SLCIC = 0xFFFFFFFF;
  SLCIE = 0;
  SLCTXL &= ~(SLCTXLAM << SLCTXLA); // clear TX descriptor address
  SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address
  int x;
  for (x=0; x<SLC_BUF_CNT; x++) {
    free(i2s_slc_buf_pntr[x]);
  }
}
```
