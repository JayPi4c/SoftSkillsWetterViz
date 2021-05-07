# SoftSkillsWetterViz

## Inhaltsverzeichnis
* [Kurzbeschreibung](#kurzbeschreibung)
* [Inbetriebnahme](#inbetriebnahme)
  * [Demo](#demo)
* [Hardware](#hardware)
* [Software](#software)
  * [Bibliotheken](#bibliotheken)
  * [Boardmanager](#boardmanager)
  * [Code](#code)
* [Features](#features)
  * [Darstellung des Wetters](#darstellung-des-wetters)
  * [Darstellung einer Animation](#darstellung-einer-animation)

## Kurzbeschreibung

Dieser Code gehört zu dem Wetter-Gadget, welches für das Modul Soft Skills und technische Kompetenzen als Semester-Projekt ensteht. Die Aufgabe war es, Wetterdaten, bereitgestellt von [OpenWeatherMap](https://openweathermap.org/), mittels eines WiFi-Fähigen Arduinos und LEDs darzustellen.

Weitere Bedingungen des Projektes waren es, dass mindestens ein Teil des Gadgets aus dem 3D-Drucker kommen oder mit dem Laser-Cutter ausgeschnitten worden sein sollte. Nach einem differenziertem Brainstorming sind wir zu folgenden Prototypen gekommen:
![Prototyp](https://user-images.githubusercontent.com/32594337/112378671-7f89bf80-8ce7-11eb-89aa-254b4fed2ed3.jpg)

## Inbetriebnahme
Um das Gadget starten zu können muss der Code heruntergeladen werden. Sind die entsprechenden [Biblitheken](#bibliotheken) und der [Boardmanager](#boardmanager) installiert, muss nur noch die `secrets.h` Datei erstellt und mit Inhalt gefüllt werden. Dies lässt sich durch kopieren des Inhalts aus der `secrets_example.h` erreichen, webei die Variablen noch mit den persönlichen Daten gefüllt werden müssen. `SSID` und `Passwort` beziehen sich auf das lokale WLAN-Netzwerk mit dem sich der Mikro-Controller verbinden soll. Den Schlüssel für `OWM_API_KEY` erhält man kostenlos von [OpenWeatherMap.org](https://openweathermap.org/price) nach einer Anmeldung. Den Token für die `BLYNK_API_KEY`-Variable erhält man aus der Blynk App, nachdem man den QR-Code eingescannt hat.
![WhatsApp Image 2021-04-09 at 17 00 35](https://user-images.githubusercontent.com/32594337/114276554-6a22ce00-9a27-11eb-9527-97093c7d489d.jpeg)

### Demo
[![Demo-Video](https://img.youtube.com/vi/5HPHukewiGE/0.jpg)](https://www.youtube.com/watch?v=5HPHukewiGE)



## Hardware
- LOLIN(WEMOS) D1 mini Lite 
- LED-Stripe
- LED

## Software

### Bibliotheken
- FastLED (v3.3.3)
- ArduinoJson (v6.17.2)
- Blynk (v1.0.0-beta.3)
### Boardmanager
Um für den ESP8266 Code kompilieren zu können, muss in den Arduino Einstellungen im Menüpunkt "weitere Board manager URLs" folgender Link hinzugefügt werden: http://arduino.esp8266.com/stable/package_esp8266com_index.json. Anschließend kann der entsprechend Board Manager von Arduino heruntergeladen werden.

### Code
Der Code lässt sich grob in vier Abschnitte unterteilen:
* Bibliotheken und globale Variablen
* Einbindung der Blynk API
* `setup`- und `loop`-Funktionen
* weitere Helper-Funktionen

Um zu verstehen, wie die Anbindung mit Blynk funktioniert ist die [Dokumentation](https://docs.blynk.cc/) der beste Ort für Fragen und um ein Verständis zu entwickeln.
In der `setup`-Funktion wird der LED-Streifen initialisiert und eine Verbindung zu Blynk aufgebaut. Hierbei werden vor dem Verbindungsaufbaue alle Scheiben mit einer zufälligen Farbe initialisiert. Konnte eine Verbindung hergestellt werden, so gehen alle LEDs aus und es werden die Wetterdaten von der API bezogen und dargstellt. Sollte ein Fehler auftreten, so leuchten alle Scheiben rot auf.
In der `loop`-Funktion werden, abhängig von dem aktuellen Modus neue Wetterdaten bezogen oder es wird eine Animation dargestellt. Hierbei wird sich viel der Helper-Funktionen bedient.

![Diagramm](https://user-images.githubusercontent.com/32594337/117454389-614deb00-af46-11eb-9305-150a40540539.png)

Die Helper-Funktionen umfassen Methoden zur Darstellung der Wetterdaten und zum anfordern der Wetterdaten, aber auch kleine Funktionen, welche dafür sorgen, dass man ohne großen Aufwand die richtige Scheibe mit einer Farbe beleuchtet kann. Hervorzuheben ist hier allerdings, dass die Funktion `void applyConditions(bool forceUpadte)` schon einmal oben definiert sein muss, da Arduino sonst Probleme damit hat, das Standard-Argument für die Funktion anzunehmen. Ohne Standard-Argument ist die Definition des Funktions-Kopfes nicht notwendig.

## Features
Das Gadget lässt sich auf zwei Grundmodi reduzieren:
* Darstellung der aktuellen Wetterdaten
* Darstellung einer Animation

### Darstellung des Wetters

Für die Darstellung des Wetters wurden die Obergruppen der OpenWeatherMap-API verwendet. Diese umfassen:
* Sonnig
* Bewölkt
* Regnerisch
* Gewitter
* Niesel
* Schnee
* "Atmosphere"

Durch empfangen unterschiedlicher Wetterverhältnisse werden die Scheiben unterschiedlich beleuchtet und man kann das Wetter erkennen.

Im Einstellungs-Tab in der Blynk-App kann zudem noch der Ort eingestellt werden, für den die Daten bezogen werden sollen. Es ist also nicht notwendig, dies vor dem Hochladen festzulegen. Weiterhin lässt sich in dem Tab auch einstellen, in welchen Intervallen der Controller nach Updates suchen soll. Einzustellen ist ein Intervall zwischen 5 und 120 Minuten. 

### Darstellung einer Animation

Neben der Darstellung des Wetters gibt es auch verschieden Animationsmodi, welche das gesamte Gerät in schönen Farben erleuchten lassen. Um zu erkenne, dass sich das Gerät aktuell in einer Animation beindet und nicht die Wetterdaten dargestellt werden, leuchtet dauerhaft die obere LED. Die Animationen umfassen aktuell:
* Darstellung des Wetters (unabhängig des wirklichen Wetters)
* Individuelle Einfärbung der einzelnen Scheiben
* Farb-Fading Animationen

Bei der Darstellung des Wetters kann begutachtet werden, wie ein empfangenes Wetter dargestellt wird. Insbesondere in der Entwicklung ist dies sehr wertvoll, kann auch auch für den Endnutzer von Interesse sein, um zu sehen, wie welches Wetter aussieht.

Die individuelle Einfärbung erlaubt es den Nutzer jeder Scheibe eine inidviduelle Farbe zu geben und so das Gerät in einem einzigartigen glanz erscheinen zu lassen.

Die Farb-Fading Animationen sind Farbverläufe über die Scheiben und geben so ein harmonisch, faszinierendes Erscheinungsbild. Besonders hervorzuheben sind aktuell `Bounce fade` und `Offsetfade` in der App, welche beide besonders gelungene Animationen starten.

Neben diesen verschiedenen Darstellungen gibt es auch die Möglichkeit, das Gadget komplett abzudunkeln, sodass keine Lampe mehr leuchtet. Besonders im Schlafzimmer kann dies von großem Interesse sein. Dies lässt sich sowohl manuell, sowie auch per Zeitsteurung machen.
