# Zadanie transport

## Co się przydaa - przesuwne okno nadawcy

Nadawca utrzymuje roboczy bufor:
- Po lewej pakiety wysłane i potwierdzone.
- Potem mamy okno rozmiaru K, które ma nie potwierdzone, wysłane, część potwierdzonych.

Jak dostaniemy potwierdzenie, to patrzymy czy to nowa informacja. Jeśli jest nowa to oznaczamy na zielono (jako wysłane).

Jeśli to spowoduje, że prefiks okna jest zielony, to przesuwamy okno o długość tego prefiksu w prawo. (1 element okna nie może być zielony).

Dla każdego wysłanego mamy: wysłałeś pakiet, długo nie ma odpowiedzi, to wyślij go ponownie. Stop and wait to ten mechanizm z oknem o rozmiarze 1 - słaba prędkość. Kilkaset kb może?

Można o tym myśleć, że wielkość okna, to szybkość przesyłu. Jak przesadzimy, tozalejemy odbiorce danymi i lipa.


## Jak w wiresharku
Odpowiedź przyjdzie protokołem TCP od serwera z portu 443 np. a przychodzi do nas na port naszego komputera np 58702.

Seq= od którego bajtu
Len= jaka długość

Zmienia się rozmiar okna. 


## Do odplania:
Z liczeniem ramu:
/usr/bin/time -l ./transport 192.168.5.120 50000 moj_plik1 15000
Bez liczenia ramu:
/usr/bin/time -l ./transport 192.168.5.120 50000 moj_plik1 15000
