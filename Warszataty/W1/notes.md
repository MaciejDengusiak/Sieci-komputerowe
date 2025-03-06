# Warsztaty 1


### Zapytanie do serwera WWW
Mamy utworzonego GET'a. Poniższą metodą wyślemy zapytanie do example.org na port 80 z danym getem. Żeby otrzymać odkodowaną odpowiedź trzeba usunąć Accept-Encoding linijkę.

> nc -q 3 example.org 80 < HTTPHeaderLive.txt


### Otwarcie strumienia danych
> telnet example.org 80

Otwieramy w ten sposób strumień danych do example.org na porcie 80.


### Podlgąd przypiętych usług do konkretnych portów warstwy transportowej
Wartości numeryczne 
> netstat -l46n

Interpretacja za pomocą /etc/services
> netstat -l46

### Uruchomienie serwera SSH
> sudo systemctl start ssh



