# Wykład Sieci Komputerowe - warstwa aplikacji HTTP

Przesyłamy tekst z odnośnikami. HTTP korzysta z protokołu TCP, portu 80 (z szyfrowanym 443).


## Adresy URL (Uniform resource locator)
Składa się z 2 częśći:
- schemat - http, https, mailto, itd.
- część zależna od schematu - mojastrona.php/wiki, 


### Dla http lub https

//nazwa_serwera(opcjonaknie jeszcze port)/identyfikator_wewnatrz_serwera.

Przykład:
http:/en.wikipedia.org/wiki/3

Przeglądarka nawiązuje połączenie TCP na porcie 80 (dla http) i klasycznie jest ACK itd.

```
GET /wiki/HTTP/3 HTTP/1.1
Host: en.wikipedia.org
```

Przeglądarka ciacha na kawałeczki te zapytanie i nawiązuje konkretne połączenie.

Następnie serwer analizuje żądanie i pobiera z dysku odpowiedni plik. Serwer sprawdza typ MIME pliku, np:
- text/plain
- text/html
- image/jpeg
- video/mpeg
- application/pdf

MIME pliku to jest typ zwracanego pliku przez serwer. Czyli serwer generuje opdowiedz typu:

```
HTTP/1.1 200 OK
Server: ...
Last-Modified: ...
Content-Length: 5387
Content-Type: text/html

TU PLIK (w tym przypadku html)
```

Przeglądarka na podstawie Content-Type wykonuje działanie, jeśli to pdf to zapisuje, jak text/html to wyświetla, itd.

### Typy odpowiedzi

O typie świeadczy pierwsza litera:
- 1xx - informacyjne
- 2xx - sukces(200 = OK)
- 3xx - przekierowania
- 4xx - błąd po stronie klienta
- 5xx - błąd po stronie serwera


## Komunikacja między serwerem a aplikacją
> Protokół HTTP jest bezstanowy

Kommunikacja:
- opcja1: każdorazowo uruchaniamie aplikacji jako proces w serwerse
- opcja2: aplikacje jest modułem serwera (w php, perl, python)
- opcja3: aplikacja rozumie HTTP i zachowuje się jak serwer HTTP. Prawdziwy serwer tylko przekazuje wtedy lub zapewnia szyfrowanie. 

Dzięki temu aplikacja może przechowywać stan.

### Stan poprzez aplikację

1. Klient wysyłą Get /app?set_name=Marcin
2. Aplikacja zwraca OK i ustawia ciasteczko. (Set-Cookie: sID=b1ah3953)

Zapamiętuje informacje o użytkowniku w idntyfikatorze sesji w ciasteczku.

Od teraz klinet HTTP posiada w nagłówku:

```
Cookie: sID=b1ah3953
```

### Formularze

#### Wysyłanie metodą GET
Przeglądarka pobiera stronę http://domena/prorram?param1=val1&param2=val2

Ograniczona liczba parametrów.


#### Wysyłanie metodą POST
Przeglądarka wysyła żądanie POST o stronę http://domena/prorram

W treści żądania, nie w nagłówku znajduje się param1=val1&param2=val2

Żądania POST nie są cachowane, GET już bardziej.

### JavaScript
Część dynamiki po stronie klienta bez komunikacji z serwerem. Js jest zintegowany z HTML i ma łątwy dostęp do dokumentu.

Poza tym Js może też wysyłać zapytania HTTP do serwera.


## Skk