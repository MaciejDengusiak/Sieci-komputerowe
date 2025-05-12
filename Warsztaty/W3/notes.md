# Warsztaty 3 - sieci komputerowe

## Konfiguracja

Klasycznie utworzone maszyny wirtualne i interfejsy sieciowe.

Dalej aktywowałem interfejsy sieciowe enp-rem1, enp-rem2, enp-rem3 poleceniami **ip link set enp0s3 name enp-rem_i**, **ip link set up dev enp-rem_i**. 

Na Virbian 1 ustawiłem bramę domyślną na 192.168.1.2 poleceniem **ip route add default via 192.168.1.2**. Podobnie na virbianie 4 ustawiłem: **ip route add default via 192.168.3.3**.


## Tutorial 1
W pliku /etc/frr/daemons zmieniłem ripd=no na ripd=yes. (Żeby wejść w sudo zrobiłem sudo -s)

Uruchomiłem usługę **frr** za pomocą **systemctl start frr**. Następnie sprawdziłem poleceniem **systemctl status frr**. Rzeczywiście znajdował się napis **ripd state -> up**.

Wszedłem do konfiguracji routingu dynamicznego (RIP) poleceniem **vtysh**:
- Dopełniamy polecenia klawiszem **tab** jak w powłoce
- aby otrzymać pomoc kliknąć **?**
- aby wyjść używamy **exit**

Terminal konfiguracyjny vtysh działa w dwóch trybach.

### Tryb konfiguracji vtysh
Jak już wejdziemy do vtysh to do konfiguracji wchodzimy za pomocą polecenia **configure terminal**. Znak zachęty to virbian(config)#. Aby wrócić należy użyć **exit**. Również jeśli wejdziemy głębiej w konfiguracji wracamy do poprzedniego exitem. Aby zakończyć całkowicie tryb konfiguracji wpisujemy **end**



### Tryb wydawania poleceń vtysh
Tu jesteśmy z automatu wchodząc do vtysh. Znak zachęty to virbian#. Wyświetlamy dostępne intefejsy i tablicę routingu za pomocą: **show interface** oraz **show ip route**

### Ciąg dalszy RIP
W konfiguracji włączamy protokół RIP dla sieci połączonych z enp-rem1 i enp-rem2 za pomocą poleceń:
- **router rip**
- **version 2**
- **network 192.168.1.0/24**
- **network 192.168.2.0/24**

W przypadku pomyłki można cofnąć polecenie network wpisując: **no network adres_sieci**

Jeśli potem zmodyfikujemy adres poleceniem ip konfiguracja RIP przestanie działać. Należy wtedy zrestartować frr poleceniem **systemctl restart frr** i na nowo skonfigurować RIP.

Aby wyświetlić aktualną konfigurację w trybie wydawania poleceń używa się **show running config**. Zapisujemy konfigurację poleceniem **copy running-config startup-config**

Wykonałem tę samą pracę dla Virbiana3.

Do wyświetlenia tras skonfigurowanych przez RIP używamy **show ip rip** w vtysh.

### Odległości poprzez show ip rip
Odległości to 1 lub 2. Przykładowo z Virbian2 odległości to:
- 1 do 192.168.1.0/24 oraz 192.168.2.0/24
- 2 do 192.168.3.0/24

Co ma sens bo do 192.168.3.0/24 z Virbiana2 trzeba dojść przez Virbian3.

Na koniec wygląda że każdy adres IP jest osiągalny z każdej z maszyn.

## Tutorial 2
Doaktywujemy interfejsy enp-rem4 oraz przypisujemy im adresy IP.

Usuwamy domyślne trasy używając **ip route del default**. Rzeczywiście na maszynach Virbian1 i 4 są znane trasy tylko do bezpośrednio połączonych sieci.

Wykonujemy RIP dla v1 i v4.

## Co najważniejsze
dla adresu 192.168.1.0/24 trzeba jako sudo:
- w /etc/frr zmienić w deamon **ripd=yes**
- włączyć usługę frr za pomocą **systemctl start frr**
- wejść do konfiguracji RIP za pomocą **vtysh** a potem **configure terminal**
- użyć komend - **router rip**, **version 2**, **network 192.168.1.0/24**
- zapisać za pomocą **copy running-config startup-config**

Możemy wtedy wyświetlić tablice routingu w vtysh za pomocą **show ip rip**.


## Wyzwanie

1. Konfigurujemy masyzny Virbian1-Virbian5:
- w advanced->network ustawiamy lokalne sieci
- wchodzimy do każdej maszyny i w każdej odpowiednie interfejsy sieci locali nazywamy enp-loci. Następnie włączamy je. Przykład local1: **ip link set enp0s3 name enp-loc1**, **ip link set up dev enp-loc1**.

2. Dodajemy adresy IP do interfejsów. Przykład local1: **ip addr add 192.168.1.1/24 dev enp-loc1** lub dla innego interfejsu możemy  **ip addr add 192.168.1.2/24 dev enp-loc1** 

3. Ustawiamy trasy domyślne:
- w Virbian1: **ip route add default via 192.168.0.1** - domyślnie przez V2
- w Virbian5: **ip route add default via 192.168.4.1** - domyślnie przez V4

4. Włączamy protokół RIP dokładnie jak w Co najważniejsze.

5. Sprawdzamy osiągalność pingując maszyny Virbian1, Virbian3, Virbian5 io traceroutując.