# Sieci komputerowe - Warsztaty 6

## Konfiguracja wstępna
Klasycznie tworzymy Virbian0 z jednym NATem (domyślna konfiguracja). Nazywamy kartę sieciową jako enp0 i pobieramy konfigurację sieciową przy pomocy **dhclient**.

## Tutorial 1

Sprawdzamy jakie adresy IP są przypisane do www.debian.org, używając **host -a www.debian.org**. NIestety ta komenda zwraca mi podpis kryptograficzny rekordu DNS:

 www.debian.org.		946	IN	RRSIG	NS 8 3 28800 20250610193402 20250511193402 57838 www.debian.org. CoECczDSnaTLNRGN2ig6JwBcw6zYEmIlXdxAOJMLZDQsOTTUqT5756v4 XHb8pTVEajEkrHwZZHpyr9KBNUUaDiStKzZB/zUgGojsl0zptHrGivz/ ZlbjTf4OXsl54JQrgcLenRC/NxWsZXu8Y2mW4l+1lKIfgban9u+UMvFB b4JgenaQZe1AMbHvHsMKdMUuVBAiOurk6RaP9E7f+s6ppE+ffTOyzOHx R90iqteb+YnlSFlqFhP//rn+IgNe4pOW

> [PYTANIE] Czy tak powinno być? Zamiast tego użyłem **host -t A www.debian.org**

Przykłądowy adres debiana to: 194.177.211.216.

W jednej konsoli zrobiłem **(while true; do netstat -tan | grep 194.177.211.216; done)** | tee tcp log, zaś w drugiej **wget http://194.177.211.216/**. 

To utworzyło mi plik tcp i log, niestety za szybkie łącze spowodowało, że nie da się zaobserwować stanów TCP: **gniazda SYN SENT, ESTABLISHED**. Zmniejszyłem prędkość pobierania, używając **trickle -d 10 wget http://194.177.211.216/**


### Podgląd w Wiresharku

Mamy rozmowę między:
- Source(ja) - AdresIP = 10.0.2.15, Port = 43246
- Dest(debian) - ADresIP = 194.177.211.216, Port = 80

Segment - warstwa transportowa (Tu tcp). Weźmy na przykład 1 segment, wysłanie requesta:
- Włączona flaga SYN 
- Które bajty sa przesyłąne w segmencie (squent number, next sequent number) - od 0 do 1.
- które bajty potwierdzane (Acknowledgment number) - 0


Otwarcie aktywne wykonujemy my, bo wysyłamy SYN. Aktywne zamknięcie wykonuje serwer, bo wysła FIN. 
### Flagi

1. SYN - inicjuje połączenie z serwerem
2. ACK - potwierdza otrzymanie danych
3. FIN - zakończenie połączenia


## Tutorial 2 - Protokół DNS

Polecenie **dig** odpytuje kolejne serwery DNS. Przy jego pomocy szukamy adresu IP związanego z www.cs.uni.wroc.pl. Zaczynamy od jednego z głównych serwerów: 198.41.0.4.

Czyli wykonujemy **dig www.cs.uni.wroc.pl @198.41.0.4**. Dostajemy wiele adresów odpowiedzialnych za pl. Np.: **192.102.225.53**

Ten serwer odpowie adresami serwerów DNS odpowiedzialnych za strefę pl. Kolejne zapytania będziemy doprecyzowywać. Zapytamy serwery DNS odpowiedzialne za wroc.pl, uni.wroc.pl, cs.uni.wroc.pl.


Całą pracę teraz wykonamy hednym poleceniem **dig +trace -4 www.cs.uni.wroc.pl @198.41.0.4**.

Jeśli nie podamy serwera DNS po znaku @, to zapytanie będzie wysyłane do domyślnego serwera, zdefiniowanego w pliku /etc/resolv.conf.

Sprzwdzamy adres IP, serwery nazw, serwer obsługujący pocztę dla ii.uni.wroc.pl:
- **IP**: dig -t a ii.uni.wroc.pl - 156.17.4.11
- **Serwery nazw:** dig -t ns ii.uni.wroc.pl - apollo.ii.uni.wroc.pl, hermes.ii.uni.wroc.pl
- **Serwer:** dig -t mx ii.uni.wroc.pl - 5 ALT1.ASPMX.L.GOOGLE.COM, i różne inne


Sprawdzimy nazwę domeny związaną z adresem **156.17.4.11**, używając **dig -t ptr 11.4.17.156.in-addr.arpa**.

## Tutorial 3 -  Profesjonalniej DIG

Uruchamiamy nc w trybie UDP nasłuchujące na porcie 10053: **nc -u -l -p 10053**.

W drugiej konsoli wykonujemy: **dig -p 10053 www.wikipedia.pl @127.0.0.1 +tries=1**. To wyśle jedno zapytanie DNS o adres IP dla www.wikipedia.pl z portu 10053. Zapytanie zostanie wypisane w konsoli nasłuchującej poprzez nc, które nasłuchuje na 10053.'

Dane, które się wypisują niestety są w formie binarnej. Odpalimy serwer UDP w trybie zapisu do pliku, dns_request komendą: **nc -u -l -p 10053 | tee dns_request**.

Możemy teraz odczytać formę binarną z dns_request. Możemy też podejrzeć szesnastkową odpowiedź, jak w wiresharku: **hexdump -C dns_request**. Podglądając wiresharka rzeczywiście zawartość tego pliku odpowiada datagramowi przechwyconemu w wiresharku.

Te zapytanie możemy przesłać dowolnemu serwerowi DNS, np Googleowi. Używamy do tego **nc -q 1 -u 8.8.8.8 53 < dns_request**





