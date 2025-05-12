# Warsztaty 4 - sieci komputerowe

## Konfiguracja - jak zawsze.


## Tutorial 1

Dodajemy zgodnie z wskazowkami zegara domyślne bramy poleceniem, np dla V1: **ip route add default via 192.168.1.1**.

Tablice routingu zawierają po dokladnie 3 wpisy.


## Tutorial 2

Zmieniamy w /etc/frr/daemons ospfd na yes i uruchamiamy frr poprzez **systemctl start frr**.

**systemctl status frr** wskazuje że usługa routingu dynamicznego działa i rzeczywiście ospfd state -> up.

### W vtysh
Uruchamiamy **vtysh**. Wyświetlamy tablicę routingu używająć **show ip route**. 

Wchodzimy do konfiguracji routingu OSPF używając: **configure terminal** **router ospf**.

Włączamy protokół OSPF dla kolejnych sieci przyłączonych enp-rem_i przy użyciu np  **# network 192.168.1.0/24 area 0** dla VIrbian 1 i enp-rem1. Można to cofnąć używając **no network adres sieci area 0**.

Robimy **end** i upewniamy się że dobrze skonfigurowaliśmy używając **show running-config**. Zapisujemy całość używając **copy running-config startup-config**.

Nie da się sprawdzić przy pomocy show ip route odległości w vtysh. Trzeba do tego dodać show ip route 192.178.2.0/24, czyli dla konkretnego adresu.


### OSPF
Oglądamy w wiresharku i widać, ze protokół OSPF jest osadzony bezpośrednio w warstwie sieciowej IP a nie korzysta z warstwy transportowej.


### CD
Problem jest taki że po tej konfiguracji ospf nie działa mi pingowanie V3 z V1. Trzeba to naprawić. 



## Solutions
W 5 kropce - nie da się pingnąć ani 8.8.8.8 ani v2.