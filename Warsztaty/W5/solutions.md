# Solutions - Warsztaty 5 sieci komputerowe

Do 5 kropki triv klasyka, konfiguracja i bramy domyślne. 

## Odpowiedzi na pytania:


### Jaka jest sugerowana przez maszynę Virbian2 modyfikacja tablicy routingu na maszynie Virbian1 ?
ICMP->Gateway access = 192.168.1.3 

Oznacza to, że Virbian2 sugeruje, żeby Virbian1 dodał Virbiana3(192.168.1.3) jako default aby dostać się do Virbian4.

### Dlaczego taka zmiana ma sens?
Virbian2 dosłownie przesyła wiadomość: "Niepotrzebnie wysyłasz pakiety do mnie, skoro możesz wysłać je bezpośrednio do innej bramy”

Zamiast V1->V2->V3->V4 można V1->V3->V4 i to V2 sygnalizuje.

### W jaki sposób maszyna Virbian2 mogła wykryć powyższy problem?
Virbian2 dostał pakiet z V1 które jest w tej samej sieci iprzesyła do V3 które jest w tej samej sieci. Stąd wykrywa, że V1 może bezpośrednio przesłać do V3.