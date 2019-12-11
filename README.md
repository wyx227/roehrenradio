# roehrenradio
 Umbausprojekt fürs Röhrenradio mit Raspberry Pi

 Dieses Projekt findet im Rahmen der Studienarbeit für den Kurs TEL17AAT an der DHBW Mannheim.

 Im Rahmen des Projektes wird ein Röhrenradio mit einem Raspberry PI angesteuert und als ein MP3-Player verwendet werden können.

 Die bereits implementierten Funktionen:
 1. Wiedergabe mit MPG321
 2. Ansteuern mit den vorhandenen Tasten
 3. Automatisches Erstellen einer Playliste aus den gespeicherten Liedern in einem vorgegeben Verzeichnis.

 Noch zu implementierenden Funktionen:
 1. Auslesen des USB-Speichers
 2. Statusanzeige für den Betrieb und Debuggen

 Am 10.12 wird das Programm ohne die oben erwähnten Funktionen, die noch implementiert wurden, abgegeben.
 
 Bekannte Bugs:
  1. Die Tasteneingabe funktioniert nicht einwandfrei. 
  2. Die maximale erlaubte Playlistegröße ist auf 32768 beschränkt.
 
 Verfolgen Sie bitte den Verdrahtungsplan, der in Radio beigelegt ist.
 
 Vor dem Starten des Programms bitte unbedingt die GPIO-Ports per Kommandozeilen freigeben (gpio export XXX input/output)
 
 Für weitere Unterstützung bitte die Kontaktinfo auf dem Blatt nehmen, das im Radio beigelegt ist.

 Das Projekt wird unter MIT-Lizenz veröffentlich. Änderungen, Weitergabe und kommerzielle Nutzung des bzw. eines Teils des Projektes sind ohne Zustimmung des Autors möglich.
