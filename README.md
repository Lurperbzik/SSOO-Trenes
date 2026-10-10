
**Enunciado**

Práctica de SSOO realizada en el curso académico 2021/2022, el enunciado completo se encuentra en http://avellano.fis.usal.es/~labssoo/pract207.htm

**Enfoque de la solución** 

Evitar las colisiones entre trenes es trivial. Basta con asignar un recurso de sincronización (semáforo, mutex, buzón, etc.) a cada una de las coordenadas del mapa. Sin embargo, esta solución, lleva de forma inevitable a una situación de interbloqueo entre procesos.  

Por tanto, y teniendo en cuenta que la longitud de un tren L <= 19 unidades espaciales, se optó por tratar el mapa como un grafo dirigo ponderado, donde cada intersección en un nodo y la distancia entre dos nodos es el peso de la arista.
De esta forma, la primera fase del problema es hayar la división territorial que maximice el número de subgrafos de tal forma que para cada subgrafo:
	
- *La distancia entre cualquiera de sus nodos de entrada con cualquiera de sus nodos de salida >= L**

Una vez realizada esta división, que no se contempla de forma programática, quedarán definidas las secciones.

En la entrada de cada sección se coloca un semáforo y se coordina la comunicación entre procesos empleando un buzón de mensajes. Durante la ejecución del programa podemos llegar a diferentes situaciones:

* 1. Coordinación entre 2 procesos 

Si el tren A quiere acceder a la sección del tren B y viceversa:

Como la velocidad de los trenes es arbitraria, debe sicronizarse el intercambio entre secciones empleando un semáforo. Al no ser accesible la información de la longitud de los trenes, se emplea L como la unidad de seguridad. Durante los L siguientes desplazamientos, se coordina de forma intercalada el avance de A y el avance de B, de unidad espacial en unidad espacial.

* 2. Coordinación entre más de 2 procesos *

Homólogo al caso anterior pero estableciendo la coordinación de avance entre más de 2 procesos.     
 
**Implementación futura**

Queda por estudiar un algoritmo para encontrar las subsecciones de forma automática para cualquier mapa, obviando las soluciones de tiempo no polinomial.


**Ejecución del proyecto**

Compilar con: 
```bash
gcc -m32 lomo.c -L. -llomo -o lomo
```
Ejecutar:
```bash
./lomo <velocidad> <número_trenes>
```
