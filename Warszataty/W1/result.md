# Warsztaty 1 - odpowiedzi

Wpisujemy startx
## Tutorial 1
W example.org mamy 2 wysłane żądania:
- example.org/favicon - 04 NotFound
- example.org 200 OK

Z adresem example.org związany jest adres IP: 
- 96.7.128.186
- 96.7.128.192
- 23.215.0.132
- 23.215.0.133

Dla example.org w wireshark mam:
- IP - Adres źródłowy(Src): 10.0.2.15, Adres docelowy(Dst): 96.7.128.192
- TCP - Port źródłowy(Src): 45036, Port docelowy: 80


Dla odpowiedzi 200 OK dla tego żądania mamy:
- IP - Adres źródłowy(Src): 96.7.128.192, Adres docelowy(Dst): 10.0.2.15
- TCP - Port źródłowy(Src): 80, Port docelowy: 45036

Dane identyfikujące połączenie nie zmieniły się, jedynie zamieniły się miejscami, bo dla adres docelowy dla żądania jest adresem źródłowym dla odpwoedzi i vice versa.


Po kliknięciu Save w HTTP Header Live poprze nano w terminalu modyfikujemy nagłówek HTTP. Żeby zapisać plik w nano: ctrl+x, potem y potem enter i mamy.

Zostawiając jedynie 2 pierwsze wiersze GET i Host: dalej dostajemy odpowiedź.
Jeśli zostawimy tylko 1 wiersz to dostajemy odpowiedź 404 Bad Request bo tak naprawdę robimy zapytanie GET do żadnej strony.

Aby opuścić telnet wpisz quit. Do wypisania netstatem robi się l (jak lód a nie 1).

Wysyłanie danych do kilka logalnych usług:
- do serwera echa on odpowiada to samo

## Tutorial 2
Trzeba było najpierw zmienić w ustawieniach w virrtualbox w sekcji Network na local i nazwa local0. Teraz mamy obiemaszyny w tej samej lokalnej sieci czy cos.

Ustawiliśmy /24 maskę podsieci i nadaliśmy odpowiednie adresy IP maszynom.
Wchodzimy w wireshark i w enp0s3. 
Średni czas rtt to 0.2 ms na virbianie0 i 0.02 na virbian1. Znaczniki czasowe (timestamp) w zapytaniu i odpowiedzi nie różnią się.

Ustawiłem 192.168.0.2 test w etc hosts  

Udaje mi się uzyskać prędkość przeysłania na poziomie 4.43 Gbit/sec


## Wyzwanie
1. Tworzenie Virbian2 jak w tutorialu (nalezy tylko pamietac ze trzeba ustawić Hard Disk -> use existing virual hard disk file). Podłączenie karty sieciowej Adapter1 też jak w tutorialu.

2. Aktywujemy karty sieciowe na obu urządzeniach poleceniami:  **ip link set up dev enp0s3**, potem **sudo ethtool enp0s3** i mamy stan warstwy fizycznej dla interfejsu enp0s3.

3. Wykonujemy:
- Na virbian1: **sudo ip addr add 192.168.100.1/24 dev enp0s3**
- Na virbian2: **sudo ip addr add 192.168.100.2/24 dev enp0s3**

4. W virbian1 robimy **ping 192.168.100.2** i widzimy że virbian2 jest osiągalny. RTT równy 0.15-0.2. Wchodzimy w wireshark i w enp0s3. Tam wybieramy jakiś reply i podlgądamy pakiet. W sekcji Internet Protocol Version 4 (IP v4) Mamy Src: 192.168.100.1 I Dst: 192.168.100.2. 

5. Na Virbian2 robimy **iperf3 -s** . Na virbian1 robimy **iperf3 -c 102.168.100.2**. Monitorujemy bitrate, wyszło 1.34Gbit/sec.

6. Na Virbianie1 wywołujemy **telnet 102.168.100.2 7** to połączy się z serwerem echo Virbiana2, bo domyślnie port 7 to serwer echo. Podglądając pakiety widać te echo. 

7. Dekonfigurujemy na ibu virbianach: **sudo ip addr flush dev enp0s3** , **ip link set down dev enp0s3**