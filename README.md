# Arduino Retro Game Console 🕹️

[cite_start]Consolă de jocuri portabilă bazată pe Arduino, cu meniu interactiv și grafică custom pe ecran OLED. 

## 🛠️ Hardware & Pinout
* **Microcontroler:** Arduino Uno / Nano 
* **Display:** OLED SSD1306 (128x32 px) via I2C 
* **Joystick:** Axe X/Y (Pinii A0, A1) 
* **Butoane:** * Buton B (Pin 3): Confirmare / Start [
    * Buton D (Pin 4): Functii Speciale / Cheat 
    * Buton E (Pin 6): Ieșire / Meniu 

## 🎮 Jocuri Incluse
1. **Car Race:** Evită vehiculele inamice pe 3 benzi. [cite_start]Include 6 tipuri de mașini cu design diferit și dificultate progresivă (viteză crescută). 
2. **Brick Breaker:** Distruge grid-ul de cărămizi folosind paleta și bila. [cite_start]Include fizică pentru unghiul de ricoșeu.

## ⚙️ Specificații Tehnice
* **Motor Grafic:** Randare bazată pe stări (State Machine) și cadre (frames).
* [**Memorie:** Sprite-urile vehiculelor sunt stocate în `PROGMEM` pentru optimizarea resurselor.
* **Funcții extra:** Sistem de "God Mode" pentru mașini și "Instant Win" pentru cărămizi (Buton D). 
