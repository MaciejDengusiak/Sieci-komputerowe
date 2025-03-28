# Wyzwanie - lista2

## Krok 1 - konfiguracja
Poprzez ustawienia każdej z maszyn w trybie expert w zakładce network konfigurujemy kolejne karty sieciowe połączone z wirtualnymi sieciami opisanymi w wyzwaniu.

## Krok 2 - nazwy interfejsów
Uruchomiłem wszystkie maszyny.

![alt text](image-1.png)

Nazywamy kolejne interfejsy jak na rysunku komendą **ip link set 'stara_nazwa' name 'nowa_nazwa'**. Dla rysunku z wyzwania komendy będą wyglały następująco.

### Virbian0
![alt text](image-6.png)
![alt text](image-2.png)

### Virbian1
![alt text](image-3.png)

### Virbian2
![alt text](image-4.png)


Aby upewnić się, że dobrze nazwałem interfejsy, użyłem komendy **ip link**. Przykład z virbiana 1 na którym widać już nowo ustawione nazwy kart sieciowych.

![alt text](image-5.png)


### Konfiguracja sieciowa enp-ext
![alt text](image-7.png)


## Krok 3 - przypisanie IP
Każdą kartę aktywuję poleceniem **ip link set up dev 'nazwa'** a następnie przypisuje adres IP poleceniem **sudo ip addr add 'IP' dev 'nazwa'**.

### Virbian0
![alt text](image-8.png)
### Virbian1
![alt text](image-9.png)
### Virbian2
![alt text](image-10.png)

### Sprawdzenie pingiem

Z każdej maszyny sprawdziłem osiągalność intefejsów bezpośrednio połączonych maszyn. Poniżej przykład sprawdzenia osiągalności Virbiana2 z Virbiana1.

![alt text](image-11.png)

## Krok 4 - konfiguracja routingu

### Virbian0
Określamy, że do sieci 192.168.1.0/24 można dostać się wysyłając pakiety do 192.168.0.2
![alt text](image-15.png)

### Virbian1
Ustawiamy bramę domyślną równą 192.168.0.1
![alt text](image-14.png)

### Virbian2
Ustawiamy bramę domyślną równą 192.168.1.1
![alt text](image-13.png)

## Krok 5 - test osiągalności interfejsów

Sprawdzimy czy z każdej maszyny są dostępne wszystkie interfejsy. Poniżej przykład sprawdzenia pingiem osiągalności Virbiana2 z Virbiana0, czyli IP 192.168.1.2 z Virbian0.

![alt text](image-16.png)

Trasa z maszyny Virbian2 do maszyny Virbian0 przy pomocy traceroute.

![alt text](image-17.png)


## Krok 6 - ping 8.8.8.8

### Virbian0
Otrzymujemy odpowiedź oraz podglądając wiresharki odpowiedź widzimy wyłącznie na maszynie Virbian0.
![alt text](image-21.png)
![alt text](image-18.png)

### Virbian2
W przypadku pingowania z Virbiana2 zapytanie dochodzi do do maszyny Virbian0 co obserwujemy w wiresharku. Nie otrzymujemy za to odpowiedzi. Wynika to z tego, że Vibian2 jest w lokalnej sieci, google nie wie o jego istnieniu i nie potrafi odesłać odpowiedzi. Żeby odpowiedź wracała do Virbiana2 trzebaby powiadomić google o konfiguracji naszej sieci. 

![alt text](image-20.png)
![alt text](image-19.png)