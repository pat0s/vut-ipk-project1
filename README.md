# Projekt 1
**Autor:** Patrik Sehnoutek (xsehno01)

Server komunikujúci prostredníctvom protokolu HTTP implementovaný v jazyku C. 

## Preloženie a spustenie

Program je možné preložiť pomocou príkazu **make** a spustiť nasledovným spôsobom.

```
$ make
$ ./hinfosvc [port-number]
```
**port-number** - číslo lokálneho portu, na ktorom server počúva

## Použitie

Server spracováva 3 dotazy:
* /hostname - slúži na získavanie doménového mena
* /cpu-name - vypísanie informácií o CPU
* /load - aktuálna záťaž CPU

Odoslanie dotazu pomocou:

**GET**
```
$ GET http://localhost:12345/hostname
$ patos-VirtualBox
```

**curl**
```
$ curl http//localhost:12345/load
$ 5.14%
```

**webového prehliadača**

![Browser example](https://github.com/pat0s/vut-ipk-project1/blob/master/cpu-name.PNG)

## Použité knižnice

```
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
```
